#include	"dv_ap.h"

void	echo_jitter_init (struct echo *echo);
void	echo_jitter_flush(struct echo *echo);
void    temp_file_close (struct echo *echo);
void    dv_pkt_set(struct dv_packet *hdr);
void    echo_header_send_set(void);
void    echo_server_terminate (struct echo *echo);
void	short_msg(struct echo *echo, char voice[]);

enum
{
        ECHO_SKIP = 0,
        ECHO_HEADER,
        ECHO_VOICE,
        ECHO_LAST
} echo_state = ECHO_SKIP;

struct	timeval echo_interval;
struct	timeval	echo_temp;
char	echo_buff[58];
extern	char	lastframe[];

void	echo_server_header (char header[])
{
	time_t	atime;
	struct	echo	*echo_tmp;
	struct	echo	*echo_next;

	time(&atime);
	echo_next = echo_pnt;
	while (echo_next)
	{
		if (!memcmp (&echo_next->frame_id, &header[12], 2)) 
		{
			gettimeofday (&echo_next->EchoInTime, NULL);
			return;
		}
		echo_next = echo_next->next;
	}
	echo_tmp = malloc (sizeof (struct echo));
	if (echo_tmp == NULL)
	{
		fprintf (log_file, "%24.24s not memory allocation for echo table\n", ctime(&atime));
		return;
	}
	echo_tmp->next = NULL;
	echo_jitter_init (echo_tmp);
	echo_tmp->tmp_file = tmpfile();
	echo_tmp->echo_state = ECHO_SKIP;
	echo_tmp->recv_seq = 0;
	echo_tmp->in_cnt = 0;
	echo_tmp->out_cnt = 0;
	gettimeofday (&echo_tmp->EchoInTime, NULL);
	memcpy (&echo_tmp->frame_id, &header[12], 2);
	memcpy (&echo_tmp->callsign, &header[42], 8);
	memset (echo_tmp->msg, 0x20, 20);
       	if (echo_tmp->tmp_file == NULL)
       	{
               	fprintf (log_file, "%24.24s tmpfile open error %s\n",
                       	ctime(&atime), strerror(errno));
               	fflush (log_file);
               	return;
	}
	if (echo_pnt == NULL) echo_pnt = echo_tmp;
	else
	{
		echo_next = echo_pnt;
		while (echo_next)
		{
			if (echo_next->next == NULL)
			{
				echo_next->next = echo_tmp;
				break;
			}
			echo_next = echo_next->next;
		}
	}
}

int	echo_server_voice (char voice[])
{
	unsigned char	frame_seq;
	struct	echo	*echo_next;

	echo_next = echo_pnt;
	while (echo_next)
	{
		if (!memcmp (&echo_next->frame_id, &voice[12], 2))
		{
                        if (voice[14] & 0x40)
                        {
                                memcpy (&voice[24], lastframe, 6);
                                memset (&echo_next->frame_id, 0x00, 2);
				echo_server_terminate (echo_next);
                        }
                        else
			{
				gettimeofday (&echo_next->EchoInTime, NULL);
				frame_seq = voice[14] & 0x1f;
				if (frame_seq < 21)
				{
					memcpy (&echo_next->jitter[frame_seq][0], &voice[15], 12);
					echo_next->jitter_cnt++;
					if (echo_next->jitter_cnt > 10)
						echo_jitter_flush (echo_next);
					short_msg (echo_next, voice);
					echo_next->in_cnt++;
				}
			}
			return TRUE;
		}
		echo_next = echo_next->next;
	}
	return FALSE;
}

int	echo_server_last (char	voice[])
{
	struct	echo	*echo_next;

	echo_next = echo_pnt;
	while (echo_next)
	{
		if (!memcmp (&echo_next->frame_id, &voice[12], 2))
		{
			echo_server_terminate (echo_next);
			return TRUE;
		}
		echo_next = echo_next->next;
	}
	return FALSE;
}

void	echo_server_terminate (struct echo *echo)
{
	unsigned short int tmp;
	time_t	atime;

	while (echo->jitter_cnt)
	{
		echo_jitter_flush(echo);
	}
	gettimeofday (&echo_temp, NULL);
	echo_interval.tv_sec = 1;
	echo_interval.tv_usec = 0;
	timeradd (&echo_temp, &echo_interval, &echo->send_time);
	fflush (echo->tmp_file);
	rewind (echo->tmp_file);
	echo->EchoInTime.tv_sec = 0;
	echo->EchoInTime.tv_usec = 0;
	memset (&echo->frame_id, 0x00, 2);
	time(&atime);
	echo->in_cnt++;
	echo->echo_state = ECHO_HEADER;
	fprintf (log_file, "%24.24s echo received packets %ld from %8.8s (%20.20s)\n",
		ctime(&atime), echo->in_cnt, echo->callsign, echo->msg);
	fflush (log_file);
}

int	echo_server_send (struct echo *echo)
{
	time_t atime;

	gettimeofday (&echo_temp, NULL);
	switch (echo->echo_state)
	{
		case ECHO_SKIP:
			break;

		case ECHO_HEADER:
			if (timercmp (&echo->send_time, &echo_temp, >)) break;
			echo->echo_state = ECHO_VOICE;
			echo_interval.tv_sec = 0;
			echo_interval.tv_usec = 100000;  /* 100 m sec. */
			timeradd (&echo_temp, &echo_interval, &echo->send_time);
			memset (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header, 0x00, 41);
			echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.flags[0] = 0x06; /* auto reply */
			memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.RPT1Call, gateway_callsign, 8);
			memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.RPT2Call, echo_area_rep_callsign, 8);
			memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall, echo->callsign, 8);
			memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.MyCall, echo_server, 8);
			memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.MyCall2, "ECHO", 4);
			echo_header_send_set();
			echo_dv_pkt.pkt_type = 0x10;
			echo->seq = 0;
			if (echo->msg[0] == '@')
			{
				memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall, &echo->msg[1], 8);
			}
			putFifo (56, echo_dv_pkt);
			break;

		case ECHO_VOICE:
			if (timercmp (&echo->send_time, &echo_temp, >)) break;
			memcpy (&echo_temp, &echo->send_time, sizeof (echo_temp));
			echo_interval.tv_sec = 0;
			echo_interval.tv_usec = 20000;  /* 20 m sec. */
			timeradd (&echo_temp, &echo_interval, &echo->send_time);
			if (fread (echo_buff, 12, 1, echo->tmp_file) == 1)
			{
				echo->out_cnt++;
				memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.voice_d, &echo_buff, 12);
                               	dv_pkt_set(&echo_dv_pkt);
                               	echo_dv_pkt.dstar.b_bone.b_b.seq = echo->seq;
                               	echo->seq++;
				echo_dv_pkt.pkt_type = 0x20;
                               	if (echo->seq > 20) echo->seq = 0;
				putFifo (27, echo_dv_pkt);
				break;
			}
			if (feof (echo->tmp_file))
				echo->echo_state = ECHO_LAST;
			break;

		case ECHO_LAST:
                        dv_pkt_set(&echo_dv_pkt);
                        memcpy (&echo_dv_pkt.dstar.b_bone.dstar_udp.voice_d.voice_segment,
                                &lastframe[3], 3);
                        echo_dv_pkt.dstar.b_bone.b_b.seq = echo->seq | 0x40;
                        echo->out_cnt++;
                        echo_dv_pkt.pkt_type = 0x20;
                        putFifo (30, echo_dv_pkt);
			time(&atime);
			fprintf (log_file, "%24.24s echo sent packets %ld to %8.8s\n",
					ctime(&atime), echo->out_cnt, echo->callsign);
			fflush (log_file);
			temp_file_close (echo);
			return TRUE;
	}
	return FALSE;
}

void	temp_file_close (struct echo *echo)
{

	struct	echo	*echo_next;
	struct	echo	*echo_save;

        fclose (echo->tmp_file);
        echo_save = echo;
        echo_next = echo_pnt;
        if (echo_save == echo_pnt)
        {
        	echo_pnt = echo_save->next;
                free (echo_save);
        }
        else
        {
                while (echo_next)
                {
                	if (echo_next->next == echo_save)
                        {
                        	echo_next->next = echo_save->next;
                                free (echo_save);
                                break;
                        }
                        echo_next = echo_next->next;
               }
	}
}


char    DummyVoice[12] = {0x9e,0x8d,0x32,0x88,0x26,0x1a,0x3f,0x61,
                                0xe8,0xe7,0x84,0x76};
char    ReSync[3] = {0x55, 0x2d, 0x16};

void	echo_jitter_init(struct echo *echo)
{
	int	k;

	for (k = 0; k < 21 ; k++)
	{
		memcpy (&echo->jitter[k][0], DummyVoice, 12);
	}
	memcpy (&echo->jitter[0][9], ReSync, 3);
	echo->jitter_out_pnt = 0;
	echo->jitter_cnt = 0;
}

void	echo_jitter_flush (struct echo *echo)
{
	fwrite (&echo->jitter[echo->jitter_out_pnt][0], 12, 1, echo->tmp_file);
	memcpy (&echo->jitter[echo->jitter_out_pnt][0], DummyVoice, 12);
	if (echo->jitter_out_pnt == 0) memcpy (&echo->jitter[0][9], ReSync, 3);
	echo->jitter_out_pnt++;
	if (echo->jitter_out_pnt >= 21) echo->jitter_out_pnt = 0;
	echo->jitter_cnt--;
}

void	echo_jitter_flush_ex (void)
{
	struct	echo	*echo_next;
	struct	timeval timeout;
	struct	timeval current_time;
	struct	timeval	tmp_time;

	gettimeofday (&current_time, NULL);

	timeout.tv_sec = 0;
	timeout.tv_usec = 500000;

	echo_next = echo_pnt;
	while (echo_next)
	{
		if (echo_next->jitter_cnt > 10) echo_jitter_flush (echo_next);
		if (echo_next->EchoInTime.tv_sec  || echo_next->EchoInTime.tv_usec)
		{
			timersub (&current_time, &echo_next->EchoInTime, &tmp_time);
			if (timercmp (&tmp_time, &timeout, >))
			{
				time(&cur_time);
				fprintf (log_file, "%24.24s drop packet %ld sec.  %ld msec.\n", 
					ctime(&cur_time), tmp_time.tv_sec, tmp_time.tv_usec/1000);
				echo_server_terminate (echo_next);
				break;
			}
		}
		echo_next = echo_next->next;
	}
}

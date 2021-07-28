#include "dv_ap.h"

void	IDxxPlus_close(void);
void	inet_send_buff_set(void);
void	inet_send_position_update (char call[]);
void	send_IDxxPlus_init (void);
void	IDxxPlus_read (void);
int	IDxxPlus_open (void);
void	send_IDxxPlus_alive (void);
void	IDxxPlus_reinit (void);
int	IDxxPlus_status (void);
void    IDxxPlus_write (int length, char buff[]);
void 	ReqPositionUpdate (struct dv_packet *pkt);
void    status_send_ptton (void);
void    status_send_pttoff (void);
void    status_send_update (struct dv_packet *pkt);
void	IDxxPlus_send_wait_set(void);
void    short_message(struct aprs_msg *msg, struct dv_packet *voice);
int     IDxxPlus_getFifo (char pkt[]);
void    IDxxPlus_LastFrameSend (void);

char	init_pkt[] = {0xff, 0xff, 0xff};
char	alive[] = {0x02, 0x02, 0xff};
int	gw_on;
unsigned	char	IDxxPlus_buff[1024];
time_t  IDxxPlus_alive_send_time;
char	IDxxPlus_send_buff[128];
char	IDxxPlus_last_frame_seq;

extern	char	lastframe[];

enum
{
        RIG_OPEN = 0,
        RIG_INIT,
        RIG_INIT_DONE,
        RIG_READ
} IDxxPlus_state = RIG_OPEN;

void	IDxxPlus_init_recv (int length)
{
	int	i;

	for (i = 0 ; i < length - 2 ; i++)
	{
		if ((IDxxPlus_buff[i] == 0xff)
			&& (IDxxPlus_buff[i+1] == 0xff)
			&& (IDxxPlus_buff[i+2] == 0xff))
			{
				time (&IDxxPlus_alive_recv);
				IDxxPlus_state = RIG_READ;
				return;
			}
	}
	return;
}
	
void	IDxxPlus(void)
{
	time_t	cur_time;

	switch (IDxxPlus_state)
	{
		case RIG_OPEN:
			if (!IDxxPlus_open()) sig_term = TRUE;
			else	IDxxPlus_state = RIG_INIT;
			break;

		case RIG_INIT:
			send_IDxxPlus_init();
			IDxxPlus_state = RIG_INIT_DONE;
			time (&IDxxPlus_init_time);
			IDxxPlus_alive_send_time = 0;
			break;
	
		case RIG_INIT_DONE:
			time (&cur_time);
			if ((cur_time - IDxxPlus_init_time) > 1)
				IDxxPlus_state = RIG_INIT;
			break;

		case RIG_READ:
			time (&cur_time);
                	if ((cur_time - IDxxPlus_alive_send_time) >= 1)
                	{
				IDxxPlus_write (3, alive);
                        	IDxxPlus_alive_send_time = cur_time;
               		}
                	if ((cur_time - IDxxPlus_alive_recv) >= 10)
                	{
                        	IDxxPlus_reinit();
                	}
			break;
	}
}

void	IDxxPlus_reinit(void)
{
	time(&cur_time);
	fprintf (log_file, "%24.24s rig not terminal/access mode\n", ctime(&cur_time));
	fflush (log_file);
	IDxxPlus_state = RIG_INIT;
}

void	IDxxPlus_read (void)
{
	int	length;
	int	len;
	int	k;
	int	ret;
	unsigned short int	tmp;
	int	n;

	length = read (IDxxPlus_fd, &IDxxPlus_buff[IDxxPlus_buff_pnt], 1024 - IDxxPlus_buff_pnt);


	if (IDxxPlus_state != RIG_READ)
	{
		IDxxPlus_init_recv (length);
		return;
	}

	if (length < 0)
	{
		time (&cur_time);
		fprintf (log_file,  "%24.24s Rig read error %s\n", ctime(&cur_time), strerror(errno));
		fflush (log_file);
		IDxxPlus_close();
		return;
	}

	IDxxPlus_buff_pnt += length;
	while (IDxxPlus_buff_pnt)
	{
		if (IDxxPlus_buff[0] != 0xff)
		{
			len = IDxxPlus_buff[0];
			if (len > IDxxPlus_buff_pnt) return;

			if (len == 3)
			{
				if (IDxxPlus_buff[0] == 0x03) time (&IDxxPlus_alive_recv);	
			}
			else if (len == 4)
			{
				//syslog (LOG_INFO, "%2.2x %2.2x %2.2x %2.2x", IDxxPlus_buff[1], IDxxPlus_buff[2], IDxxPlus_buff[3], IDxxPlus_buff[4]);
			}
			else if (len == 44)	/* header */
			{
				time(&cur_time);
				fprintf (log_file, "%24.24s %8.8s from Rig\n", ctime(&cur_time), &IDxxPlus_buff[29]);
				gw_on = FALSE;
				dv_pkt_set (&IDxxPlus_pkt);
				memcpy (&IDxxPlus_pkt.dstar.b_bone.dstar_udp.rf_header, &IDxxPlus_buff[2], 41);
				ReqPositionUpdate (&IDxxPlus_pkt);
        			IDxxPlus_pkt.pkt_type = 0x10;
        			IDxxPlus_pkt.dstar.b_bone.b_b.send_terminal_id = 0x04;
        			IDxxPlus_pkt.dstar.b_bone.b_b.id = 0x20;      /* voice */
				tmp = rand() & 0xffff;
        			memcpy (IDxxPlus_pkt.dstar.b_bone.b_b.frame_id, &tmp, 2);
				IDxxPlus_pkt.dstar.b_bone.b_b.seq = 0x80;
        			IDxxPlus_pkt.dstar.b_bone.dstar_udp.rf_header.flags[0] &= 0xbf;       /* clear repeater flag */
        			if (dv_status.port) status_send_ptton();
				putFifo (56, IDxxPlus_pkt);
				memcpy (&IDxxPlus_pkt_header, &IDxxPlus_pkt, 56);
				IDxxPlus_voice_send_sw = TRUE;
				gettimeofday(&IDxxPlusCosOffTime, NULL);
			}
			else if (len == 16)	/* voice */
			{
        			memcpy (&IDxxPlus_pkt.dstar.b_bone.dstar_udp.voice_d, &IDxxPlus_buff[3], 12);
        			dv_pkt_set(&IDxxPlus_pkt);
        			IDxxPlus_pkt.pkt_type = 0x20;
        			IDxxPlus_pkt.dstar.b_bone.b_b.send_terminal_id = 0x04;
                                memcpy (&IDxxPlus_pkt.dstar.b_bone.b_b.seq, &IDxxPlus_buff[3], 13);
                                if (IDxxPlus_pkt.dstar.b_bone.b_b.seq == 20)
                                {
                                        putFifo (56, IDxxPlus_pkt_header);
                                }
        			if (IDxxPlus_pkt.dstar.b_bone.b_b.seq & 0x40)
        			{
                			memcpy (&IDxxPlus_pkt.dstar.b_bone.dstar_udp.voice_d.data_segment, lastframe, 6);

                			if (IDxxPlus_voice_send_sw) putFifo (30, IDxxPlus_pkt);
                			if (dv_status.port) status_send_pttoff();
                			IDxxPlus_last_frame_sw = FALSE;
                			if (IDxxPlus_gw_resp_sw)
                        			IDxxPlus_send_wait_set();
				}
                		else
				{
                        		IDxxPlus_NoRespReply_sw = TRUE;
                			if (!memcmp (&IDxxPlus_buff[4], &lastframe[3], 3)
                				&& !memcmp (&IDxxPlus_buff[13], &lastframe, 3))
                			{
                        			memset (&IDxxPlus_pkt.dstar.b_bone.dstar_udp.voice_d, 0x00, 3);
                			}
                			short_message (&dvap_msg, &IDxxPlus_pkt);
                			if (IDxxPlus_voice_send_sw) putFifo (27, IDxxPlus_pkt);
                			if (dv_status.port) status_send_update(&IDxxPlus_pkt);
				}
				gettimeofday(&IDxxPlusCosOffTime, NULL);
        		}
		}
		else
			len = 1;

		k = IDxxPlus_buff_pnt - len;
		if (k > 0) memmove (&IDxxPlus_buff[0], &IDxxPlus_buff[len], k);
		IDxxPlus_buff_pnt = k;
	}
}

void	IDxxPlus_write (int length, char buff[])
{
	int	write_len;
	int	new_posit;
	int	new_length;
	int	n;

	new_length = length;
	new_posit = 0;
	write_len = 0;

	while (new_length)
	{
		write_len = write (IDxxPlus_fd, &buff[new_posit], new_length);
		new_posit += write_len;
		new_length -= write_len;
	}
	
}

void	send_IDxxPlus_init (void)
{
	int	len;

	IDxxPlus_write (3, init_pkt);
	time (&IDxxPlus_init_time);
}

int	IDxxPlus_status (void)
{
	if (IDxxPlus_state == RIG_READ) return TRUE;
	return FALSE;
}



int	IDxxPlus_open(void)
{
        struct termios attr;
	int	wrt_len;

        IDxxPlus_fd = open(IDxxPlus_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (IDxxPlus_fd < 0) 
	{
		time (&cur_time);
		fprintf (log_file, "%24.24s USB open error %s\n", ctime (&cur_time), IDxxPlus_DEVICE);
		fflush (log_file);
                return FALSE;
        }

        tcgetattr(IDxxPlus_fd, &attr);
        save_attr = attr;

        cfsetispeed(&attr, IDxxPlus_SPEED);
        cfsetospeed(&attr, IDxxPlus_SPEED);
        cfmakeraw(&attr);
        attr.c_cflag |= CS8 | CLOCAL | CREAD;
        attr.c_iflag = IGNPAR;
        attr.c_oflag = 0;
        attr.c_lflag = 0;
        attr.c_cc[VMIN] = 0;
        attr.c_cc[VTIME] = 0;

        tcsetattr(IDxxPlus_fd, TCSANOW, &attr);
	ioctl (IDxxPlus_fd, TCSETS, &attr);

	time (&cur_time);
	fprintf (log_file, "%24.24s RIG(ID-xxPlus)  open\n", ctime(&cur_time));
	fflush (log_file);
	FD_SET (IDxxPlus_fd, &save_rfds);
	return TRUE;
}

void	IDxxPlus_close (void)
{
	tcsetattr (IDxxPlus_fd, TCSANOW, &save_attr);
	close (IDxxPlus_fd);
	time (&cur_time);
	fprintf (log_file, "%24.24s RIG(ID-xxPlus) down.\n", ctime(&cur_time));
	fflush (log_file);
	FD_CLR (IDxxPlus_fd, &save_rfds);
}

void	send_IDxxPlus (void)
{
	int	length;
	int	n;

	length = IDxxPlus_getFifo (&IDxxPlus_send_buff[3]);
	if (length == 42)
	{
		IDxxPlus_write (42, &IDxxPlus_send_buff[3]);
		IDxxPlus_seq = 0;
	}
	else if (length == 13)
	{
		IDxxPlus_send_buff[0] = 0x10;
		IDxxPlus_send_buff[1] = 0x22;
		IDxxPlus_send_buff[2] = IDxxPlus_seq & 0xff;
		IDxxPlus_seq++;
		IDxxPlus_send_buff[16] = 0xff;
		IDxxPlus_write (17, IDxxPlus_send_buff);
		if (IDxxPlus_send_buff[3] & 0x40)
		{
			IDxxPlus_send_sw = FALSE;
			memset (IDxxPlus_save_frame_id, 0x00, 2);
		}
		IDxxPlus_last_frame_seq = IDxxPlus_send_buff[3];
	}
	else
	{
		memset (IDxxPlus_save_frame_id, 0x00, 2);
		IDxxPlus_LastFrameSend ();
		IDxxPlus_send_sw = FALSE;
	}
}

void    IDxxPlus_LastFrameSend (void)
{
	unsigned  char NullVoice[12] = {0x9e,0x8d,0x32,0x88,0x26,0x1a,0x3f,0x61,0xe8,0x55,0x2d,0x16};

        IDxxPlus_send_buff[0] = 0x10;
        IDxxPlus_send_buff[1] = 0x22;
        IDxxPlus_send_buff[2] = IDxxPlus_seq & 0xff;
	IDxxPlus_send_buff[3] = IDxxPlus_last_frame_seq & 0xff;
	IDxxPlus_send_buff[3] |= 0x40;
        memcpy (&IDxxPlus_send_buff[4], NullVoice, 12);
        IDxxPlus_send_buff[16] = 0xff;
        IDxxPlus_write (17, IDxxPlus_send_buff);
}


int	IDxxPlus_getFifo	(char pkt[])
{
	struct	FifoPkt	*tmp;
	int	len;

	if (IDxxPlusRp->next ==	NULL)	return	0;
	tmp = IDxxPlusRp;
	IDxxPlusRp = IDxxPlusRp->next;
	len = IDxxPlusRp->length;
	memcpy (pkt, &IDxxPlusRp->pkt, IDxxPlusRp->length);
	free (tmp);
	IDxxPlusFifo_cnt--;
	return	len;
}

void    IDxxPlus_putFifo (int len, unsigned char pkt[])
{
        struct FifoPkt  *ret;
        ret = malloc (sizeof(struct FifoPkt) - sizeof(struct dv_packet) + len);
        if (ret == NULL)
        {
		time (&cur_time);
                fprintf (log_file, "%24.24s memory error in IDxxPlus Fifo\n", ctime(&cur_time));
		fflush (log_file);
                return;
        }
        ret->next = NULL;
        ret->length = len;
        memcpy (&ret->pkt, pkt, len);
        IDxxPlusWp->next = ret;
        IDxxPlusWp = ret;
        IDxxPlusFifo_cnt++;
}


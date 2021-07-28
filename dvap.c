#include	"dv_ap.h"

void	dvap_voice_send(char voice[]);
void	dvap_header_send(char header[]);
void	dvap_skip(void);
void	dvap_send_wait_set(void);
int	header_check(unsigned char string[]);
void	short_message(struct aprs_msg *msg, struct dv_packet *voice);
void	ReqPositionUpdate(struct dv_packet *pkt);
void	status_send_ptton(void);
void	status_send_pttoff(void);
void	status_send_update(struct dv_packet *pkt);

struct	termios	save_attr;
char	dvap_buff[800];
char	dvap_header[47];
char	dvap_voice[18];
extern	char	lastframe[];
extern	char	dvap_ptt_on[];
extern	char	dvap_ptt_off[];
int	dvap_check_wait;
char	dvap_check_status;
int	dvap_loop_cnt;
int	band_scan_sw;
char	rssi[201];

char		RunState[5]   = {0x05, 0x00, 0x18, 0x00, 0x01};
unsigned char	band_scan[11] = {0x0b, 0x20, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

enum
{
	DVAP_IDLE = 0,
	DVAP_ERROR_CHECK,
	DVAP_ERROR_CHECK_WAIT,
	DVAP_MODULATION,
	DVAP_MODULATION_CHECK,
	DVAP_MODULATION_WAIT,
	DVAP_FREQUENCY,
	DVAP_FREQUENCY_CHECK,
	DVAP_FREQUENCY_WAIT,
	DVAP_TX_FREQUENCY,
	DVAP_TX_FREQUENCY_CHECK,
	DVAP_TX_FREQUENCY_WAIT,
	DVAP_RX_FREQUENCY,
	DVAP_RX_FREQUENCY_CHECK,
	DVAP_RX_FREQUENCY_WAIT,
	DVAP_SQUELCH,
	DVAP_SQUELCH_CHECK,
	DVAP_SQUELCH_WAIT,
	DVAP_CALIBRATION,
	DVAP_CALIBRATION_CHECK,
	DVAP_CALIBRATION_WAIT,
	DVAP_RUN,
	DVAP_CALIBRATION_CLEAR,
	DVAP_CALIBRATION_CLEAR_CHECK,
	DVAP_CALIBRATION_CLEAR_WAIT,
	DVAP_BAND_SCAN,
	DVAP_BAND_SCAN_CHECK,
	DVAP_BAND_SCAN_WAIT
} dvap_state = DVAP_ERROR_CHECK;

void	dvap_last_frame_send (void)
{
	if (!dvap_voice_send_sw) return;
	if (!dvap_last_frame_sw) return;
	dvap_rig_pkt_cnt++;
	dvap_rig_pkt_total_cnt++;
	memcpy (&dvap_pkt.dstar.b_bone.dstar_udp.voice_d, &dvap_voice[6], 12);
	dv_pkt_set(&dvap_pkt);
	dvap_pkt.pkt_type = 0x20;
	dvap_pkt.dstar.b_bone.b_b.send_terminal_id = 0x04;
	dvap_pkt.dstar.b_bone.b_b.seq = dvap_voice[4];
	memcpy (&dvap_pkt.dstar.b_bone.dstar_udp.voice_d.data_segment, lastframe, 6);
	putFifo (30, dvap_pkt);
	if (dv_status.port) status_send_pttoff();
	dvap_last_frame_sw = FALSE;
	if (dvap_gw_resp_sw)
	{
		dvap_send_wait_set();
	}
}

void	dvap(void)
{
	int		offset_tmp;
	long int	freq;
	int		k;
	int		step;
	ssize_t		wrt_len;

	char		modulation[5]	= {0x05, 0x00, 0x28, 0x00, 0x01};
//	char    	TargetName[4]	= {0x04, 0x20, 0x01, 0x00};
	char		frequency[8]	= {0x08, 0x00, 0x20, 0x02, 0x00, 0x00, 0x00, 0x00};
	signed char	squelch[5]	= {0x05, 0x00, 0x80, 0x00, 0x00};
	char		calibration[6]	= {0x06, 0x00, 0x00, 0x04, 0x00, 0x00};
	char		error_check[4]	= {0x04, 0x20, 0x05, 0x00};

	struct	timeval	timer_tmp1;
	struct	timeval timer_tmp2;

	switch (dvap_state)
	{
		case DVAP_IDLE:
			if (dvap_last_frame_sw)
			{
				gettimeofday (&timer_tmp2, NULL);
				timersub (&timer_tmp2, &dvap_InTime, &timer_tmp1);
				timer_tmp2.tv_sec = 0;
				timer_tmp2.tv_usec = 200000;
				if (timercmp (&timer_tmp1, &timer_tmp2, >)) 
				{
					dvap_last_frame_send();
				}	
			}
			break;

		case DVAP_ERROR_CHECK:
			dvap_check_wait = FALSE;
			wrt_len = write (dvap_fd, error_check, 4);
			if (wrt_len < 0) 
			{
				dvap_close();
				return;
			}
			dvap_loop_cnt = 0;
			dvap_state = DVAP_ERROR_CHECK_WAIT;
			break;

		case DVAP_ERROR_CHECK_WAIT:
			if (!dvap_check_wait) 
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_ERROR_CHECK;		
				}
				break;
			}
			if (dvap_check_status != 0x00)
				dvap_state = DVAP_ERROR_CHECK;
			else
				dvap_state = DVAP_MODULATION;
			break;

		case DVAP_MODULATION:
			wrt_len = write (dvap_fd, modulation, 5);
			if (wrt_len < 0)
			{
				dvap_close ();
				return;
			}
			dvap_state = DVAP_MODULATION_CHECK;
			break;

		case DVAP_MODULATION_CHECK:
                	dvap_check_wait = FALSE;
                	wrt_len = write (dvap_fd, error_check, 4);
                	if (wrt_len < 0)
                	{
				dvap_close();
				return;
			}
			dvap_loop_cnt = 0;
			dvap_state = DVAP_MODULATION_WAIT;
			break;

		case DVAP_MODULATION_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_MODULATION;
				}
				break;
                        }
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_MODULATION_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_MODULATION;
			}
			else
			{
				dvap_state = DVAP_FREQUENCY;
				time(&cur_time);
				fprintf (log_file, "%24.24s DVAP D-STAR Modulation set\n", ctime(&cur_time));
				fflush (log_file);
			}
			break;

		case DVAP_FREQUENCY:
			if (dvap_freq)
			{
				frequency[3] = 0x02;
				frequency[4] = dvap_freq & 0xff;
				frequency[5] = (dvap_freq >> 8) & 0xff;
				frequency[6] = (dvap_freq >> 16) & 0xff;
				frequency[7] = (dvap_freq >> 24) & 0xff;
				wrt_len = write (dvap_fd, frequency, 8);
				if (wrt_len < 0)
				{
					time(&cur_time);
					fprintf (log_file, "%24.24s DVAP write (freuqncy) error %s\n",
						ctime(&cur_time), strerror(errno));
					fflush (log_file);
					dvap_close();
					return;
				}
				dvap_state = DVAP_FREQUENCY_CHECK;
			}
			else 
				dvap_state = DVAP_TX_FREQUENCY;
			break;

		case DVAP_FREQUENCY_CHECK:
			dvap_check_wait = FALSE;
			wrt_len = write (dvap_fd, error_check, 4);
			if (wrt_len < 0)
			{
				dvap_close();
				return;
			}
			dvap_loop_cnt = 0;
			dvap_state = DVAP_FREQUENCY_WAIT;
			break;

			case DVAP_FREQUENCY_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_FREQUENCY;
				}
				break;
			}
                        if (dvap_check_status == 0x01)
                        {
                                dvap_state = DVAP_FREQUENCY_CHECK;
                        }
                        else if (dvap_check_status != 0x00)
                        {
                                dvap_state = DVAP_FREQUENCY;
                        }
                        else
			{
				dvap_state = DVAP_TX_FREQUENCY;
				time(&cur_time);
				fprintf(log_file, "%24.24s DVAP TX/RX Frequency Set %ld Hz\n",
					ctime(&cur_time), dvap_freq);
				fflush(log_file);
			}
			break;

		case DVAP_TX_FREQUENCY:
			if (dvap_tx_freq)
			{
				frequency[3] = 0x01;
				frequency[4] = dvap_tx_freq & 0xff;
				frequency[5] = (dvap_tx_freq >> 8) & 0xff;
				frequency[6] = (dvap_tx_freq >> 16) & 0xff;
				frequency[7] = (dvap_tx_freq >> 24) & 0xff;
				wrt_len = write (dvap_fd, frequency, 8);
				if (wrt_len < 0)
				{
					dvap_close ();
					return;
				}
				dvap_state = DVAP_TX_FREQUENCY_CHECK;
			}
			else
				dvap_state = DVAP_RX_FREQUENCY;
			break;

		case DVAP_TX_FREQUENCY_CHECK:
			dvap_check_wait = FALSE;
			wrt_len = write (dvap_fd, error_check, 4);
			if (wrt_len < 0)
                        {
				dvap_close();
				return;
                        }
			dvap_loop_cnt = 0;
			dvap_state = DVAP_TX_FREQUENCY_WAIT;
			break;

		case DVAP_TX_FREQUENCY_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_TX_FREQUENCY;
				}
				break;
			}
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_TX_FREQUENCY_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_TX_FREQUENCY;
			}
			else
			{
				dvap_state = DVAP_RX_FREQUENCY;
				time(&cur_time);
				fprintf (log_file, "%24.24s DVAP TX Frequency Set %ld Hz\n",
					ctime(&cur_time), dvap_tx_freq);
				fflush (log_file);
			}
			break;

		case DVAP_RX_FREQUENCY:
			if (dvap_rx_freq)
			{
				frequency[3] = 0x00;
				frequency[4] = dvap_rx_freq & 0xff;
				frequency[5] = (dvap_rx_freq >>  8) & 0xff;
				frequency[6] = (dvap_rx_freq >> 16) & 0xff;
				frequency[7] = (dvap_rx_freq >> 24) & 0xff;
				wrt_len = write (dvap_fd, frequency, 8);
				if (wrt_len < 0)
				{
					dvap_close ();
					return;
				}
				dvap_state = DVAP_RX_FREQUENCY_CHECK;
			}	
			else 
				dvap_state = DVAP_SQUELCH;
			break;

		case DVAP_RX_FREQUENCY_CHECK:
			dvap_check_wait = FALSE;
			wrt_len = write(dvap_fd, error_check, 4);
			if (wrt_len < 0)
			{
				dvap_close();
				return;
			}
			dvap_loop_cnt = 0;
			dvap_state = DVAP_RX_FREQUENCY_WAIT;
			break;

		case DVAP_RX_FREQUENCY_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_RX_FREQUENCY;
				}
				break;
			}
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_RX_FREQUENCY_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_RX_FREQUENCY;
			}
			else
			{
				dvap_state = DVAP_SQUELCH;
				time(&cur_time);
				fprintf (log_file, "%24.24s DVAP RX Frequency Set %ld Hz\n",
					ctime(&cur_time), dvap_rx_freq);
				fflush (log_file);
			}
			break;

		case DVAP_SQUELCH:
			if (dvap_squelch)
			{
				squelch[4] = dvap_squelch;
				wrt_len = write (dvap_fd, squelch, 5);
				dvap_state = DVAP_SQUELCH_CHECK;
			}
			else 
				dvap_state = DVAP_CALIBRATION;
			break;

		case DVAP_SQUELCH_CHECK:
                        dvap_check_wait = FALSE;
                        wrt_len = write (dvap_fd, error_check, 4);
                        if (wrt_len < 0)
                        {
                                dvap_close();
                                return;
                        }
                        dvap_loop_cnt = 0;
                        dvap_state = DVAP_SQUELCH_WAIT;
                        break;

		case DVAP_SQUELCH_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_SQUELCH;
				}
				break;
			}
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_SQUELCH_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_SQUELCH;
			}
			else
			{
				dvap_state = DVAP_CALIBRATION;
				time(&cur_time);
				fprintf (log_file, "%24.24s DVAP Squelch Set %d\n",
				ctime(&cur_time), dvap_squelch);
				fflush (log_file);
			}
			break;

		case DVAP_CALIBRATION:
			if (!dvap_auto_calibration || dvap_auto_calibration_set)
			{
				calibration[4] = dvap_calibration & 0xff;
				calibration[5] = (dvap_calibration >> 8) & 0xff;
				wrt_len = write (dvap_fd, calibration, 6);
				dvap_check_wait = FALSE;
				dvap_state = DVAP_CALIBRATION_CHECK;
			}
			else
				dvap_state = DVAP_RUN;
			dvap_auto_calibration_set = FALSE;
			break;

		case DVAP_CALIBRATION_CHECK:
			wrt_len = write (dvap_fd, error_check, 4);
			dvap_loop_cnt = 0;
			dvap_state = DVAP_CALIBRATION_WAIT;
			break;

		case DVAP_CALIBRATION_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_CALIBRATION;
				}
				break;
			}
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_CALIBRATION_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_CALIBRATION;
			}
			else
			{
				dvap_state = DVAP_RUN;
				time(&cur_time);
				fprintf (log_file, "%24.24s DVAP Calibration Set %d Hz\n",
					ctime(&cur_time), dvap_calibration);
				fflush (log_file);
			}
			break;

		case DVAP_RUN:
			RunState[4] = 0x01;
			wrt_len = write(dvap_fd, RunState, 5);
			if (wrt_len < 0)
			{
				dvap_close ();
				return;
			}
			dvap_state = DVAP_IDLE;
			time (&dvap_keep_alive);
			break;

		case DVAP_CALIBRATION_CLEAR:
			RunState[4] = 0x00;
			wrt_len = write(dvap_fd, RunState, 5);
			calibration[4] = 0x00;
			calibration[5] = 0x00;
			wrt_len = write (dvap_fd, calibration, 6);
			dvap_check_wait = FALSE;
			dvap_state = DVAP_CALIBRATION_CLEAR_CHECK;
			break;

                case DVAP_CALIBRATION_CLEAR_CHECK:
			wrt_len = write(dvap_fd, error_check, 4);
			dvap_loop_cnt = 0;
			dvap_state = DVAP_CALIBRATION_CLEAR_WAIT;
			break;

		case DVAP_CALIBRATION_CLEAR_WAIT:
			if (!dvap_check_wait)
			{
				if (dvap_loop_cnt++ > 50)
				{
					dvap_state = DVAP_CALIBRATION_CLEAR;
				}
				break;
			}
			if (dvap_check_status == 0x01)
			{
				dvap_state = DVAP_CALIBRATION_CLEAR_CHECK;
			}
			else if (dvap_check_status != 0x00)
			{
				dvap_state = DVAP_CALIBRATION_CLEAR;
			}
			else
				dvap_state = DVAP_BAND_SCAN;
			break;

		case DVAP_BAND_SCAN:
			freq = dvap_freq - 10000;
			step = 201;
                	band_scan[4] = step & 0xff;
                	band_scan[5] = (step >> 8) & 0xff;
                	band_scan[6] = 0x01;
                	band_scan[7] = freq & 0xff;
                	band_scan[8] = (freq >> 8) & 0xff;
                	band_scan[9] = (freq >> 16) & 0xff;
                	band_scan[10] = (freq >> 24) & 0xff;
                	band_scan_sw = FALSE;
                	wrt_len = write (dvap_fd, band_scan, 11);
                	dvap_state = DVAP_BAND_SCAN_CHECK;
			break;

		case DVAP_BAND_SCAN_CHECK:
			if (band_scan_sw)
			{
				dvap_state = DVAP_BAND_SCAN_WAIT;
			}
			break;

		case DVAP_BAND_SCAN_WAIT:
			offset_tmp = 0;
			for (k = 0 ; k < 201 ; k++)
			{
				offset_tmp += rssi[k];
			}
			offset_tmp /= 2;
			freq = dvap_freq - 10000;
			time(&cur_time);
			for (k = 0 ; k < 201 ; k++)
			{
				offset_tmp -= rssi[k];
				if (offset_tmp == 0)
				{
					dvap_calibration = freq - dvap_freq;
					
					fprintf(log_file, "%24.24s DVAP Calibration offset %d Hz\n", 
						ctime(&cur_time), dvap_calibration);
					break;
				}
				else if (offset_tmp < 0)
				{
					dvap_calibration = freq - dvap_freq - 50;
					fprintf(log_file, "%24.24s DVAP Calibration offset %d Hz\n", 
						ctime(&cur_time), dvap_calibration);
					break;
				}
				else 
					freq += 100;
			}
			dvap_auto_calibration_set = TRUE;
			dvap_state = DVAP_ERROR_CHECK;
			break;
	}
}
			
void	dvap_open(void)
{
	struct	termios attr;
	ssize_t	wrt_len;


	if (dvap_area_rep_callsign[0] == 0x20) 
	{
		dvap_sw = FALSE;
		dvap_fd = -1;
		return;
	}

	dvap_fd = open(DVAP_DEVICE, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (dvap_fd < 0) 
	{
		if (dvap_sw)
		{
			time(&cur_time);
                	fprintf(log_file, "%24.24s open error %s\n", ctime(&cur_time), DVAP_DEVICE);
		}
		dvap_sw = FALSE;;
		return;
	}

	tcgetattr(dvap_fd, &attr);
	save_attr = attr;

	cfsetispeed(&attr, DVAP_SPEED);
	cfsetospeed(&attr, DVAP_SPEED);
	cfmakeraw(&attr);
	attr.c_cflag |= CS8 | CLOCAL | CSTOPB;
	attr.c_iflag	= IGNPAR;
	attr.c_oflag	= 0;
	attr.c_lflag	= 0;
	attr.c_cc[VMIN] = 0;
	attr.c_cc[VTIME] = 0;

	tcsetattr(dvap_fd, TCSANOW, &attr);

	dvap_state = DVAP_ERROR_CHECK;;

	dvap_buff_pnt = 0;
	dvap_InTime.tv_sec = 0;
	dvap_InTime.tv_usec = 0;

	dvap_sw = TRUE;
	gettimeofday (&dvap_in_time, NULL);

	memset (&dvap_save_frame_id, 0x00, 2);
	memset (&dvap_pkt.dstar.b_bone.b_b.frame_id, 0x00, 2);

	time(&cur_time);
	fprintf(log_file, "%24.24s DVAP open/reopen\n", ctime(&cur_time));
	fflush(log_file);
	FD_SET (dvap_fd, &save_rfds);
	RunState[4] = 0x00;
	wrt_len = write(dvap_fd, RunState, 5);
	return;
}

void	dvap_close (void)
{
	ssize_t	wrt_len;

	RunState[4] = 0x00;
	wrt_len = write (dvap_fd, RunState, 5);
	tcsetattr (dvap_fd, TCSANOW, &save_attr);
	close (dvap_fd);
	dvap_NoRespReply_sw = FALSE;
	dvap_skip();
	dvap_gw_resp_sw = FALSE;
	dvap_last_frame_sw = FALSE;
	dvap_voice_send_sw = FALSE;
	if (dvap_sw)
	{
		time(&cur_time);
		fprintf(log_file, "%24.24s DVAP down.\n", ctime(&cur_time));
		fflush(log_file);
	}
	dvap_sw = FALSE;
	FD_CLR (dvap_fd, &save_rfds);
}

void	dvap_send_header (void)
{
	time (&dvap_recv_time);
	if (!memcmp (&dvap_header[25], "CALIBRAT", 8))
	{
		dvap_state = DVAP_CALIBRATION_CLEAR;;
		return;
	}
//	if (memcmp (&dvap_header[17], dvap_area_rep_callsign, 8)) return;
	if (!memcmp(dvap_pkt.dstar.b_bone.b_b.frame_id, &dvap_header[2], 2)) return;

	memcpy (&dvap_pkt.dstar.b_bone.dstar_udp.rf_header, &dvap_header[6], 41);
	dvap_pkt.dstar.b_bone.dstar_udp.rf_header.flags[0] &= 0xbf;		/* clear repeater flag */

	dv_pkt_set(&dvap_pkt);
	dvap_pkt.pkt_type = 0x10;
	dvap_pkt.dstar.b_bone.b_b.send_terminal_id = 0x04;
        dvap_pkt.dstar.b_bone.b_b.id = 0x20;      /* voice */
        memcpy(dvap_pkt.dstar.b_bone.b_b.frame_id, &dvap_header[2], 2);
        dvap_pkt.dstar.b_bone.b_b.seq = dvap_header[4];
	if (dvap_header == 0)
	{
		putFifo (56, dvap_pkt);
	}
	ReqPositionUpdate (&dvap_pkt);
	if (dv_status.port) status_send_ptton();

	if (memcmp (&dvap_header[9], &dvap_header[17], 8))
	{
		putFifo (56, dvap_pkt);
		memcpy (&dvap_pkt_header, &dvap_pkt, 56);
		dvap_voice_send_sw = TRUE;
		dvap_msg.AprsSend = 0x00;
		gettimeofday(&DvapCosOffTime, NULL);
	}
	else
	{
	dvap_voice_send_sw = FALSE;
	}

	dvap_NoRespReply_sw = FALSE;
	memcpy (&dvap_NoResp, &dvap_header[6], 41);
	dvap_last_frame_sw = TRUE;
	dvap_rig_pkt_cnt = 0;
}

void	dvap_send_voice (void)
{
	dvap_rig_pkt_cnt++;
	dvap_rig_pkt_total_cnt++;
	memcpy(&dvap_pkt.dstar.b_bone.dstar_udp.voice_d, &dvap_voice[6], 12);
	dv_pkt_set(&dvap_pkt);
	dvap_pkt.pkt_type = 0x20;
	dvap_pkt.dstar.b_bone.b_b.send_terminal_id = 0x04;
	dvap_pkt.dstar.b_bone.b_b.seq = dvap_voice[4];
	if (dvap_voice[4] & 0x40) 
	{
		memcpy(&dvap_pkt.dstar.b_bone.dstar_udp.voice_d.data_segment, lastframe, 6);
		
		if (dvap_voice_send_sw) putFifo(30, dvap_pkt);
		if (dv_status.port) status_send_pttoff();
		dvap_last_frame_sw = FALSE;
		if (dvap_gw_resp_sw)
			dvap_send_wait_set();
		else
			dvap_NoRespReply_sw = TRUE;
	}
	else
	{
		if (!memcmp(&dvap_voice[6], &lastframe[3], 3) && !memcmp(&dvap_voice[15], &lastframe, 3))
		{
			memset(&dvap_pkt.dstar.b_bone.dstar_udp.voice_d, 0x00, 3);
		}
		short_message(&dvap_msg, &dvap_pkt);
		if (dvap_voice_send_sw) putFifo(27, dvap_pkt);
		if (dv_status.port) status_send_update(&dvap_pkt);
	}
	gettimeofday(&DvapCosOffTime, NULL);
}

void	dvap_read(void)
{
	int	len;
	int	k;
	int	n;
	int	header_check_rtn;
	char	dvap_ack[3] = {0x03, 0x60, 0x00};
	struct	timeval	in_time;
	struct	timeval	tmp_time;
	struct	timeval	interval_time;
	ssize_t	wrt_len;

	interval_time.tv_sec  = 0;
	interval_time.tv_usec = 200000;
	gettimeofday(&in_time, NULL);
	timersub(&in_time, &dvap_in_time, &tmp_time);
	if (timercmp(&tmp_time, &interval_time, >)) dvap_buff_pnt = 0; 
	dvap_in_time.tv_sec = in_time.tv_sec;
	dvap_in_time.tv_usec = in_time.tv_usec;

	len = read (dvap_fd, &dvap_buff[dvap_buff_pnt], sizeof(dvap_buff));
	if (len < 0)
	{
		time(&cur_time);
		fprintf(log_file, "%24.24s DVAP read error %s\n", 
			ctime(&cur_time), strerror(errno));
		fflush(log_file);
		dvap_close();
		return;
	}
	dvap_buff_pnt += len;

	while (dvap_buff_pnt)
	{
		n = dvap_buff[0] | ((dvap_buff[1] & 0x1f) << 8);
		if (n > dvap_buff_pnt) return;

		/* header */
		if (n == 47)
		{
			if ((dvap_buff[1] & 0xe0) == 0xa0)	/* header */
			{
				dvap_ack[2] = 0x01;
				wrt_len = write(dvap_fd, dvap_ack, 3);
				if ((header_check_rtn = header_check(&dvap_buff[6])) == ALLOW)
				{
					gettimeofday(&dvap_InTime, NULL);
					memcpy(dvap_header, dvap_buff, 47);
					dvap_send_header();
					dvap_msg.msg_pnt = 0;
					memset(dvap_msg.short_msg, 0x20, 20);
				}
				else if (header_check_rtn == APRS) dvap_voice_send_sw = FALSE;
			}	/* ack */
			else if ((dvap_buff[1] & 0xe0) == 0x60)
			{
				//dvap_header_send_ok = TRUE;
			}
		}
		/* voice */
		if ((n == 18) && ((dvap_buff[1] & 0xe0) == 0xc0))
		{
			dvap_ack[2] = 0x02;
			wrt_len = write(dvap_fd, dvap_ack, 3);

			gettimeofday(&dvap_InTime, NULL);
			memcpy(dvap_voice, dvap_buff, 18);
			dvap_send_voice();
		}

		if ((dvap_buff[2] == 0x04) && (dvap_buff[3] == 0x04))
		{
			band_scan_sw = TRUE;
			memcpy(rssi, &dvap_buff[4], 201);
		}
		/* status / error code */
		if (n >= 5)
		{
			if ((dvap_buff[2] == 0x05) && (dvap_buff[3] == 0x00))
			{
				if ((dvap_buff[1] & 0xe0) == 0x00)
				{
					if (n == 5)
					{
						dvap_check_wait = TRUE;
						dvap_check_status = dvap_buff[4];
						if (dvap_check_status)
						{
							time(&cur_time);
							fprintf(log_file, "%24.24s DVAP Error Code %2.2x\n", 
								ctime(&cur_time), dvap_buff[4]);
							fflush(log_file);
						}
					}
					else
					{
						if (n > 6)
						{
							time(&cur_time);
							fprintf(log_file, "%24.24s DVAP Error String ", ctime(&cur_time));
							for (k = 4 ; k < dvap_buff[0] ; k++)
							{
								fprintf(log_file, "%c", dvap_buff[k]);
							}
							fprintf(log_file, "\n");
							fflush(log_file);
						}
					}
				}
			}
			if (n == 7)
			{
				if ((dvap_buff[2] == 0x90) && (dvap_buff[3] == 0x00)) 
					dvap_squelch_status = dvap_buff[5];
			}
		}	

		k = dvap_buff_pnt - n;
		if (k > 0) memmove(&dvap_buff[0], &dvap_buff[n], k);
		dvap_buff_pnt = k;
	}
}
	

void	send_keep_alive(void)
{
	int	wrt_len; 
	char	keep_alive_str[3] = {0x03, 0x60, 0x00};

	wrt_len = write(dvap_fd, keep_alive_str, 3);
	if (wrt_len < 0)
	{
		dvap_close();
		return;
	}
	time(&dvap_keep_alive);
}

	

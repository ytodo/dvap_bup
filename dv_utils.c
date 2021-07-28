#include	"dv_ap.h"

unsigned short int	update_crc_dstar( unsigned short int crc, unsigned char c );
unsigned short int	result_crc_dstar(unsigned short int crc);
int	callsign_check(char CallSign[]);
int	check_get_send_req_gateway_wait(void);
void	upnp_msearch_set(void);
void	gps_a(struct aprs_msg *msg);
void	gps(struct aprs_msg *msg);
void	status_short_message(char msg[]);

extern	int node_response_state;
void	node_term(void);


void	sig_init(void)
{
	sigemptyset(&save_sigset);
	sigaddset(&save_sigset, SIGUSR1);
	sigaddset(&save_sigset, SIGUSR2);
	sigaddset(&save_sigset, SIGTERM);
	sigaddset(&save_sigset, SIGINT);
}

void	node_close(void)
{
	usb_reset(udev);
	usb_close(udev);
	node_term();
}

void	node_term(void)
{
	node_NoRespReply_sw = FALSE;
	node_response_state = 0;
	node_gw_resp_sw = FALSE;
	node_last_frame_sw = FALSE;
	dvap_voice_send_sw = FALSE;
	memset(&node_save_frame_id, 0x00, 2);
	if (node_sw)
	{
		time(&cur_time);
		fprintf(log_file, "%24.24s Node Adapter down.\n", ctime(&cur_time));
		fflush(log_file);
	}
	node_sw = FALSE;
}

void	dv_pkt_set(struct dv_packet *hdr)
{
	memcpy(hdr->id, "DSVT", 4);
	memset(hdr->filler, 0x00, 2);	
	hdr->dstar.b_bone.b_b.dest_repeater_id = 0x00;
	hdr->dstar.b_bone.b_b.send_repeater_id = 0x01;
	hdr->dstar.b_bone.b_b.send_terminal_id = 0x03;
}

int	cos_check(void)
{
	if (usb_control_msg(udev, 0xC0, GET_AD_STATUS, 0, 0, &status, 1, 100) < 0)
	{
		node_close();		/* node adapter down ? */
		return FALSE;
	}
	if (status & COS_OnOff) return TRUE;
	return FALSE;
}

int	ptt_check(void)
{
	if (dvap_area_rep_callsign[0] != 0x20) return FALSE;
	if (IDxxPlus_area_rep_callsign[0] != 0x20) return FALSE;
	if (usb_control_msg(udev, 0xC0, GET_AD_STATUS, 0, 0, &status, 1, 100) < 0)
	{
		node_close();		/* node adapter down ? */
		return FALSE;
	}
	if (status & PTT_OnOff) return TRUE;
	return FALSE;
}

int	htoi(const char *s)
{
	int n;

	if ( *s != '0' || !(*(s+1) != 'x' || *(s+1) != 'X') ) return 0;
    
	for (n=0, s+=2 ; *s ; s++)
	{
		if ( *s >= '0' && *s <= '9' ) 
		{
			n = 16 * n + (*s - '0');
		}
		else if ( *s >= 'a' && *s <= 'f' ) {
			n = 16 * n + ((*s - 'a') + 10);
		}
		else if ( *s >= 'A' && *s <= 'F' ) {
			n = 16 * n + ((*s - 'A') + 10);
		}
	}
    
	return n;
}

void	printOnOff(char sw)
{
	if (sw) fprintf(log_file, "ON\n");
	else	fprintf(log_file, "OFF\n");
}

void	putFifo(int len, struct dv_packet pkt)
{
	struct	FifoPkt	*ret;

	ret = malloc(sizeof(struct FifoPkt) - sizeof(struct dv_packet) + len);
	if (ret == NULL)
	{
		fprintf(log_file, "memory error\n");
		fflush(log_file);
		return;
	}
	ret->next = NULL;
	ret->length = len;
	memcpy (&ret->pkt, &pkt, len);
	Wp->next = ret;
	Wp = ret;
}

int	getFifo(int *len, struct dv_packet *pkt)
{
	struct	FifoPkt	*tmp;

	if (Rp->next == NULL) return FALSE;
	tmp = Rp;
	Rp = Rp->next;
	*len = Rp->length;
	memcpy(pkt, &Rp->pkt, Rp->length);
	free(tmp);
	return TRUE;
}

void	aprs_putFifo(int len, char  pkt[])
{
	struct	aprsFifoPkt	*ret;
	ret = malloc(sizeof(struct aprsFifoPkt) - sizeof(struct dv_packet) + len);
	if (ret == NULL)
	{
		fprintf(log_file, "memory error\n");
		fflush(log_file);
		return;
	}
	ret->next = NULL;
	ret->length = len;
	memcpy (ret->data, pkt, len);
	aprs_Wp->next = ret;
	aprs_Wp = ret;
}

int	aprs_getFifo(int *len, char pkt[])
{
	struct	aprsFifoPkt	*tmp;

	if (aprs_Rp->next == NULL) return FALSE;
	tmp = aprs_Rp;
	aprs_Rp = aprs_Rp->next;
	*len = aprs_Rp->length;
	memcpy(pkt, &aprs_Rp->data, aprs_Rp->length);
	free(tmp);
	return TRUE;
}

void	ReqPositionInfo(char call[])
{
	char ReqPosition[16];

	memset(ReqPosition, 0x00, 16);
	ReqPosition[0] = 0x01;
	memcpy(&ReqPosition[8], call, 8);
	ReqPosition[4] = 0x12;
	sendto(in_sd, &ReqPosition, 16, 0, trust_sock->ai_addr, trust_sock->ai_addrlen);
}

void	ReqAreaPositionInfo(char call[])
{
	char ReqPosition[16];

	memset (ReqPosition, 0x00, 16);
	ReqPosition[0] = 0x01;
	memcpy (&ReqPosition[8], call, 8);
	ReqPosition[4] = 0x14;
	sendto(in_sd, &ReqPosition, 16, 0, trust_sock->ai_addr, trust_sock->ai_addrlen);
}

void	read_trust(char buff[], int len)
{
	if (((buff[4] == 0x12) || (buff[4] == 0x14)) && (buff[2] == 0x80) && (buff[3] == 0x00))
	{
		if (check_get_send_req_gateway_wait())
		{
			get_position_sw = TRUE;
			memcpy (gateway_position, buff, len);
		}
	}
	if (buff[4] == 0x11)
	{
		if (memcmp (Gw_IP, &buff[32], 4))
		{
			memcpy(Gw_IP, &buff[32], 4);
			time(&cur_time);
			fprintf(log_file, "%24.24s GatewayIpUpdate resp. %d.%d.%d.%d (port:%d)\n", 
				ctime(&cur_time), 
				buff[32], buff[33], buff[34], buff[35], buff[36] << 8 | buff[37]);
			fflush(log_file);
		}
	}
}

void	ReqPositionUpdate(struct dv_packet *pkt)
{
	char	temp[32];

	memset(temp, 0x00, 32);
	temp[0] = 0x01;
	temp[4] = 0x11;
	memcpy(&temp[16], gateway_callsign, 8);

	if (node_area_rep_callsign[0] != 0x20)
	{
		memcpy(&temp[24], node_area_rep_callsign, 8);
	}
	if (dvap_area_rep_callsign[0] != 0x20)
	{
		memcpy(&temp[24], dvap_area_rep_callsign, 8);
	}
	if (IDxxPlus_area_rep_callsign[0] != 0x20)
	{
		memcpy(&temp[24], IDxxPlus_area_rep_callsign, 8);
	}
	temp[23] = 0x20;
	memcpy(&temp[8], pkt->dstar.b_bone.dstar_udp.rf_header.MyCall, 8);
//	in_addr_len = sizeof(struct sockaddr_storage);
        sendto(in_sd, &temp, 32, 0, trust_sock->ai_addr, trust_sock->ai_addrlen);
}

int	ReqPositionUpd(void)
{
	char	temp[32];

	memset(temp, 0x00, 32);
	temp[0] = 0x01;
//	temp[1] = 0x04;
	temp[4] = 0x11;
	memcpy(&temp[16], gateway_callsign, 8);

	if (node_area_rep_callsign[0] != 0x20)
	{
		memcpy(&temp[8],  node_area_rep_callsign, 8);
		memcpy(&temp[24], node_area_rep_callsign, 8);
	}
	if (dvap_area_rep_callsign[0] != 0x20)
	{
		memcpy(&temp[8],  dvap_area_rep_callsign, 8);
		memcpy(&temp[24], dvap_area_rep_callsign, 8);
	}
	if (IDxxPlus_area_rep_callsign[0] != 0x20)
	{	
		memcpy(&temp[8],  IDxxPlus_area_rep_callsign, 8);
		memcpy(&temp[24], IDxxPlus_area_rep_callsign, 8);
	}
	temp[23] = 0x20;
//      in_addr_len = sizeof(struct sockaddr_storage);
	if (sendto(in_sd, &temp, 32, 0, trust_sock->ai_addr, trust_sock->ai_addrlen) < 0)
	{
		fprintf(log_file, "%24.24s position update packet sent error %s\n",
			ctime(&cur_time), strerror(errno));
		fflush(log_file);
		return FALSE;
	}

	keep_alive += keep_alive_interval;
	if (upnp_sw) upnp_msearch_set();
	return TRUE;
}

void	send_echo_position(void)
{
	char	temp[32];

	if (echo_server[0] == 0x20) return;

	memset(temp, 0x00, 32);
	temp[0] = 0x01;
	temp[4] = 0x11;

	memcpy(&temp[8],  echo_server, 8);
	memcpy(&temp[16], gateway_callsign, 8);
	memcpy(&temp[24], echo_area_rep_callsign, 8);

	sendto (in_sd, &temp, 32, 0, trust_sock->ai_addr, trust_sock->ai_addrlen);

	if (debug)
	{
		fprintf(log_file, "%24.24s Echo Server Position sent\n", ctime (&echo_position_send_time));
	fflush (log_file);
	}
	echo_position_send_time += echo_position_send_interval;
}

void	dv_log_send(struct dv_packet *pkt)
{
	char	log_temp[80];
	int	ret;

	struct	timeval	c_time;

	if (debug >= 3) return;

	if (!memcmp(pkt->dstar.b_bone.dstar_udp.rf_header.RPT1Call, "DIRECT  ", 8) 
		|| !memcmp(pkt->dstar.b_bone.dstar_udp.rf_header.RPT2Call, "DIRECT  ", 8))
	return;

	memset(&hints, 0x00, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = PF_UNSPEC;
	sprintf(PORT, "%d", logd_port);
	if ((ret = getaddrinfo (trust_name, PORT, &hints, &logd_sock)) != 0)
	{
		time (&cur_time);
		fprintf(log_file, "%24.24s getaddrinfo error(logd sock) %s\n",
			ctime(&cur_time), gai_strerror(ret));
		fprintf(log_file, "%24.24s trust : %s\n", ctime(&cur_time), trust_name);
		fflush(log_file);
	}
	if ((logd_sd = socket(logd_sock->ai_family, logd_sock->ai_socktype, logd_sock->ai_protocol)) < 0)
        {
		fprintf(log_file, "%24.24s socket error in logd sock %s\n",
			ctime(&cur_time), strerror(errno));
		fflush(log_file);
		freeaddrinfo (logd_sock);
		return;
	}

	ret = connect(logd_sd, logd_sock->ai_addr, logd_sock->ai_addrlen);
	int	nodelay_flag = 1;
	setsockopt(logd_sd, IPPROTO_TCP, TCP_NODELAY, (void*) &nodelay_flag, sizeof(int));
	memset(log_temp, 0x00, 80);
	memcpy(log_temp, "DSLG", 4);
	log_temp[7] = 0x01;
	memcpy(&log_temp[8], gateway_callsign, 8);
	gettimeofday(&c_time,  NULL);

	log_temp[16] = (c_time.tv_sec >> 24) & 0xff;
	log_temp[17] = (c_time.tv_sec >> 16) & 0xff;
	log_temp[18] = (c_time.tv_sec >> 8) & 0xff;
	log_temp[19] = c_time.tv_sec & 0xff;
	log_temp[20] = (c_time.tv_usec >> 24) & 0xff;
	log_temp[21] = (c_time.tv_usec >> 16) & 0xff;
	log_temp[22] = (c_time.tv_usec >> 8) & 0xff;
	log_temp[23] = c_time.tv_usec & 0xff;
	memcpy(&log_temp[24], pkt->dstar.b_bone.dstar_udp.rf_header.MyCall,   8);
	memcpy(&log_temp[32], pkt->dstar.b_bone.dstar_udp.rf_header.YourCall, 8);
	memcpy(&log_temp[48], pkt->dstar.b_bone.dstar_udp.rf_header.RPT2Call, 8);
	memcpy(&log_temp[56], &gateway_position[16], 8);
	memcpy(&log_temp[64], pkt->dstar.b_bone.dstar_udp.rf_header.RPT1Call, 8);
	memcpy(&log_temp[72], &gateway_position[24], 8);

	ret = send(logd_sd, log_temp, 80, 0);
	ret = close (logd_sd);
	freeaddrinfo(logd_sock);
	logd_sd = 0;

}

void	short_message(struct aprs_msg *msg, struct dv_packet *voice)
{
	int	data_length;
	if ((voice->dstar.b_bone.b_b.seq & 0x1f) == 0x00) return;
	if ((voice->dstar.b_bone.b_b.seq & 0x1f) % 2)
	{
		msg->tmp.mini_header = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[0] ^ 0x70;
		msg->tmp.temp[0] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[1] ^ 0x4f;
		msg->tmp.temp[1] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[2] ^ 0x93;
	}
	else
	{
		msg->tmp.temp[2] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[0] ^ 0x70;
		msg->tmp.temp[3] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[1] ^ 0x4f;
		msg->tmp.temp[4] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[2] ^ 0x93;
		switch (msg->tmp.mini_header)
		{
			case 0x40:		// short message
				memcpy(&msg->short_msg[0],  &msg->tmp.temp[0], 5);
				break;
			case 0x41:
				memcpy(&msg->short_msg[5],  &msg->tmp.temp[0], 5);
				break;
			case 0x42:
				memcpy(&msg->short_msg[10], &msg->tmp.temp[0], 5);
				break;
			case 0x43:
				memcpy(&msg->short_msg[15], &msg->tmp.temp[0], 5);
				status_short_message(msg->short_msg);
				break;

			case 0x31:		// slow data
			case 0x32:
			case 0x33:
			case 0x34:
			case 0x35:
				data_length = msg->tmp.mini_header & 0x0f;
//				if (data_length > 5) data_length = 5;
				memcpy(&msg->aprs_msg[msg->msg_pnt], &msg->tmp.temp[0], data_length);
				msg->msg_pnt += data_length;
				if (msg->aprs_msg[msg->msg_pnt - 1] == 0x0d)
					msg->aprs_msg[msg->msg_pnt++] = 0x0a;
				if (msg->aprs_msg[msg->msg_pnt - 1] == 0x0a)
				{
					msg->aprs_msg[msg->msg_pnt] = 0x00;
					if (msg->msg_pnt > 6)
					{
						if (!strncmp(&msg->aprs_msg[0], "$$CRC", 5))
						{
							gps_a(msg);
						}
						else if (!strncmp(&msg->aprs_msg[0], "$GP", 3))
						{
							gps(msg);
						}
						else if (msg->aprs_msg[0] != 0x0a)
						{
							gps(msg);
						}
					}
					msg->msg_pnt = 0;
				}
				if (msg->msg_pnt > 240) msg->msg_pnt = 240;
				break;
		}
	}
}

void	inet_short_message(struct inet_short_msg *msg, struct dv_packet *voice)
{
	int	data_length;
	if ((voice->dstar.b_bone.b_b.seq & 0x1f) == 0x00) return;
	if ((voice->dstar.b_bone.b_b.seq & 0x1f) % 2)
	{
		msg->mini_header = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[0] ^ 0x70;
		msg->temp[0] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[1] ^ 0x4f;
		msg->temp[1] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[2] ^ 0x93;
	}
	else
	{
		msg->temp[2] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[0] ^ 0x70;
		msg->temp[3] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[1] ^ 0x4f;
		msg->temp[4] = voice->dstar.b_bone.dstar_udp.voice_d.data_segment[2] ^ 0x93;
		switch (msg->mini_header)
		{
			case 0x40:	// short message
				memcpy(&msg->short_msg[0], &msg->temp[0], 5);
				break;
			case 0x41:
				memcpy(&msg->short_msg[5], &msg->temp[0], 5);
				break;
			case 0x42:
				memcpy(&msg->short_msg[10], &msg->temp[0], 5);
                                break;
                        case 0x43:
				memcpy(&msg->short_msg[15], &msg->temp[0], 5);
				status_short_message(msg->short_msg);
				break;
		}
	}
}

void	short_msg(struct echo *echo, char voice[])
{
	if ((voice[14] & 0x1f) % 2)
	{
		echo->mini_header = voice[24] ^ 0x70;
		echo->msg_tmp[0] =  voice[25] ^ 0x4f;
		echo->msg_tmp[1] =  voice[26] ^ 0x93;
	}
	else
	{
		echo->msg_tmp[2] =  voice[24] ^ 0x70;
		echo->msg_tmp[3] =  voice[25] ^ 0x4f;
		echo->msg_tmp[4] =  voice[26] ^ 0x93;
		switch (echo->mini_header)
		{
			case 0x40:	// short message
				memcpy(&echo->msg[0],  &echo->msg_tmp[0], 5);
				break;
			case 0x41:
				memcpy(&echo->msg[5],  &echo->msg_tmp[0], 5);
				break;
			case 0x42:
				memcpy(&echo->msg[10], &echo->msg_tmp[0], 5);
				break;
			case 0x43:
				memcpy(&echo->msg[15], &echo->msg_tmp[0], 5);
				break;
		}
	}
}

int	header_check(unsigned char string[])
{
	unsigned short int	crc_dstar_ffff, k;
	int	i;

	for (i = 3 ; i < 35 ; i++)
	{
		if ((i != 19) || (string[19] != '/'))
		{
			if (string[i] != 0x20)
			{
				if ((string[i] < '0') || (string[i] > '9'))
				{
					if ((string[i] < 'A') || (string[i] > 'Z')) return FALSE;
				}
			}
		}
	}

	crc_dstar_ffff = 0xffff;	/* nornal value 0xffff */

	for (i = 0 ; i < 39 ; i++)
	{
		crc_dstar_ffff = update_crc_dstar( crc_dstar_ffff, string[i]);
	}

	crc_dstar_ffff = result_crc_dstar(crc_dstar_ffff);

	k = string[39] << 8 | string[40];
	if (k != crc_dstar_ffff) return FALSE;

	return callsign_check(&string[27]);
}

int	getOwnIp(void)
{
	int	fd;
	struct	ifreq	ifr;
	int	ret;

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name, NicDevice, IFNAMSIZ);
	ret = ioctl(fd, SIOCGIFADDR, &ifr);
	close(fd);
	time(&cur_time);
	if (ret == -1)
	{
		fprintf(log_file, "%24.24s ioctl error: %s\n", ctime(&cur_time), strerror(errno));
		fflush(log_file);
		return FALSE;
	}
	if (debug)
	{
		fprintf(log_file, "%24.24s Own IP address %s\n", ctime(&cur_time),
        		inet_ntoa (((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		fflush(log_file);
	}
	OwnDvApIP.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
	return TRUE;
}

int	callsign_check(char CallSign[])		/* MyCallSign field Check */
{
	char	buf[80],str[20];
	char	*token;
	int	length, len, i, k;

	acc_file = fopen (ACC_FILE, "r");
	if (!acc_file)
	{
		if (debug >= 2)
		{
			time(&cur_time);
			fprintf(log_file, "%24.24s ACCESS CONTROL file not found\n", ctime(&cur_time));
			fflush(log_file);
		}
		return ALLOW;
	}

	memset(buf, 0x00, 80);
	if (fgets (buf, 80, acc_file) == NULL)
		len = 0;
	else
		len = strlen (buf);
	while (len)
	{
		if (debug > 2)
		{
			time(&cur_time);
			fprintf(log_file, "%24.24s Access Ctrl: %s\n", ctime(&cur_time), buf);
			fflush(log_file);
		}
		if (buf[0] != '#')
		{
			k = 0;
			memset(str, 0x20, 8);
			for  (i = 0 ; i < len ; i++)
			{
				if (buf[i] == 0x09) k = 8;
				else
					str[k++] = buf[i];
				if (k == 20) break;
			}
			if (str[k-1] == 0x0a)
			{
				str[k-1] = 0x00;
				k--;
			}
			token = str;
			for (i = 0 ; i < k ; i++)
			{
				*token = toupper (*token);
				token++;
			}
			length = 8;
			for (i = 0 ; i <= 7 ; i++)
			{
				if (str[7-i] == '*')
				{
					length = 7 - i;
					break;
				}		
			}
			if (length == 0) length = 8;

			if (!memcmp (str, "*", 1))
			{
				if (!memcmp(&str[8], "DENY", 4))
				{
					fclose (acc_file);
					return DENY;
				}
				if (!memcmp(&str[8], "ALLOW", 5))
				{
					fclose (acc_file);
					return ALLOW;
				}
				if (!memcmp(&str[8], "APRS", 4))
				{
					fclose (acc_file);
					return APRS;
				}
			}
			else if (!memcmp (str, CallSign, length))
			{ 
				if (!memcmp(&str[8], "DENY", 4))
				{
					fclose (acc_file);
					return DENY;
				}
				if (!memcmp(&str[8], "ALLOW", 5))
				{
					fclose (acc_file);
					return ALLOW;
				}
				if (!memcmp(&str[8], "APRS", 4))
				{
					fclose (acc_file);
					return APRS;
				}
			}
		}
		memset(buf, 0x00, 20);
		if (fgets (buf, 80, acc_file) == NULL)
			len = 0;
		else
			len = strlen (buf);
	}
	fclose (acc_file);
	return DENY;
}

void	echo_header_send_set(void)
{
	unsigned short int	tmp;

	dv_pkt_set(&echo_dv_pkt);
	echo_dv_pkt.dstar.b_bone.b_b.id = 0x20;		/* voice */
	tmp = rand() & 0xffff;
	memcpy (echo_dv_pkt.dstar.b_bone.b_b.frame_id, &tmp, 2);
	echo_dv_pkt.dstar.b_bone.b_b.seq = 0x80;
}

#define CRCPOLY2 0xEDB88320

unsigned int crc32(int n, unsigned char c[])
{
	int i, j;
	unsigned int r;

	r = 0xFFFFFFFFUL;
	for (i = 0; i < n; i++) {
		r ^= c[i];
		for (j = 0; j < 8 ; j++)
			if (r & 1) r = (r >> 1) ^ CRCPOLY2;
			else       r >>= 1;
	}
	return r ^ 0xFFFFFFFF;
}

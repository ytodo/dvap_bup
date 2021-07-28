#include	"dv_ap.h"

int	handler_init(void);
int     read_config(int argc, char **argv);
void	sig_init(void);
int	getOwnIp(void);
void	httpd_init(void);
void	send_inet_init(void);
unsigned int crc32(int n, unsigned char c[]);

extern  char    dvap_header[47];
extern  char    dvap_inet_header[41];
extern  char    node_inet_header[41];
extern	long int	timezone;
extern	int	daylight;

int	init(int argc, char **argv)
{
	int	err;
	struct	STATUS_Login_frame	Login_Frm;
	struct	status	*sta_pnt;
	int     fd;
        struct  ifreq   ifr;

	sig_init();
	uname (&uname_buf);
	tzset();
	node_NoRespReply_sw = FALSE;
	dvap_NoRespReply_sw = FALSE;
	IDxxPlus_NoRespReply_sw = FALSE;
	HeaderLength = 0;
	VenderID = 0x4d8;
	ProductID = 0x300;

	keep_alive_interval = 0;

	get_position_sw = FALSE;
	
	node_sw = TRUE;
	dvap_sw = TRUE;
	IDxxPlus_sw = TRUE;
	dvap_freq = 0;
	dvap_tx_freq = 0;
	dvap_rx_freq = 0;
	dvap_squelch = 0;
	dvap_calibration = 0;
	dvap_auto_calibration = FALSE;
	dvap_auto_calibration_set = FALSE;
	dvap_squelch_status = 0x00;

	aprs_port = 0;
	aprs_send_interval = 0;
	aprs_cnt = 0;
	aprs_beacon_cnt = 0;
	send_check_pnt = NULL;
	time (&BeaconTime);

	http_port = 0;

        tv.tv_sec = 0;
        tv.tv_nsec = 10000000;   /* 10 m sec. */
        FD_ZERO (&save_rfds);
	
	memset (node_area_rep_callsign, 0x20, 8);
	memset (dvap_area_rep_callsign, 0x20, 8);
	memset (IDxxPlus_area_rep_callsign, 0x20, 8);

	echo_position_send_interval = 0;
	echo_pnt = NULL;

/* make PID file */
	pid_file = fopen (PID_FILE, "r");
	if (pid_file)
	{
		time (&cur_time);
		printf ("%24.24s Already running dv_ap.\n", 
			ctime(&cur_time));
		fclose (pid_file);
		exit (0);
	}
	pid_file = fopen (PID_FILE, "w");
	if (pid_file == NULL)
	{
		time (&cur_time);
		printf ("%24.24s  %s\n", ctime(&cur_time), strerror(errno));
		exit (0);
	}
	fprintf (pid_file, "%d", getpid());
	fclose (pid_file);

/* LOG FILE open */
        log_file = fopen (LOG_FILE, "a");
        time (&cur_time);
        time (&start_time);
        fprintf (log_file, "\n%24.24s dv_ap start V%s (Compiled %s %s)\n", ctime(&cur_time), PACKAGE_VERSION, __DATE__, __TIME__);
        fflush(log_file);

	trust_port = TRUST_PORT;
	logd_port  = LOGD_PORT;
	upnp_sw = 1;
	gateway_port = GATEWAY_PORT;
        memset (NicDevice, 0x00, IFNAMSIZ);
        //strcpy (NicDevice, NicDeviceInitValue);

        fd = socket (AF_INET, SOCK_DGRAM, 0);
	ifr.ifr_ifindex = 2;
	ioctl(fd, SIOCGIFNAME, &ifr);
	close (fd);
	strncpy(NicDevice, ifr.ifr_name, IFNAMSIZ);

	if (!read_config(argc, (char **)argv)) return FALSE;
	time(&cur_time);
	fprintf(log_file, "%24.24s Interface name: %s\n",
		ctime(&cur_time), NicDevice);
	fflush (log_file);

	trust_timeout = TRUST_TIMEOUT;
	upnp_udp_sd = -1;
	upnp_http_sd = -1;

	memset (Gw_IP, 0x00, 4);
	send_inet_init();

	dv_status.port = 0;

	aprs_rf_send = FALSE;

	if (!handler_init()) return FALSE;
	memset (aprs_filter, 0x00, 256);
	if (debug)
	{
		time (&cur_time);
		if (node_area_rep_callsign[0] != 0x20)
		fprintf (log_file, "%24.24s node area repeater : %8.8s\n", 
			ctime(&cur_time), node_area_rep_callsign);
		if (dvap_area_rep_callsign[0] != 0x20) 
			fprintf (log_file, "%24.24s dvap area repeater : %8.8s\n",
			ctime(&cur_time), dvap_area_rep_callsign);
		fprintf (log_file, "%24.24s gateway call  : %8.8s\n", 
			ctime (&cur_time), gateway_callsign);
		fprintf (log_file, "%24.24s trust : %s\n",
			ctime (&cur_time), trust_name);
		fprintf (log_file, "%24.24s trust timeout %d msec.\n",
			ctime(&cur_time), trust_timeout);
		fflush (log_file);
	}
	
	voice_pnt = 0;
	seq = 0;
	node_last_frame_sw = FALSE;
	dvap_last_frame_sw = FALSE;
	IDxxPlus_last_frame_sw = FALSE;
	rep_position_send_sw = FALSE;
	sig_term = FALSE;
	qsy_info = TRUE;

	node_usb_init();
	dvap_open();

	time (&echo_position_send_time);

/* status */
        if (dv_status.port)
        {
                time (&cur_time);
                memset (&hints, 0x00, sizeof(hints));
                hints.ai_socktype = SOCK_DGRAM;
                hints.ai_family = PF_UNSPEC;
        	hints.ai_flags = AI_PASSIVE;
                sprintf (PORT, "%d", dv_status.port);
                if ((err = getaddrinfo (dv_status.fqdn, PORT, &hints, &dv_status.status_info)) != 0)
                {
                	fprintf (log_file, "%24.24s getaddrinfo error (Status:%s:%0d) %s\n",
                       		ctime(&cur_time), dv_status.fqdn, dv_status.port, gai_strerror(err));
			fflush (log_file);
                        dv_status.port = 0;
                        if (dv_status.status_info != NULL) freeaddrinfo (dv_status.status_info);
                }
             	else
                {
                	if ((dv_status.status_sd = socket (dv_status.status_info->ai_family,
                        	dv_status.status_info->ai_socktype, dv_status.status_info->ai_protocol)) < 0 )
                        {
                        	fprintf (log_file, "%24.24s STATUS UDP socket not open\n",ctime(&cur_time));
				fflush (log_file);
                                dv_status.port = 0;
                        }
                        else
                        {
                        	fprintf (log_file, "%24.24s Status Port %s:%0d open.\n",
                                	ctime(&cur_time), dv_status.fqdn, dv_status.port);
				fflush (log_file);
				FD_SET (dv_status.status_sd, &save_rfds);
                                memcpy (Login_Frm.StatusID, "DSTRST", 6);
                                memcpy (Login_Frm.Type, "00", 2);
                                #if __WORDSIZE == 64
                                time (&Login_Frm.EntryUpdateTime);
                                #else
                                time (&Login_Frm.EntryUpdateTime);
                                memset (&Login_Frm.dummy_t, 0x00, 4);
                                #endif
                                memcpy (Login_Frm.UserID, dv_status.userID, 16);
                                memset (Login_Frm.UserID, 0x20, 16);
                                memcpy (Login_Frm.UserID, client_callsign, 8);
                                memcpy (Login_Frm.Passwd, dv_status.passwd, 64);
                                memset (Login_Frm.reserve, 0x00, 4);
                                err = sendto (dv_status.status_sd, &Login_Frm, 100, 0,
                                	dv_status.status_info->ai_addr,
                                        dv_status.status_info->ai_addrlen);
                                dv_status.packets++;
                        }
           	}
        }

/* trust */
	memset (&hints, 0x00, sizeof(hints));
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_family = PF_UNSPEC;
	sprintf (PORT, "%d", trust_port);
	if ((err = getaddrinfo (trust_name, PORT, &hints, &trust_sock)) != 0)
	{
		time (&cur_time);
		fprintf (log_file, "%24.24s getaddrinfo error(trust sock) %s\n",
			ctime(&cur_time), gai_strerror(err));
		fprintf (log_file, "%24.24s trust : %s\n", ctime(&cur_time), trust_name);
		fflush (log_file);
		return FALSE;
	}
	trust_ip.s_addr = ((struct sockaddr_in *)(trust_sock->ai_addr))->sin_addr.s_addr;

/* gateway */		
        memset (&hints, 0x00, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = PF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;
        sprintf (PORT, "%d", gateway_port);
        if ((err = getaddrinfo (NULL, PORT, &hints, &gateway_in)) != 0)
        {
                time (&cur_time);
                fprintf (log_file, "%24.24s getaddrinfo error(gateway in) %s\n",
                        ctime(&cur_time), gai_strerror(err));
                fprintf (log_file, "%24.24s trust : %s\n", ctime(&cur_time), trust_name);
                fflush (log_file);
        }
        if((in_sd = socket(gateway_in->ai_family, gateway_in->ai_socktype, gateway_in->ai_protocol)) < 0)
	{
		fprintf (log_file, "%24.24s socket error in gateway in %s\n",
			ctime(&cur_time), strerror(errno));
		fflush (log_file);
                return FALSE;
        }
	FD_SET (in_sd, &save_rfds);

	time(&cur_time); 
        int yes_flag = 1;
        setsockopt(in_sd, SOL_SOCKET, SO_REUSEADDR,
		(void *)&yes_flag, sizeof(int));
	if(bind(in_sd, gateway_in->ai_addr, gateway_in->ai_addrlen) < 0) 
	{
		fprintf (log_file, "%24.24s bind error in gateway port %s\n",
			ctime(&cur_time), strerror(errno));
		fflush (log_file);
		return FALSE;
	}

	memset(buf_pkt.buf, 0, sizeof(buf_pkt.buf));
 
	memset (node_save_frame_id, 0x00, 2);
	memset (dvap_save_frame_id, 0x00, 2);
	memset (IDxxPlus_save_frame_id, 0x00, 2);
	NodeActiveTime.tv_sec = 0;
	NodeActiveTime.tv_usec = 0;
	DvapActiveTime.tv_sec = 0;
	DvapActiveTime.tv_usec = 0;
	IDxxPlusActiveTime.tv_sec = 0;
	IDxxPlusActiveTime.tv_usec = 0;
	aprs_SendTime.tv_sec = 0;
	aprs_SendTime.tv_usec = 0;
	aprs_ptt_onoff = FALSE;

	Rp = malloc (sizeof (struct FifoPkt));
	Wp = Rp;
	Rp->next = NULL;

	IDxxPlusRp = malloc (sizeof (struct FifoPkt));
	IDxxPlusWp = IDxxPlusRp;
	IDxxPlusRp->next = NULL;
	IDxxPlusFifo_cnt = 0;

	aprs_Rp = malloc (sizeof (struct aprsFifoPkt));
	aprs_Wp = aprs_Rp;
	aprs_Rp->next = NULL;

	init_sw = FALSE;
	node_voice_send_sw = FALSE;
	dvap_voice_send_sw = FALSE;
	IDxxPlus_voice_send_sw = FALSE;
	IDxxPlus_send_sw = FALSE;

	/* upnp upd */
	if (upnp_sw)
	{
		if (!getOwnIp()) return FALSE;;
        	memset (&hints, 0x00, sizeof(hints));
        	hints.ai_socktype = SOCK_DGRAM;
        	hints.ai_family = PF_UNSPEC;
                if ((err = getaddrinfo (inet_ntoa (OwnDvApIP), NULL, &hints, &upnp_bind_sock)) != 0)
                {
                        time (&cur_time);
                        fprintf (log_file, "%24.24s getaddrinfo error(upnp bind sock) %s\n",
                                ctime(&cur_time), gai_strerror(err));
                        fprintf (log_file, "%24.24s upnp : %s\n", ctime(&cur_time), inet_ntoa (OwnDvApIP));
                        fflush (log_file);
                }
                if((upnp_udp_sd = socket(upnp_bind_sock->ai_family, upnp_bind_sock->ai_socktype, upnp_bind_sock->ai_protocol)) < 0)
                {
                        fprintf (log_file, "%24.24s socket error in upnp bind sock %s\n",
                                ctime(&cur_time), strerror(errno));
                        fflush (log_file);
                        return FALSE;
                }

		if (bind (upnp_udp_sd, upnp_bind_sock->ai_addr, upnp_bind_sock->ai_addrlen) == -1)
		{
			fprintf (log_file, "%24.24s socket error in upnp bind %s\n",
				ctime(&cur_time), strerror(errno));
			fflush (log_file);
			return FALSE;
		}

		memset (&hints, 0x00, sizeof(hints));
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_family = PF_UNSPEC;
		
        	if ((err = getaddrinfo ("239.255.255.250", "1900", &hints, &upnp_sock)) != 0)
        	{
                	time (&cur_time);
                	fprintf (log_file, "%24.24s getaddrinfo error(upnp sock) %s\n",
                        	ctime(&cur_time), gai_strerror(err));
                	fprintf (log_file, "%24.24s upnp : 239.255.255.250\n", ctime(&cur_time));
                	fflush (log_file);
        	}
        	FD_SET (upnp_udp_sd, &save_rfds);
	}

        // httpd
        if (http_port > 0) httpd_init();

	time (&re_check);

/* APRS */
	node_msg.msg_pnt = 0;
	dvap_msg.msg_pnt = 0;

	node_recv_time = 0;
	node_inet_recv_time = 0;
	dvap_recv_time = 0;
	dvap_inet_recv_time = 0;

	node_rig_pkt_total_cnt = 0;
	dvap_rig_pkt_total_cnt = 0;
	IDxxPlus_rig_pkt_total_cnt = 0;
	node_inet_pkt_total_cnt = 0;
	dvap_inet_pkt_total_cnt = 0;
	IDxxPlus_inet_pkt_total_cnt = 0;

	/* HTTP */
	memset (&node_save_hdr, 0x20, sizeof(node_save_hdr));
	memset (&node_inet_header, 0x20, sizeof(node_inet_header));
	memset (&dvap_header, 0x20, sizeof(dvap_header));
	memset (&dvap_inet_header, 0x20, sizeof(dvap_inet_header));

        srand (crc32(8, (unsigned char *)gateway_callsign));
        status_start = rand() % 300;

/* IDxxPlus */
	IDxxPlus_buff_pnt = 0;

	return TRUE;
}


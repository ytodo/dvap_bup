#include	"dv_ap.h"

int     init(int argc, char **argv);
void	node_NoRespReply(struct dv_header NoResp);
void    dvap_NoRespReply(struct dv_header NoResp);
int	from_inet(void);
void	send_pkt(void);
void	node_send_wait_set(void);
void	dvap_send_wait_set(void);
void	IDxxPlus_send_wait_set(void);
void	send_keep_alive(void);
void	dvap(void);
void	node_send_response(void);
void	dvap_send_response(void);
void	node_last_frame_send(void);
void	dvap_read(void);
int	aprs(void);
void	upnp (void);
void	upnp_portmap_del (void);
void	upnp_send_http(void);
void	ReqPositionUpd (void);
void	send_inet (void);
void	read_upnp_udp (void);
void	read_upnp_http (void);
void	httpd_srv_accept (void);
void	httpd_srv_recv (int i);
int     echo_server_send(struct echo *echo);
void    echo_server_last_ex (void);
void    echo_jitter_flush_ex(void);
void    send_echo (struct echo *echo);
void	send_echo_position (void);
void    status_keep_alive_send(void);
void	read_status_udp (void);
void	aprs_msg_send (void);
void	IDxxPlus(void);
void	IDxxPlus_read(void);
void	send_IDxxPlus(void);

extern	char	lastframe[];

int main(int argc, char **argv)
{
	int	ret;
	struct	timeval	tmp_tm;
	struct	timeval	one_sec;
	struct	timeval	s100m;
	struct	timeval	s500m;
	struct	timeval	tm1;
	struct	timeval c_time;
	struct  timeval rig_send_time_20mSec;
	int	i;
	struct	echo	*echo_next;

	one_sec.tv_sec = 1;
	one_sec.tv_usec = 0;
	s100m.tv_sec = 0;
	s100m.tv_usec = 100000;
	s500m.tv_sec = 0;
	s500m.tv_usec = 500000;
        rig_send_time_20mSec.tv_sec = 0;
        rig_send_time_20mSec.tv_usec = 20000;

	if (init(argc, (char **)argv))
	{
		ReqPositionUpd ();
		while (1)
		{
			memcpy (&rfds, &save_rfds, sizeof(save_rfds));
			memcpy (&sigset, &save_sigset, sizeof(save_sigset));
			ret = pselect (FD_SETSIZE, &rfds, (fd_set *)NULL, 
					(fd_set *)NULL, &tv, &sigset);
			time(&cur_time);
			if (ret < 0)
			{
				fprintf (log_file, "%24.24s pselect error %s\n",
					ctime(&cur_time), strerror(errno));
				fflush (log_file);
				break;
			}
			if (ret > 0)
			{
				if (FD_ISSET (in_sd, &rfds))
				{
					if (!from_inet()) break;
				}
				if (dvap_sw && FD_ISSET (dvap_fd, &rfds))
				{
					dvap_read();
				}
				if (IDxxPlus_sw && FD_ISSET (IDxxPlus_fd, &rfds))
				{
					IDxxPlus_read();
				}
				if (upnp_sw)
				{
					if (FD_ISSET (upnp_udp_sd, &rfds))
					{
						read_upnp_udp();
					}
					if ((upnp_http_sd > 0) &&FD_ISSET (upnp_http_sd, &rfds))
					{
						read_upnp_http();
					}
				}
				if (dv_status.port)
				{
					if (FD_ISSET (dv_status.status_sd, &rfds))
					{
						read_status_udp();
					}
				}
				if (http_port)
				{
					if (FD_ISSET (httpd_sd, &rfds))
					{
						httpd_srv_accept();
					}
        	                       	for (i = 1 ; i < FD_SETSIZE ; i++)
                	               	{
						if ((i == in_sd)
							|| (i == httpd_sd)
							|| (i == upnp_http_sd)
							|| (i == upnp_udp_sd)
							|| (i == dvap_fd	)
							|| (i == logd_sd)
							|| (i == dv_status.status_sd)
							|| (i == aprs_sd)) continue;
                                              	else if (FD_ISSET (i, &rfds)) httpd_srv_recv(i);
                        	       	}
                       		}
			}

               		/* read from rig (DV packket) */
			if (node_sw)
			{
				if (cos_check()) gettimeofday(&NodeCosOffTime, NULL);
               			if (status & (HeaderDecodeDone | COS_OnOff)) 
               			{
                       			if (HeaderLength == 0) 
					{
						header_read_from_rig ();
					}
                  	     		else voice_read_from_rig();
               			}
              	 		else	/* last frame check */
               			{
                       			if (HeaderLength)
                       			{
                               			if (voice_read_from_rig() == 0)
						{
							if (node_last_frame_sw) node_last_frame_send();
							HeaderLength = 0;
							voice_pnt = 0;
							if (node_gw_resp_sw)
							{
								node_send_wait_set();
							}	
						}
                       			}
               			}
				if (node_last_frame_sw)
				{
					gettimeofday (&tm1, NULL);
					timersub (&tm1, &Node_InTime, &tmp_tm);
					if (timercmp(&tmp_tm, &s100m,  >)) node_last_frame_send();
				}
				node_send_response();
			}
			if (dvap_sw)
			{
				dvap();
				if ((cur_time - dvap_keep_alive) >= 2)
						send_keep_alive(); 
				dvap_send_response();
			}
			if (IDxxPlus_sw)
			{
				if ((IDxxPlusFifo_cnt > 20) && (!IDxxPlus_send_sw)) 
				{
					IDxxPlus_send_sw = TRUE;
					gettimeofday (&IDxxPlus_send_time, NULL);
				}
				IDxxPlus();
			}
			if (IDxxPlus_send_sw)
			{
				gettimeofday (&c_time, NULL);
				timeradd (&IDxxPlus_send_time, &rig_send_time_20mSec, &tmp_tm);
				if (timercmp (&c_time, &tmp_tm, >))
				{
					send_IDxxPlus();
					IDxxPlus_send_time.tv_sec = tmp_tm.tv_sec;
					IDxxPlus_send_time.tv_usec = tmp_tm.tv_usec;
				}
			}
			/* device re check */
			if (!(node_sw && dvap_sw))
			{
				if ((cur_time - re_check) > RECHECK)
				{
					if (!node_sw && (node_area_rep_callsign[0] != 0x20)) node_usb_init();
					if (!dvap_sw && (dvap_area_rep_callsign[0] != 0x20)) dvap_open();
					re_check = cur_time;
				}
			}
	
			gettimeofday (&tm1, NULL);
		
			if (node_NoRespReply_sw)
			{
				timersub (&tm1, &NodeActiveTime, &tmp_tm);
				if (timercmp(&tmp_tm, &s500m,  >)) memset (node_save_frame_id, 0x00, 2);
				timersub (&tm1, &NodeCosOffTime, &tmp_tm);
				if (timercmp (&tmp_tm, &one_sec, >))
				{
					node_send_wait_set();
				}
			}
			if (dvap_NoRespReply_sw)
			{
				timersub (&tm1, &DvapActiveTime, &tmp_tm);
				if (timercmp(&tmp_tm, &s500m,  >)) memset (dvap_save_frame_id, 0x00, 2);
				timersub (&tm1, &DvapCosOffTime, &tmp_tm);
				if (timercmp (&tmp_tm, &one_sec, >))
				{
					dvap_send_wait_set();
				}
			}
			if (IDxxPlus_NoRespReply_sw)
			{
				timersub (&tm1, &IDxxPlusActiveTime, &tmp_tm);
				if (timercmp(&tmp_tm, &s500m,  >)) memset (IDxxPlus_save_frame_id, 0x00, 2);
				timersub (&tm1, &IDxxPlusCosOffTime, &tmp_tm);
				if (timercmp (&tmp_tm, &one_sec, >))
				{
					IDxxPlus_send_wait_set();
				}
			}
			send_inet();
			if (dvap_sw)
			{	
				if ((cur_time - dvap_in_time.tv_sec) > 3) dvap_close();
			}
	
			if (keep_alive_interval)
			{
				if ((cur_time - keep_alive) >= keep_alive_interval)
				{
					//GatewayIpUpdate();
					ReqPositionUpd ();
				}
			}
			if (upnp_sw) upnp();
			if (aprs_port) 
			{
				if (!aprs())
				{
					fprintf (log_file, "%24.24s aprs error\n", ctime(&cur_time));
					fflush (log_file);
					aprs_port = 0;
					FD_CLR (upnp_http_sd, &save_rfds);
				}
			}
                        echo_next = echo_pnt;
                        while (echo_next)
                        {
                                if (echo_server_send(echo_next)) echo_next = echo_pnt;
                                else    echo_next = echo_next->next;
                        }

			echo_jitter_flush_ex();
                        if (echo_position_send_interval)
                        {
                                time (&cur_time);
                                if (cur_time >= echo_position_send_time)
                                        send_echo_position();
                        }

                	if ((dv_status.port) && ((cur_time - status_keep_alive) >= status_start))
                	{
                        	status_keep_alive_send();
                                status_start = 300;
                	}

			if (aprs_rf_send) aprs_msg_send ();
			
			if (sig_term) break;
		}
	}

	if (node_sw)
	{
		usb_reset (udev); 
		usb_close (udev);
	}
	if (dvap_sw)
	{
		dvap_close();
	}
	close(in_sd);
	time(&cur_time);
	remove (PID_FILE);
	freeaddrinfo (gateway_in);
	freeaddrinfo (gateway_out);
	if (upnp_sw) upnp_portmap_del();
	fprintf (log_file, "%24.24s dv_ap end\n", ctime(&cur_time));
	fflush (log_file)+
	fclose (log_file);

	return 0;
}


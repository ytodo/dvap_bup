#include	"dv_ap.h"

void	putFifo (int len, struct dv_packet pkt);
int	getFifo (int *len, struct dv_packet *pkt);
int	GatewayIpUpdate (void);
void	dv_log_send (struct dv_packet *pkt);
void    ReqAreaPositionInfo (char call[]);
void    ReqPositionInfo (char call[]);
int	ReqPositionUpd (void);

int     init(int argc, char **argv);
extern	char	lastframe[];
int	buff_length;
struct	dv_packet buff_pkt;

enum
{
	SEND_REQ_GATEWAY = 0,
	SEND_REQ_GATEWAY_WAIT,
	SEND_VOICE,
	SEND_INIT
} dv_packet = SEND_REQ_GATEWAY; 
	
void	send_inet(void)
{
	int	ret;
	char	temp[16];
	int	err;
	struct	timeval temp_time, c_time;
	struct	timeval timeout;

	switch (dv_packet)
	{
		case SEND_REQ_GATEWAY:
			if (getFifo (&buff_length, &buff_pkt))
			{
				if (buff_length == 56)
				{
					memset (temp, 0x20, 8);
					if (buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall[0] == '/')
					{
						memcpy (temp, &buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall[1], 6);
						temp[7] = buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall[7];
						ReqAreaPositionInfo (temp);
						memcpy (buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall, "CQCQCQ  ", 8);
					}
					else
					{
						ReqPositionInfo (buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall);
					}
					dv_packet = SEND_REQ_GATEWAY_WAIT;
					get_position_sw = FALSE;
					gettimeofday (&req_time, NULL);
				}	
			}
			break;

		case SEND_REQ_GATEWAY_WAIT:
			if (get_position_sw)
			{
				if (debug < 3) dv_log_send (&buff_pkt);
				
				memcpy (buff_pkt.dstar.b_bone.dstar_udp.rf_header.RPT2Call,
					&gateway_position[24], 8);
				memcpy (buff_pkt.dstar.b_bone.dstar_udp.rf_header.RPT1Call,
					&gateway_position[16], 8);
				buff_pkt.dstar.b_bone.dstar_udp.rf_header.RPT1Call[7] = 'G';
				sprintf (rep_ip_addr, "%d.%d.%d.%d",
						gateway_position[32],
						gateway_position[33],
						gateway_position[34],
						gateway_position[35]);
				gateway_port = gateway_position[40] << 8 | gateway_position[41];
				if (debug)
				{
					time (&cur_time);
					fprintf (log_file, "%24.24s Dest. IP address: %s  Port: %d\n", ctime (&cur_time), rep_ip_addr, gateway_port);
					fflush (log_file);
				}
				if (!memcmp (&gateway_position[32], &Gw_IP, 4))
				{
					time (&cur_time);
					fprintf (log_file, "%24.24s Dest. IP address changed to %s\n",
						ctime(&cur_time),
						inet_ntoa (OwnDvApIP));
					fflush (log_file);
					sprintf (rep_ip_addr, "%s", inet_ntoa (OwnDvApIP));
					
				}
				memset (&hints, 0x00, sizeof(hints));
				hints.ai_socktype = SOCK_DGRAM;
				hints.ai_family = PF_UNSPEC;
				sprintf (PORT, "%d", gateway_port);

        			if ((err = getaddrinfo (rep_ip_addr, PORT, &hints, &gateway_out)) != 0)
        			{
                			fprintf (log_file, "%24.24s getaddrinfo error(gateway out) %s\n",
                       				ctime(&cur_time), gai_strerror(err));
               	 			fprintf (log_file, "%24.24s trust : %s\n", ctime(&cur_time), rep_ip_addr);
                			fflush (log_file);
					return;
        			}
				sendto (in_sd, &buff_pkt, buff_length, 0,
					gateway_out->ai_addr, gateway_out->ai_addrlen);
				sendto (in_sd, &buff_pkt, buff_length, 0,
					gateway_out->ai_addr, gateway_out->ai_addrlen);
				dv_packet = SEND_VOICE;
			}
			else
			{
				gettimeofday (&c_time, NULL);
				timersub (&c_time, &req_time, &temp_time);
				timeout.tv_sec = 0;
				timeout.tv_usec = trust_timeout * 1000;
				if (timercmp (&temp_time, &timeout, >))
				{
					time (&cur_time);
					dv_packet = SEND_REQ_GATEWAY;
					if (buff_pkt.dstar.b_bone.b_b.send_terminal_id == 0x03) node_gw_resp_sw = TRUE;
					else if (buff_pkt.dstar.b_bone.b_b.send_terminal_id == 0x04) dvap_gw_resp_sw = TRUE;
					fprintf (log_file, "%24.24s No response from Trust server %8.8s\n", 
						ctime (&cur_time), 
						buff_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall);
					fflush (log_file);
				}
			}
			break;

		case SEND_VOICE:
			if (getFifo (&buff_length, &buff_pkt))
			{
				buff_pkt.dstar.b_bone.b_b.seq &= 0x1f;
			
				if (buff_length == 27)
				{	
					sendto (in_sd, &buff_pkt, 27, 0,
						gateway_out->ai_addr, gateway_out->ai_addrlen);
				}
				else if (buff_length == 30)
				{
					memcpy (&buff_pkt.dstar.b_bone.dstar_udp.voice_d.voice_segment,
						&lastframe[3], 3);
					if (buff_pkt.dstar.b_bone.b_b.seq > 20)
					{
						buff_pkt.dstar.b_bone.b_b.seq = 0;
					}
					buff_pkt.dstar.b_bone.b_b.seq |= 0x40;
					sendto (in_sd, &buff_pkt, 27, 0,
						gateway_out->ai_addr, gateway_out->ai_addrlen);
				
					dv_packet = SEND_REQ_GATEWAY;
					freeaddrinfo (gateway_out);
				}
				
			}
			break;

		case SEND_INIT:
			if (ReqPositionUpd ())
			{
				dv_packet = SEND_REQ_GATEWAY;
				time(&keep_alive);
			}
                        break;
	}
}

void	send_inet_init(void)
{
	dv_packet = SEND_INIT;
}

int	check_get_send_req_gateway_wait(void)
{
	if (dv_packet == SEND_REQ_GATEWAY_WAIT) return TRUE;
	return FALSE;
}


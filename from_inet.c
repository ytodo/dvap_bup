#include	"dv_ap.h"

int     init(int argc, char **argv);
void	reply_busy(struct dv_header dv_hdr);
void	NoRespReply(struct dv_header NoResp);
void	send_reply(char buf[]);
void    inet_short_message(struct inet_short_msg *msg, struct dv_packet *voice);
void    echo_server_header(char buf[]);
int     echo_server_voice(char buf[]);
int     echo_server_last(char buf[]);
void	read_trust(char buff[], int len);
void	IDxxPlus_last_send (char str[]);
void	IDxxPlus_putFifo (int len, char str[]);

extern	char	lastframe[];
char	dvap_inet_header[41];
char	node_inet_header[41];

int	from_inet (void)
{
	int	len;

	struct	dv_header	dv_hdr;
	char	IDxxPlus_send_buff[64];

	in_addr_len = sizeof(struct sockaddr_storage);

	if((len = recvfrom(in_sd, &buf_pkt, sizeof(buf_pkt), 0, 
		(struct sockaddr *)&in_addr, &in_addr_len)) < 0) 
	{
		time(&cur_time);
		fprintf (log_file, "%24.24s recvfrom error %s\n", 
			ctime(&cur_time), strerror(errno));
		fflush (log_file);
       		return FALSE;
	}

	if (memcmp (buf_pkt.buf, "DSVT", 4))
	{
		read_trust((char *)&buf_pkt, len);
		return TRUE;
	}

	if (len == 56)
	{
		memcpy (&dv_hdr, &buf_pkt.buf[15], 41); /* rf header save */
		if (buf_pkt.buf[11] == 0xff) /* response ? */
		{
			if (!memcmp (&buf_pkt.buf[18], node_area_rep_callsign, 8) && node_sw)
			{
				memcpy (&node_gw_resp, &buf_pkt.buf[15], 41);
				node_gw_resp_sw = TRUE;
				node_NoRespReply_sw = FALSE;
			}
			else if (!memcmp (&buf_pkt.buf[18], dvap_area_rep_callsign, 8) && dvap_sw)
			{
				memcpy (&dvap_gw_resp, &buf_pkt.buf[15], 41);
				dvap_gw_resp_sw = TRUE;
				dvap_NoRespReply_sw = FALSE;
			}
			else if (!memcmp (&buf_pkt.buf[18], IDxxPlus_area_rep_callsign, 8) && IDxxPlus_sw)
			{
				memcpy (&IDxxPlus_gw_resp, &buf_pkt.buf[15], 41);
				IDxxPlus_gw_resp_sw = TRUE;
				IDxxPlus_NoRespReply_sw = FALSE;
			}
		}
		else
		{
                        if (!memcmp (&buf_pkt.buf[18], echo_area_rep_callsign, 8)
                        	&& !memcmp(&buf_pkt.buf[34], echo_server, 8))
                        {
                                if (buf_pkt.buf[15] == 0x00)
                        	        echo_server_header (buf_pkt.buf);
                        }
			else if (!memcmp (&buf_pkt.buf[18], node_area_rep_callsign, 8))
			{

				if ((node_save_frame_id[0] == 0x00) && (node_save_frame_id[1] == 0x00))
				/* frame ID check */
				{
					node_gw_resp_sw = FALSE;
					gettimeofday(&NodeActiveTime, NULL);
					memcpy (node_save_frame_id, &buf_pkt.buf[12], 2);
					if (buf_pkt.buf[33] == 'G')
					{
						memcpy (dv_hdr.RPT2Call, &buf_pkt.buf[26], 8);
						memcpy (dv_hdr.RPT1Call, &buf_pkt.buf[18], 8);
					}
					header_send(dv_hdr);
					node_NoRespReply_sw = FALSE;
				}
				else if (memcmp (node_save_frame_id, &buf_pkt.buf[12], 2))
				{
					reply_busy (dv_hdr);
				}
				time (&node_inet_recv_time);
				memcpy (&node_inet_header, &buf_pkt.buf[15], 41);
				node_inet_pkt_cnt = 0;
				memset (&node_inet_msg.short_msg, 0x20, 20);
			}
			else if (!memcmp (&buf_pkt.buf[18], dvap_area_rep_callsign, 8))
			{
				if ((dvap_save_frame_id[0] == 0x00) && (dvap_save_frame_id[1] == 0x00))
				/* frame ID check */
				{
					dvap_gw_resp_sw = FALSE;
					gettimeofday(&DvapActiveTime, NULL);
					memcpy (dvap_save_frame_id, &buf_pkt.buf[12], 2);
					if (buf_pkt.buf[33] == 'G')
					{
						memcpy (dv_hdr.RPT2Call, &buf_pkt.buf[26], 8);
						memcpy (dv_hdr.RPT1Call, &buf_pkt.buf[18], 8);
					}
					header_send(dv_hdr);
					dvap_NoRespReply_sw = FALSE;
				}
				else if (memcmp (dvap_save_frame_id, &buf_pkt.buf[12], 2))
				{
					reply_busy (dv_hdr);
				}
				time (&dvap_inet_recv_time);
				memcpy (&dvap_inet_header, &buf_pkt.buf[15], 41);
				dvap_inet_pkt_cnt = 0;
				memset (&dvap_inet_msg.short_msg, 0x20, 20);
			}
                        else if (!memcmp (&buf_pkt.buf[18], IDxxPlus_area_rep_callsign, 8))
                        {
                                if ((IDxxPlus_save_frame_id[0] == 0x00) && (IDxxPlus_save_frame_id[1] == 0x00))
                                /* frame ID check */
                                {
                                        IDxxPlus_gw_resp_sw = FALSE;
                                        gettimeofday(&IDxxPlusActiveTime, NULL);
                                        memcpy (IDxxPlus_save_frame_id, &buf_pkt.buf[12], 2);
                                        if (buf_pkt.buf[33] == 'G')
                                        {
                                                memcpy (dv_hdr.RPT2Call, &buf_pkt.buf[26], 8);
                                                memcpy (&buf_pkt.buf[26], &buf_pkt.buf[18], 8);
						memcpy (&buf_pkt.buf[18], dv_hdr.RPT2Call, 8);
                                        }
					IDxxPlus_NoRespReply_sw = FALSE;
					IDxxPlus_send_buff[0] = 0x29;
					IDxxPlus_send_buff[1] = 0x20;
                                       	memcpy (&IDxxPlus_send_buff[2], &buf_pkt.buf[15], 39);
					IDxxPlus_send_buff[41] = 0xff;
					IDxxPlus_putFifo (42, IDxxPlus_send_buff);
                                        IDxxPlus_NoRespReply_sw = FALSE;
                                }
                                else if (memcmp (IDxxPlus_save_frame_id, &buf_pkt.buf[12], 2))
                                {
                                        reply_busy (dv_hdr);
                                }
                                time (&dvap_inet_recv_time);
                                memcpy (&dvap_inet_header, &buf_pkt.buf[15], 41);
                                dvap_inet_pkt_cnt = 0;
                                memset (&dvap_inet_msg.short_msg, 0x20, 20);
                        }
		}
	}
	else if (len == 27)
	{
		if (!memcmp (node_save_frame_id, &buf_pkt.buf[12], 2))
		{
			gettimeofday(&NodeActiveTime, NULL);
			node_inet_pkt_cnt++;
			node_inet_pkt_total_cnt++;
			if (buf_pkt.buf[14] & 0x40)
			{
				memcpy (&buf_pkt.buf[24], lastframe, 6);
				node_last_send (&buf_pkt.buf[15]);
				memset (node_save_frame_id, 0x00, 2);
			}
			else  node_voice_send (&buf_pkt.buf[15]);
			inet_short_message (&node_inet_msg, (struct dv_packet *)&buf_pkt);
		}
                else if (!memcmp (dvap_save_frame_id, &buf_pkt.buf[12], 2))
                {
       			gettimeofday(&DvapActiveTime, NULL);
                        dvap_inet_pkt_cnt++;
			dvap_inet_pkt_total_cnt++;
                        if (buf_pkt.buf[14] & 0x40)
                        {
                        	memcpy (&buf_pkt.buf[24], lastframe, 6);
                                dvap_last_send (&buf_pkt.buf[15]);
                                memset (dvap_save_frame_id, 0x00, 2);
                        }
                        else  dvap_voice_send (&buf_pkt.buf[15]);
			inet_short_message (&dvap_inet_msg, (struct dv_packet *)&buf_pkt);
       		}
		else if (!memcmp (IDxxPlus_save_frame_id, &buf_pkt.buf[12], 2))
		{
			gettimeofday(&IDxxPlusActiveTime, NULL);
			IDxxPlus_inet_pkt_cnt++;
			IDxxPlus_inet_pkt_total_cnt++;
			IDxxPlus_putFifo (13, &buf_pkt.buf[14]);
			if (buf_pkt.buf[14] & 0x40)
			{
				memset	(IDxxPlus_save_frame_id, 0x00, 2);
				IDxxPlus_send_sw = TRUE;
			}
			inet_short_message (&IDxxPlus_inet_msg, (struct dv_packet *)&buf_pkt);
		}
                else
                {
			echo_server_voice (buf_pkt.buf);
		}
	}
	else
	{
		time (&cur_time);
		fprintf (log_file, "%24.24s length:%d from inet\n", ctime(&cur_time), len);
		fflush (log_file);
	}
	return TRUE;
}


#include	"dv_ap.h"

void	dv_pkt_set(struct dv_packet *hdr);
void	putFifo (int len, struct dv_packet pkt);
int	header_check (unsigned char string[]);
void	ReqPositionUpdate (struct dv_packet *pkt);
void    short_message(struct aprs_msg *msg, struct dv_packet *voice);
void    status_send_ptton (void);
void    status_send_pttoff (void);
void	status_send_update (struct dv_packet *pkt);

char    lastframe[6] ={0x55, 0x55, 0x55, 0x55, 0xc8, 0x7a};

void    header_read_from_rig(void)         /* read from rig (DV packet) */
{
	int	ret;
	int	header_check_rtn;

        /***  from RIG  ***/

	memset (usb.buffer, 0x20, 41);
        ret = usb_control_msg(udev, 0xC0, GET_HEADER, 0, 0, 
		usb.buffer, 32, 200);
        if (ret > 0)
        {
		gettimeofday(&Node_InTime, NULL);
		if (usb_control_msg(udev, 0xC0, GET_HEADER, 
			0, 0, &usb.buffer[32], 9, 200) < 0)
		{
			
			return;
		}
		if (status & CRC_ERROR)
		{
			time(&cur_time);
			fprintf (log_file, "%24.24s CRC ERROR of RF header\n", ctime(&cur_time));
			fflush (log_file);
			return;
		}
		if (memcmp (usb.hdr.RPT1Call, node_area_rep_callsign, 8)) 
		{
			if (debug)
			{
				time (&cur_time);
				fprintf (log_file, "%24.24s RPT1 Call is not same as repeater call %8.8s:%8.8s\n",
					ctime(&cur_time), usb.hdr.RPT1Call, node_area_rep_callsign);
				fflush (log_file);
			}
			return;
		}
		if (!(usb.hdr.flags[0] & 0x40))
		{
			/* not dup set */
			//gw_resp_sw = TRUE;
			memcpy (node_gw_resp.MyCall, node_area_rep_callsign, 8);
			memcpy (node_gw_resp.YourCall, usb.hdr.MyCall, 8);
			memcpy (node_gw_resp.RPT1Call, node_area_rep_callsign, 8);
			memcpy (node_gw_resp.RPT2Call, node_area_rep_callsign, 8);
			node_gw_resp.flags[0] = 0x01;
			node_gw_resp.flags[1] = 0x00;
			node_gw_resp.flags[2] = 0x00;
			if (debug)
			{
				time (&cur_time);
				fprintf (log_file, "%24.24s dup flag not set.\n", ctime(&cur_time));
				fflush (log_file);
			}
			return;
		}
		else
		{
			if (!memcmp (usb.hdr.MyCall, "        ", 8)
			|| !memcmp (usb.hdr.YourCall, "        ", 8))
			{
				/* My call or your call is blank */
				//gw_resp_sw = TRUE;
				memset (node_gw_resp.RPT2Call, 0x20, 36);
				memcpy (node_gw_resp.MyCall, node_area_rep_callsign, 8);
				node_gw_resp.flags[0] = 0x02;
				node_gw_resp.flags[1] = 0x00;
				node_gw_resp.flags[2] = 0x00;
				if (debug)
				{
					time (&cur_time);
					fprintf (log_file, "%24.24s MyCall or Your Call is blank\n", ctime(&cur_time));
					fflush (log_file);
				}
				return;
			}
		}

		HeaderLength = 41;
		if ((header_check_rtn = header_check(usb.buffer)) == ALLOW)
		{
			memcpy (&node_save_hdr, &usb.hdr, 41);
			time (&node_recv_time);

			node_voice_send_sw = TRUE;
			memcpy (&node_pkt.dstar.b_bone.dstar_udp.rf_header, usb.buffer, 41);
			node_pkt.dstar.b_bone.dstar_udp.rf_header.flags[0] &= 0xbf;	/* clear repeater flag */

			header_send_set_from_rig();
                        node_msg.msg_pnt = 0;
                        memset (node_msg.short_msg, 0x20, 20);
		}
		else if (header_check_rtn == APRS)
			node_voice_send_sw = FALSE;
	}
	else if (ret < 0)
	{
		time (&cur_time);
		fprintf (log_file, "%24.24s Node Adapter Down\n", ctime(&cur_time));
		fflush (log_file);
		node_close();
	}
}

void	header_send_set_from_rig(void)
{
	unsigned short int	tmp;

	dv_pkt_set(&node_pkt);
	node_pkt.pkt_type = 0x10;
	node_pkt.dstar.b_bone.b_b.id = 0x20;	/* voice */
	tmp = rand() & 0xffff;
	memcpy (node_pkt.dstar.b_bone.b_b.frame_id, &tmp, 2);
	node_pkt.dstar.b_bone.b_b.seq = 0x80;
	node_msg.msg_pnt = 0;

	ReqPositionUpdate (&node_pkt);
	if (dv_status.port) status_send_ptton();

	if (memcmp (usb.hdr.RPT2Call, node_area_rep_callsign, 8))
	{
		putFifo (56, node_pkt);
		node_pkt.dstar.b_bone.b_b.seq = 0xa0;
		memcpy (&node_pkt_header, &node_pkt, 56);
		node_voice_send_sw = TRUE;
		node_msg.AprsSend = 0x00;
	}
	else
	{
		node_voice_send_sw = FALSE;
	}

	node_NoRespReply_sw = FALSE;
	memcpy (&node_NoResp, usb.buffer, 41);
	seq = 0;
	node_last_frame_sw = TRUE;
}

int	voice_read_from_rig()
{
	int	i;
	int	ret;

	int	read_len;

	read_len = 24 - voice_pnt;

	ret = usb_control_msg(udev, 0xC0, GET_DATA, 0, 0, usb.buffer, read_len, 200);
	if (ret > 0)
	{
		gettimeofday(&Node_InTime, NULL);
		usb_read_cnt = 0;
		for (i = 0 ; i < ret; i++)
		{
			voice_save[voice_pnt] = usb.buffer[i];
			voice_pnt++;
			if (voice_pnt == 24)
			{
				voice_pnt = 12;
				memcpy (&node_pkt.dstar.b_bone.dstar_udp.voice_d, voice_save, 12);
				memcpy (voice_save, &voice_save[12], 12);
				dv_pkt_set(&node_pkt);
				node_pkt.pkt_type = 0x20;
				node_pkt.dstar.b_bone.b_b.seq = seq;
				seq++;
				if (seq >= 21)
				{
					seq = 0;
					putFifo (56, node_pkt_header);
				}
				short_message (&node_msg, &node_pkt);
				if (node_voice_send_sw) putFifo (27, node_pkt);
				if (dv_status.port) status_send_update(&node_pkt);
				node_rig_pkt_cnt++;
				node_rig_pkt_total_cnt++;
			}
			else if (voice_pnt == 15)
			{
				if (!memcmp (&voice_save[9], lastframe, 6))
				{
					memcpy (&node_pkt.dstar.b_bone.dstar_udp.voice_d, voice_save, 15);
					dv_pkt_set(&node_pkt);
					node_pkt.pkt_type = 0x20;
					node_pkt.dstar.b_bone.b_b.seq = seq;
					seq++;
					if (node_voice_send_sw) putFifo (27, node_pkt);
					dv_pkt_set(&node_pkt);
					node_pkt.pkt_type = 0x20;
					node_pkt.dstar.b_bone.b_b.seq = seq | 0x40;
					memcpy (&node_pkt.dstar.b_bone.dstar_udp.voice_d, &lastframe[3], 3);
					if (node_voice_send_sw) putFifo (30, node_pkt);
					if (dv_status.port) status_send_pttoff();
					node_last_frame_sw = FALSE;
					node_NoRespReply_sw = TRUE;
					if (debug)
					{
						time(&cur_time);
						fprintf (log_file, "%24.24s from RIG packets:%ld\n", 
							ctime(&cur_time), node_rig_pkt_cnt);
						fflush (log_file);
					}
					node_rig_pkt_cnt = 0;
				}	
			}	
		}
		return ret;
	}
	else if (ret < 0)
	{
		node_close ();
	}
	return 0;
}

void	node_last_frame_send (void)
{
	memcpy (&node_pkt.dstar.b_bone.dstar_udp.voice_d.data_segment, lastframe, 6);
	dv_pkt_set(&node_pkt);
	node_pkt.pkt_type = 0x20;
	node_pkt.dstar.b_bone.b_b.seq = seq | 0x40;
	if (node_voice_send_sw) putFifo (30, node_pkt);
	if (dv_status.port) status_send_pttoff();
	node_last_frame_sw = FALSE;
	node_NoRespReply_sw = TRUE;
}


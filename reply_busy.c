#include	"dv_ap.h"

void    dv_pkt_set(struct dv_packet *hdr);

void	reply_busy (struct dv_header	header)
{
	unsigned short int	tmp;

	dv_pkt_set(&reply_dv_pkt);
	dv_pkt.pkt_type = 0x10;
	reply_dv_pkt.dstar.b_bone.b_b.id = 0x20;	/* voice */
        reply_dv_pkt.dstar.b_bone.b_b.dest_repeater_id = 0x00;
	reply_dv_pkt.dstar.b_bone.b_b.send_repeater_id = 0x01;
        reply_dv_pkt.dstar.b_bone.b_b.send_terminal_id = 0xff;
	tmp = rand() & 0xffff;
	memcpy (reply_dv_pkt.dstar.b_bone.b_b.frame_id, &tmp, 2);
	reply_dv_pkt.dstar.b_bone.b_b.seq = 0x80;
	memcpy (&reply_dv_pkt.dstar.b_bone.dstar_udp, &header, 41);
	memcpy (&reply_dv_pkt.dstar.b_bone.dstar_udp.rf_header.MyCall, node_area_rep_callsign, 8);	
	memcpy (&reply_dv_pkt.dstar.b_bone.dstar_udp.rf_header.YourCall, &header.MyCall, 8);
	memcpy (&reply_dv_pkt.dstar.b_bone.dstar_udp.rf_header.MyCall2, "    ", 4);
	reply_dv_pkt.dstar.b_bone.dstar_udp.rf_header.flags[0] |= 0x01;	/* not transmit or busy */

	putFifo (56, reply_dv_pkt);


}


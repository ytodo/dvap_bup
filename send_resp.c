#include	"dv_ap.h"

void    node_send_resp_voice(char msg[]);
int	cos_check(void);

char   NullVoice0[12] = {0x9e,0x8d,0x32,0x88,0x26,0x1a,0x3f,0x61,0xe8,0x55,0x2d,0x16};
char   NullVoice1[12] = {0x9e,0x8d,0x32,0x88,0x26,0x1a,0x3f,0x61,0xe8,0x16,0x29,0xf5};
extern	char	dvap_ptt[];

enum
{
        SKIP = 0,
	SEND_WAIT_SET,
	SEND_WAIT_CHECK,
	HEADER_SET,
	HEADER_SEND_CHECK,
	SEND_PTT_ON,
	VOICE_SEND_CHECK,
	VOICE_SEND,
	SEND_LAST_CHECK,
	SEND_LAST,
	SEND_PTT_OFF
} node_response_state = SKIP;

struct	timeval	node_send_wait;
char	msg[20];
unsigned char	frame_seq = 0;
extern	char	lastframe[];


void	node_send_response(void)
{
	struct timeval	tm;
	struct timeval	tm_tmp;
	struct timeval	wait_timer;
	unsigned char	len;
	int	ret;
	ssize_t	wrt_len;
	

	char	msg_buf[15];

	if (!node_sw) return;

	if (!(node_gw_resp_sw || node_NoRespReply_sw) || cos_check())
	{
		if (node_response_state >= VOICE_SEND_CHECK)
			usb_control_msg(udev, 0x40, SET_PTT, OFF, 0, NULL, 0, 200);		
		node_response_state = SKIP;
		return;
	}

	wait_timer.tv_sec = 1;
	wait_timer.tv_usec = 0;

	switch (node_response_state)
	{
		case SKIP:
			break;
		case SEND_WAIT_SET:
			gettimeofday(&node_send_wait, NULL);
			node_response_state = SEND_WAIT_CHECK;
			frame_seq = 0;
			break;
		case SEND_WAIT_CHECK:
			gettimeofday(&tm, NULL);
                        timersub (&tm, &node_send_wait, &tm_tmp);
                        if (timercmp(&tm_tmp, &wait_timer,  >)) node_response_state = HEADER_SET;
			break;
		case HEADER_SET:
			/* Callsign set */
			if (node_gw_resp_sw)
			{
				usb_control_msg(udev, 0x40, SET_MyCALL, 0, 0, node_gw_resp.MyCall, 8, 200);
				usb_control_msg(udev, 0x40, SET_MyCALL2, 0, 0, node_gw_resp.MyCall2 , 4, 200);
				usb_control_msg(udev, 0x40, SET_YourCALL, 0, 0, node_gw_resp.YourCall, 8, 200);
				usb_control_msg(udev, 0x40, SET_RPT1CALL, 0, 0, node_gw_resp.RPT1Call, 8, 200);
				usb_control_msg(udev, 0x40, SET_RPT2CALL, 0, 0, node_gw_resp.RPT2Call, 8, 200);
				node_gw_resp.flags[0] &= 0x07;
				usb_control_msg(udev, 0x40, SET_FLAGS, 0, 0, node_gw_resp.flags, 3, 200);
			}
			else if (node_NoRespReply_sw)
			{
				usb_control_msg(udev, 0x40, SET_MyCALL, 0, 0, node_area_rep_callsign, 8, 200);
				usb_control_msg(udev, 0x40, SET_MyCALL2, 0, 0, "    " , 4, 200);
				usb_control_msg(udev, 0x40, SET_YourCALL, 0, 0, node_NoResp.YourCall, 8, 200);
				usb_control_msg(udev, 0x40, SET_RPT1CALL, 0, 0, node_NoResp.RPT1Call, 8, 200);
				usb_control_msg(udev, 0x40, SET_RPT2CALL, 0, 0, node_NoResp.RPT2Call, 8, 200);
				node_NoResp.flags[0] = 0x02;
				node_NoResp.flags[1] = 0x00;
				node_NoResp.flags[2] = 0x00;
				usb_control_msg(udev, 0x40, SET_FLAGS, 0, 0, node_NoResp.flags, 3, 200);
			}
			node_response_state = HEADER_SEND_CHECK;
			break;
		case HEADER_SEND_CHECK:
			usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len , 1, 200);
			if (len > 95) node_response_state = SEND_PTT_ON;
			break;
		case SEND_PTT_ON:
			usb_control_msg(udev, 0x40, SET_PTT, ON, 0, NULL, 0, 200);
			node_response_state = VOICE_SEND_CHECK;
			if (node_gw_resp_sw) memcpy (msg, "Dest. Busy          ", 20);
			else if (node_NoRespReply_sw) memcpy (msg, "No Response         ", 20);
			break;
		case VOICE_SEND_CHECK:
			usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len , 1, 200);
			if (len > 12)
			{
				node_response_state = VOICE_SEND;
			}
			break;
		case VOICE_SEND:
			if (frame_seq == 0) ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, NullVoice0, 12, 200);
			else
			{
				memcpy (msg_buf, NullVoice1, 12);
				switch (frame_seq)
				{
                                	case 1:
                                        	msg_buf[9] = 0x40 ^ 0x70;
                                        	msg_buf[10] = msg[0] ^ 0x4f;
                                        	msg_buf[11] = msg[1] ^ 0x93;
                                        	break;
                                	case 2:
                                       		msg_buf[9] = msg[2] ^ 0x70;
                                        	msg_buf[10] = msg[3] ^ 0x4f;
                                        	msg_buf[11] = msg[4] ^ 0x93;
                                        	break;
                                	case 3:
                                        	msg_buf[9] = 0x41 ^ 0x70;
                                        	msg_buf[10] = msg[5] ^ 0x4f;
                                        	msg_buf[11] = msg[6] ^ 0x93;
                                        	break;
                                	case 4:
                                        	msg_buf[9] = msg[7] ^ 0x70;
                                        	msg_buf[10] = msg[8] ^ 0x4f;
                                        	msg_buf[11] = msg[9] ^ 0x93;
                                        	break;
                                	case 5:
                                        	msg_buf[9] = 0x42 ^ 0x70;
                                        	msg_buf[10] = msg[10] ^ 0x4f;
                                        	msg_buf[11] = msg[11] ^ 0x93;
                                        	break;
                                	case 6:
                                        	msg_buf[9] = msg[12] ^ 0x70;
                                        	msg_buf[10] = msg[13] ^ 0x4f;
                                        	msg_buf[11] = msg[14] ^ 0x93;
                                        	break;
                                	case 7:
                                        	msg_buf[9] = 0x43 ^ 0x70;
                                        	msg_buf[10] = msg[15] ^ 0x4f;
                                        	msg_buf[11] = msg[16] ^ 0x93;
                                        	break;
                                	case 8:
                                        	msg_buf[9] = msg[17] ^ 0x70;
                                        	msg_buf[10] = msg[18] ^ 0x4f;
                                        	msg_buf[11] = msg[19] ^ 0x93;
                                        	break;
                        	}
				ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, msg_buf, 12, 200);
			}
			if (ret < 0) break;
			frame_seq++;
			if (frame_seq == 11) node_response_state = SEND_LAST_CHECK;
			else node_response_state = VOICE_SEND_CHECK;
			break;
		case SEND_LAST_CHECK:
			usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len , 1, 200);
			if (len > 15) node_response_state = SEND_LAST;
			break;	
		case SEND_LAST:
			memcpy (&msg_buf[9], lastframe, 6);
			ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, msg_buf, 15, 200);
			if (ret < 0) break;
			node_response_state = SEND_PTT_OFF;
			break;	
		case SEND_PTT_OFF:
			usb_control_msg(udev, 0x40, SET_PTT, OFF, 0, NULL, 0, 200);
			node_response_state = SKIP;
			if (node_gw_resp_sw) node_gw_resp_sw = FALSE;
			if (node_NoRespReply_sw) node_NoRespReply_sw = FALSE;
			break;
	}
}

void	node_send_wait_set(void)
{
	if (node_response_state == SKIP) node_response_state = SEND_WAIT_SET;
}

void    node_skip (void)
{
        //node_response_state = SEND_PTT_OFF;
	usb_control_msg(udev, 0x40, SET_PTT, OFF, 0, NULL, 0, 200);
	node_response_state = SKIP;
	if (node_gw_resp_sw) node_gw_resp_sw = FALSE;
	if (node_NoRespReply_sw) node_NoRespReply_sw = FALSE;
}

enum
{
        DVAP_SKIP = 0,
	DVAP_SEND_WAIT_SET,
	DVAP_SEND_WAIT_CHECK,
	DVAP_HEADER_SET,
	DVAP_HEADER_SEND_CHECK,
	DVAP_SEND_PTT_ON,
	DVAP_VOICE_SEND_CHECK,
	DVAP_VOICE_SEND,
	DVAP_SEND_LAST_CHECK,
	DVAP_SEND_LAST,
	DVAP_SEND_PTT_OFF
} dvap_response_state = DVAP_SKIP;

struct	timeval	dvap_send_wait;
char	dvap_message[20];
unsigned char	dvap_resp_pkt[47];
unsigned char	dvap_frame_seq = 0;
extern	char	lastframe[];


void	dvap_send_response(void)
{
	struct timeval	tm;
	struct timeval	tm_tmp;
	struct timeval	wait_timer;
	unsigned short int tmp;
	ssize_t	wrt_len;

	if (!dvap_sw) return;

        if (!(dvap_NoRespReply_sw || dvap_gw_resp_sw))
        {
                dvap_response_state = SKIP;
                return;
        }	
	wait_timer.tv_sec = 1;
	wait_timer.tv_usec = 0;

	switch (dvap_response_state)
	{
		case DVAP_SKIP:
			break;
		case DVAP_SEND_WAIT_SET:
			if (dvap_squelch_status) break;
			gettimeofday(&dvap_send_wait, NULL);
			dvap_response_state = DVAP_SEND_WAIT_CHECK;
			dvap_frame_seq = 0;
			break;
		case DVAP_SEND_WAIT_CHECK:
			if (dvap_squelch_status) break;
			gettimeofday(&tm, NULL);
                        timersub (&tm, &dvap_send_wait, &tm_tmp);
                        if (timercmp(&tm_tmp, &wait_timer,  >)) dvap_response_state = DVAP_HEADER_SET;
			break;
		case DVAP_HEADER_SET:
			dvap_resp_pkt[0] = 0x2f;
			dvap_resp_pkt[1] = 0xa0;
			tmp = rand() & 0xffff;
			memcpy (&dvap_resp_pkt[2], &tmp, 2);
			dvap_resp_pkt[4] = 0x80;
			dvap_resp_pkt[5] = 0x00;
			/* Callsign set */
			if (dvap_gw_resp_sw)
			{
				memcpy (&dvap_resp_pkt[6], &dvap_gw_resp, 41);
				dvap_resp_pkt[6] &= 0x07;
			}
			else if (dvap_NoRespReply_sw)
			{
				memcpy (&dvap_resp_pkt[6], &dvap_NoResp, 41);
				dvap_resp_pkt[6] = 0x02;
				dvap_resp_pkt[7] = 0x00;
				dvap_resp_pkt[8] = 0x00;
			}
			dvap_response_state = DVAP_HEADER_SEND_CHECK;
			wrt_len = write (dvap_fd, dvap_resp_pkt, 47);
			break;
		case DVAP_HEADER_SEND_CHECK:
			dvap_response_state = DVAP_SEND_PTT_ON;
			break;
		case DVAP_SEND_PTT_ON:
			dvap_response_state = DVAP_VOICE_SEND_CHECK;
			if (dvap_gw_resp_sw) memcpy (dvap_message, "Dest. Busy          ", 20);
			else if (dvap_NoRespReply_sw) memcpy (dvap_message, "No Response         ", 20);
			break;
		case DVAP_VOICE_SEND_CHECK:
			dvap_resp_pkt[0] = 0x12;
			dvap_resp_pkt[1] = 0xc0;
			dvap_response_state = DVAP_VOICE_SEND;
			break;
		case DVAP_VOICE_SEND:
			dvap_resp_pkt[4] = dvap_frame_seq;
			dvap_resp_pkt[5]++;
			memcpy (&dvap_resp_pkt[6], NullVoice1, 12);
			switch (dvap_frame_seq)
			{
				case 0:
					memcpy (&dvap_resp_pkt[6], NullVoice0, 12);
					break;
                               	case 1:
                                       	dvap_resp_pkt[15] = 0x40 ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[0] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[1] ^ 0x93;
                                       	break;
                               	case 2:
                              		dvap_resp_pkt[15] = dvap_message[2] ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[3] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[4] ^ 0x93;
                                       	break;
                               	case 3:
                                       	dvap_resp_pkt[15] = 0x41 ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[5] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[6] ^ 0x93;
                                       	break;
                               	case 4:
                                       	dvap_resp_pkt[15] = dvap_message[7] ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[8] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[9] ^ 0x93;
                                       	break;
                               	case 5:
                                       	dvap_resp_pkt[15] = 0x42 ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[10] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[11] ^ 0x93;
                                       	break;
                               	case 6:
                                       	dvap_resp_pkt[15] = dvap_message[12] ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[13] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[14] ^ 0x93;
                                       	break;
                               	case 7:
                                       	dvap_resp_pkt[15] = 0x43 ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[15] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[16] ^ 0x93;
                                       	break;
                               	case 8:
                                       	dvap_resp_pkt[15] = dvap_message[17] ^ 0x70;
                                       	dvap_resp_pkt[16] = dvap_message[18] ^ 0x4f;
                                       	dvap_resp_pkt[17] = dvap_message[19] ^ 0x93;
                                       	break;
			}
			wrt_len = write (dvap_fd, dvap_resp_pkt, 18);
			dvap_frame_seq++;
			if (dvap_frame_seq >= 11) dvap_response_state = DVAP_SEND_LAST_CHECK;
			break;
		case DVAP_SEND_LAST_CHECK:
			dvap_response_state = DVAP_SEND_LAST;
			break;	
		case DVAP_SEND_LAST:
			dvap_resp_pkt[4] = dvap_frame_seq;
			dvap_resp_pkt[4] |= 0x40;
			dvap_resp_pkt[5]++;
			wrt_len = write (dvap_fd, dvap_resp_pkt, 18);
			dvap_response_state = DVAP_SEND_PTT_OFF;
			break;	
		case DVAP_SEND_PTT_OFF:
			dvap_response_state = DVAP_SKIP;
			if (dvap_gw_resp_sw) dvap_gw_resp_sw = FALSE;
			if (dvap_NoRespReply_sw) dvap_NoRespReply_sw = FALSE;
			break;
	}
}

void	dvap_send_wait_set(void)
{
	if (dvap_response_state == DVAP_SKIP) dvap_response_state = DVAP_SEND_WAIT_SET;
}

void	dvap_skip (void)
{
	//dvap_response_state = DVAP_SEND_PTT_OFF;
        dvap_response_state = DVAP_SKIP;
        if (dvap_gw_resp_sw) dvap_gw_resp_sw = FALSE;
        if (dvap_NoRespReply_sw) dvap_NoRespReply_sw = FALSE;
}

char    IDxxPlus_message[20];
unsigned char   IDxxPlus_resp_pkt[47];

enum
{
        IDxxPlus_SKIP = 0,
	IDxxPlus_SEND_WAIT_SET,
	IDxxPlus_SEND_WAIT_CHECK,
	IDxxPlus_HEADER_SET,
	IDxxPlus_HEADER_SEND_CHECK,
	IDxxPlus_SEND_PTT_ON,
	IDxxPlus_VOICE_SEND_CHECK,
	IDxxPlus_VOICE_SEND,
	IDxxPlus_SEND_LAST_CHECK,
	IDxxPlus_SEND_LAST,
	IDxxPlus_SEND_PTT_OFF
} IDxxPlus_response_state = IDxxPlus_SKIP;

void	IDxxPlus_send_response(void)
{
	struct timeval	tm;
	struct timeval	tm_tmp;
	struct timeval	wait_timer;
	unsigned short int tmp;
	ssize_t	wrt_len;

	if (!IDxxPlus_sw) return;

        if (!(IDxxPlus_NoRespReply_sw || IDxxPlus_gw_resp_sw))
        {
                IDxxPlus_response_state = SKIP;
                return;
        }	
	wait_timer.tv_sec = 1;
	wait_timer.tv_usec = 0;

	switch (IDxxPlus_response_state)
	{
		case IDxxPlus_SKIP:
			break;
		case IDxxPlus_SEND_WAIT_SET:
			gettimeofday(&dvap_send_wait, NULL);
			IDxxPlus_response_state = IDxxPlus_SEND_WAIT_CHECK;
			IDxxPlus_frame_seq = 0;
			break;
		case IDxxPlus_SEND_WAIT_CHECK:
			gettimeofday(&tm, NULL);
                        timersub (&tm, &dvap_send_wait, &tm_tmp);
                        if (timercmp(&tm_tmp, &wait_timer,  >)) IDxxPlus_response_state = IDxxPlus_HEADER_SET;
			break;
		case IDxxPlus_HEADER_SET:
#if 0
			IDxxPlus_resp_pkt[0] = 0x2f;
			IDxxPlus_resp_pkt[1] = 0xa0;
			tmp = rand() & 0xffff;
			memcpy (&IDxxPlus_resp_pkt[2], &tmp, 2);
			IDxxPlus_resp_pkt[4] = 0x80;
			IDxxPlus_resp_pkt[5] = 0x00;
			/* Callsign set */
			if (IDxxPlus_gw_resp_sw)
			{
				memcpy (&IDxxPlus_resp_pkt[6], &IDxxPlus_gw_resp, 41);
				IDxxPlus_resp_pkt[6] &= 0x07;
			}
			else if (IDxxPlus_NoRespReply_sw)
			{
				memcpy (&IDxxPlus_resp_pkt[6], &IDxxPlus_NoResp, 41);
				IDxxPlus_resp_pkt[6] = 0x02;
				IDxxPlus_resp_pkt[7] = 0x00;
				IDxxPlus_resp_pkt[8] = 0x00;
			}
			IDxxPlus_response_state = IDxxPlus_HEADER_SEND_CHECK;
			//wrt_len = write (IDxxPlus_fd, _resp_pkt, 47);
#endif
			break;
		case IDxxPlus_HEADER_SEND_CHECK:
			IDxxPlus_response_state = IDxxPlus_SEND_PTT_ON;
			break;
		case IDxxPlus_SEND_PTT_ON:
			IDxxPlus_response_state = IDxxPlus_VOICE_SEND_CHECK;
			if (IDxxPlus_gw_resp_sw) memcpy (IDxxPlus_message, "Dest. Busy          ", 20);
			else if (IDxxPlus_NoRespReply_sw) memcpy (IDxxPlus_message, "No Response         ", 20);
			break;
		case IDxxPlus_VOICE_SEND_CHECK:
			IDxxPlus_resp_pkt[0] = 0x12;
			IDxxPlus_resp_pkt[1] = 0xc0;
			IDxxPlus_response_state = IDxxPlus_VOICE_SEND;
			break;
		case IDxxPlus_VOICE_SEND:
			IDxxPlus_resp_pkt[4] = IDxxPlus_frame_seq;
			IDxxPlus_resp_pkt[5]++;
			memcpy (&IDxxPlus_resp_pkt[6], NullVoice1, 12);
			switch (IDxxPlus_frame_seq)
			{
				case 0:
					memcpy (&IDxxPlus_resp_pkt[6], NullVoice0, 12);
					break;
                               	case 1:
                                       	IDxxPlus_resp_pkt[15] = 0x40 ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[0] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[1] ^ 0x93;
                                       	break;
                               	case 2:
                              		IDxxPlus_resp_pkt[15] = IDxxPlus_message[2] ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[3] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[4] ^ 0x93;
                                       	break;
                               	case 3:
                                       	IDxxPlus_resp_pkt[15] = 0x41 ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[5] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[6] ^ 0x93;
                                       	break;
                               	case 4:
                                       	IDxxPlus_resp_pkt[15] = IDxxPlus_message[7] ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[8] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[9] ^ 0x93;
                                       	break;
                               	case 5:
                                       	IDxxPlus_resp_pkt[15] = 0x42 ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[10] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[11] ^ 0x93;
                                       	break;
                               	case 6:
                                       	IDxxPlus_resp_pkt[15] = IDxxPlus_message[12] ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[13] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[14] ^ 0x93;
                                       	break;
                               	case 7:
                                       	IDxxPlus_resp_pkt[15] = 0x43 ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[15] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[16] ^ 0x93;
                                       	break;
                               	case 8:
                                       	IDxxPlus_resp_pkt[15] = IDxxPlus_message[17] ^ 0x70;
                                       	IDxxPlus_resp_pkt[16] = IDxxPlus_message[18] ^ 0x4f;
                                       	IDxxPlus_resp_pkt[17] = IDxxPlus_message[19] ^ 0x93;
                                       	break;
			}
			wrt_len = write (IDxxPlus_fd, IDxxPlus_resp_pkt, 18);
			IDxxPlus_frame_seq++;
			if (IDxxPlus_frame_seq >= 11) IDxxPlus_response_state = IDxxPlus_SEND_LAST_CHECK;
			break;
		case IDxxPlus_SEND_LAST_CHECK:
			IDxxPlus_response_state = IDxxPlus_SEND_LAST;
			break;	
		case IDxxPlus_SEND_LAST:
			IDxxPlus_resp_pkt[4] = IDxxPlus_frame_seq;
			IDxxPlus_resp_pkt[4] |= 0x40;
			IDxxPlus_resp_pkt[5]++;
			wrt_len = write (dvap_fd, dvap_resp_pkt, 18);
			IDxxPlus_response_state = IDxxPlus_SEND_PTT_OFF;
			break;	
		case IDxxPlus_SEND_PTT_OFF:
			IDxxPlus_response_state = IDxxPlus_SKIP;
			if (IDxxPlus_gw_resp_sw) IDxxPlus_gw_resp_sw = FALSE;
			if (IDxxPlus_NoRespReply_sw) IDxxPlus_NoRespReply_sw = FALSE;
			break;
	}
}

void	IDxxPlus_send_wait_set(void)
{
	if (IDxxPlus_response_state == DVAP_SKIP) IDxxPlus_response_state = IDxxPlus_SEND_WAIT_SET;
}

void	IDxxPlus_skip (void)
{
	//IDxxPlus_response_state = DVAP_SEND_PTT_OFF;
        IDxxPlus_response_state = IDxxPlus_SKIP;
        if (IDxxPlus_gw_resp_sw) IDxxPlus_gw_resp_sw = FALSE;
        if (IDxxPlus_NoRespReply_sw) IDxxPlus_NoRespReply_sw = FALSE;
}

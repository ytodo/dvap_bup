#include	"dv_ap.h"

void	node_skip (void);
void	dvap_skip (void);

unsigned char	dvap_send_buff[47];

char    dvap_ptt[5] = {0x05, 0x20, 0x18, 0x01, 0x01};

void	header_send (struct dv_header header)
{
	unsigned char	len;
	unsigned short int	tmp;
	ssize_t	wrt_len;
	unsigned char   call_temp[8];

	if (debug)
	{
		time(&cur_time);
		fprintf (log_file, "%24.24s from INET RPT2:%8.8s RPT1:%8.8s Ur:%8.8s My:%8.8s/%4.4s\n", 
			ctime(&cur_time), header.RPT2Call, header.RPT1Call, header.YourCall, 
			header.MyCall, header.MyCall2);
		fflush (log_file);
	}

	if (!memcmp (node_area_rep_callsign, header.RPT1Call, 8) && node_sw)
	{

		if (cos_check()) return;
		node_skip();
	
		/* Callsign set */
		usb_control_msg(udev, 0x40, SET_MyCALL, 0, 0, header.MyCall, 8, 100);
		usb_control_msg(udev, 0x40, SET_MyCALL2, 0, 0, header.MyCall2 , 4, 100);
		usb_control_msg(udev, 0x40, SET_YourCALL, 0, 0, header.YourCall, 8, 100);
		usb_control_msg(udev, 0x40, SET_RPT1CALL, 0, 0, header.RPT2Call, 8, 100);
		usb_control_msg(udev, 0x40, SET_RPT2CALL, 0, 0, header.RPT1Call, 8, 100);
		header.flags[0] &= 0x07;
		usb_control_msg(udev, 0x40, SET_FLAGS, 0, 0, header.flags, 3, 100);

		usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
		while (len < 95)
		{
			usleep (100000);
			usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
		}
		usb_control_msg(udev, 0x40, SET_PTT, ON, 0, NULL, 0, 100);
	}
	else if (!memcmp (dvap_area_rep_callsign, header.RPT1Call, 8) && dvap_sw)
	{
		//dvap_header_send_ok = FALSE;
		dvap_skip();
		time(&dvap_keep_alive);
		dvap_send_buff[0] = 0x2f;
		dvap_send_buff[1] = 0xa0;
		tmp = rand() & 0xffff;
		memcpy (&dvap_send_buff[2], &tmp, 2);
		dvap_send_buff[4] = 0x80;
		dvap_send_buff[5] = 0x00;
		memcpy (call_temp, header.RPT1Call, 8);
		memcpy (header.RPT1Call, header.RPT2Call, 8);
		memcpy (header.RPT2Call, call_temp, 8);
		memcpy (&dvap_send_buff[6], &header, 41);
		wrt_len = write (dvap_fd, dvap_send_buff, 47);
		dvap_send_buff[4] = 0x00;
		dvap_send_buff[5]++;
	}
}

void	node_voice_send (char voice[])
{
	int	ret;
	unsigned char	len;

	usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
	while (len < 12)
	{
		usleep (20000);
		usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
	}
	ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, voice, 12, 100);
	while (ret < 0)
	{
		usleep (2000);
		ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, voice, 12, 100);
	}
}

void	dvap_voice_send (char voice[])
{
	ssize_t	wrt_len;

	time(&dvap_keep_alive);
	dvap_send_buff[0] = 0x12;
	dvap_send_buff[1] = 0xc0;
	memcpy (&dvap_send_buff[6], voice, 12);
	wrt_len = write (dvap_fd, dvap_send_buff, 18);
	dvap_send_buff[5]++;
	dvap_send_buff[4]++;
	if (dvap_send_buff[4] == 21) dvap_send_buff[4] = 0;
}

void	node_last_send (char voice[])
{
	int	ret;
	unsigned char	len;

	usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
	while (len < 15)
       	{
               	usleep (20000);
               	usb_control_msg(udev, 0xc0, GET_REMAINSPACE, 0, 0, (char *)&len ,1, 100);
       	}
       	ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, voice, 15, 100);
       	while (ret < 0)
       	{
               	usleep (2000);
               	ret = usb_control_msg(udev, 0x40, PUT_DATA, 0, 0, voice, 15, 100);
       	}
	/* PTT OFF */
	ret = usb_control_msg(udev, 0x40, SET_PTT, OFF, 0, NULL, 0, 100);
	memset (node_save_frame_id, 0x00, 2);
}

void	dvap_last_send (char voice[])
{
	ssize_t	wrt_len;

	time (&dvap_keep_alive);
	dvap_send_buff[0] = 0x12;
	dvap_send_buff[1] = 0xc0;
	dvap_send_buff[4] |= 0x40;
	memcpy (&dvap_send_buff[6], voice, 12);
	wrt_len = write (dvap_fd, dvap_send_buff, 18);
	memset (dvap_save_frame_id, 0x00, 2);
	dvap_ptt[4] = 0x00;
	usleep (1000);
	wrt_len = write (dvap_fd, dvap_ptt, 5);
}

void    IDxxPlus_last_send (char voice[])
{
        ssize_t wrt_len;

        time (&dvap_keep_alive);
        dvap_send_buff[0] = 0x12;
        dvap_send_buff[1] = 0xc0;
        dvap_send_buff[4] |= 0x40;
        memcpy (&dvap_send_buff[6], voice, 12);
        wrt_len = write (dvap_fd, dvap_send_buff, 18);
        memset (IDxxPlus_save_frame_id, 0x00, 2);
        dvap_ptt[4] = 0x00;
        usleep (1000);
        wrt_len = write (IDxxPlus_fd, dvap_ptt, 5);
}


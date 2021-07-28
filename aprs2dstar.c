#include	"dv_ap.h"

unsigned short int        update_crc_dstar( unsigned short int  crc, unsigned char c );
unsigned short int                result_crc_dstar(unsigned short int crc);
int	ptt_check (void);

char NullVoice_ID31[12] = {0xb2,0x4d,0x22,0x48,0xc0,0x16,0x28,0x26,0xc8,0x55,0x2d,0x16};

int	n_seq;

void     aprs2dstar_CRC(unsigned char  string[], unsigned char crc_hdr[])
{
        unsigned short int      crc_dstar_ffff;
        unsigned char   *pnt;
	unsigned char	temp[10];
	int	n;

        crc_dstar_ffff = 0xffff;        /* nornal value 0xffff */
        pnt = string;

	n = 0;
        while (*pnt != 0x0a)
        {
                crc_dstar_ffff = update_crc_dstar( crc_dstar_ffff, *pnt);
                pnt++;
		n++;
        }

        crc_dstar_ffff = result_crc_dstar(crc_dstar_ffff);

	sprintf (temp, "%4.4X", crc_dstar_ffff);
	memcpy (crc_hdr, "$$CRC", 5);
	crc_hdr[5] = temp[2];
	crc_hdr[6] = temp[3];
	crc_hdr[7] = temp[0];
	crc_hdr[8] = temp[1];
	crc_hdr[9] = ',';
}

void	aprs_voice_send (char msg[])
{
	if (n_seq == 0)
	{
		aprs_putFifo (12, NullVoice_ID31);	
		n_seq++;
	}
	if (n_seq >= 21) n_seq = 0;
	msg[9] ^=  0x70;
	msg[10] ^= 0x4f;
	msg[11] ^= 0x93;
	aprs_putFifo (12, msg);
	n_seq++;
	if (n_seq >= 21) n_seq = 0;
}

void    aprs_last_send (void)
{
	extern	char	lastframe[];
	char	temp[15];

	if (n_seq == 0)
	{
		aprs_putFifo (12, NullVoice_ID31);
		n_seq++;
	}
	memcpy (&temp, &NullVoice_ID31, 12);
	memcpy (&temp[9], lastframe, 6);
	aprs_putFifo (15, temp);
}

void	aprs2dstar(char msg[])
{
        struct  dv_header       hdr;
        int     len;
        int     i;
        int     k;
        int     m;
        int     n;
        int     msg_len;
	int	aprs_send;
	int	aprs_mycall;
        char    temp[12];
        char    data_temp[6];
        char    aprs_crc_hdr[10];
	char	msg_temp[256];

	if (msg[0] == '#') return;
	msg_len = strlen(msg) - 1;

	memset (hdr.flags, 0x00, 3);
	memcpy (hdr.RPT2Call, "DIRECT  ", 8);
	memcpy (hdr.YourCall, "CQCQCQ  ", 8);
	if (node_area_rep_callsign[0] != 0x20)
	{
		memcpy (hdr.RPT1Call, node_area_rep_callsign, 8);
	} 
	else if (dvap_area_rep_callsign[0] != 0x20)
	{
		memcpy (hdr.RPT1Call, dvap_area_rep_callsign, 8);
	}
	else if (IDxxPlus_area_rep_callsign[0] != 0x20)
	{
		memcpy (hdr.RPT1Call, IDxxPlus_area_rep_callsign, 8);
	}
	memcpy (hdr.MyCall2, "APRS", 4);

	m = 0;
	while (m < msg_len)
	{
		n_seq = 0;
		n = 0;
		aprs_mycall = FALSE;
		memset (hdr.MyCall, 0x20, 8);
		k = 0;
		while (msg[m] != 0x0a)
		{
			if (!aprs_mycall)
			{
				if ((msg[m] == '-') || (msg[m] == '>'))
				{
					aprs_mycall = TRUE;
				} else {
					hdr.MyCall[k++] = msg[m];
				}
			}
			msg_temp[n++] = msg[m++];
		}
		msg_temp[n++] = 0x0a;
		msg_temp[n] = 0x00;
		aprs_send = FALSE;
		//if (!ptt_check())
		if (!aprs_ptt_onoff && !ptt_check())
		{
			//header_send (hdr);
			aprs_putFifo (41, (char *)&hdr);
			aprs_send = TRUE;
			aprs_ptt_onoff = TRUE;
		}
			
		m++;
		//if (aprs_send)
		if (aprs_ptt_onoff)
		{	
			aprs2dstar_CRC (msg_temp, aprs_crc_hdr);
			if (msg[n] == 0x0a) n++;
			memcpy (temp, NullVoice_ID31, 9);
			temp[9] = 0x35;
			memcpy (&temp[10], aprs_crc_hdr, 2);
			aprs_voice_send (temp);
			memcpy (&temp[9], &aprs_crc_hdr[2], 3);
			aprs_voice_send (temp);
			temp[9] = 0x35;
			memcpy (&temp[10], &aprs_crc_hdr[5], 2);
			aprs_voice_send (temp);
			memcpy (&temp[9], &aprs_crc_hdr[7], 3);
			aprs_voice_send (temp);

			memcpy (temp, NullVoice_ID31, 12);
			for (i = 0 ; i <  n ; i += 5)
			{
				memset (data_temp, 0x66, 6);
				k = n - i;
				if (k > 5) k = 5;
				data_temp[0] = 0x30 | (k & 0x0f);
				memcpy (&data_temp[1], &msg_temp[i], k);
				memcpy (&temp[9], &data_temp[0], 3);
				aprs_voice_send (temp);

				memcpy (&temp[9], &data_temp[3], 3);
				aprs_voice_send (temp);
			}
			aprs_last_send();
		}	
		else
		{
			time (&cur_time);
			fprintf (log_file, "%24.24s Skip APRS msg: %s\n", ctime(&cur_time), msg_temp);
			fflush (log_file);
		} 
	}

}

void	aprs_msg_send (void)
{
	int	len;
	char	temp[41];
	struct	timeval	timer_tmp1;
	struct	timeval timer_tmp2;
	struct	timeval	aprs_interval;
	struct	dv_header	hdr;

	if (aprs_Rp->next == NULL) return;

	gettimeofday (&timer_tmp2, NULL);
        if (timercmp (&aprs_SendTime, &timer_tmp2, >)) return;
	memcpy (&timer_tmp1, &aprs_SendTime, sizeof(timer_tmp1));
        aprs_interval.tv_sec = 0;
        aprs_interval.tv_usec = 20000;  /* 20 m sec. */
        timeradd (&timer_tmp1, &aprs_interval, &aprs_SendTime);
	aprs_getFifo (&len, &hdr.flags[0]);
	if (len == 41) header_send (hdr);
	else if (len == 12) node_voice_send (&hdr.flags[0]);
	else if (len == 15)
	{
		node_last_send (&hdr.flags[0]);
		aprs_ptt_onoff = FALSE;
	}
}

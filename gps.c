#include "dv_ap.h"
#include "dprs_symbol.h"

int        send_check(char call_sign[]);

extern	int	aprs_status;

int	GPS_SumCheck(unsigned char  string[])
{
	unsigned char	*pnt;
	unsigned char	sum, csum, tmp;
	time_t	atime;

	pnt = string;
	sum = 0;
	if (*pnt == '$') pnt++;
	while (*pnt !='*')
	{
		if ((*pnt == 0x0a) || (*pnt == 0x0d)) return FALSE;
		sum ^= *pnt;
		pnt++;
	}
	pnt++;
	tmp = *pnt - '0';
	if (tmp > 16) tmp -= 7;
	csum = tmp << 4;
	pnt++;
	tmp = *pnt - '0';
	if (tmp > 16) tmp -= 7;
	csum += tmp;
	if (debug)
	{
		time(&atime);
		fprintf (log_file, "%24.24s Sum Check Src:Calc  %2.2x:%2.2x\n", ctime(&atime), csum, sum);
		fflush (log_file);
	}
	if (csum == sum) return TRUE;
	return FALSE;
}

void	GPGLL (struct aprs_msg *id)
{
	unsigned char	*pnt;
	unsigned char	i;
	unsigned char	LatTemp[8];
	unsigned char	LongTemp[9];
	unsigned char	TimeTemp[7];

	pnt = id->aprs_msg + 7;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 7)
		{
			LatTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	LatTemp[7] = *pnt;
	pnt += 2;

	i = 0;
	while (*pnt != ',')
	{
		if (i < 8)
		{
			LongTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	LongTemp[8] = *pnt;
	pnt += 2;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 6)
		{
			TimeTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	TimeTemp[6] = 'h';
	pnt++;
	if (*pnt == 'V')
	{
		id->AprsSend = APRS_NG;
		memcpy (id->aprs_msg_save, id->aprs_msg, 256);
		return;
	}
	id->RadioGpsStatus = TRUE;
	memcpy (&id->RadioLat[0], LatTemp, 8);
	memcpy (&id->RadioLong[0], LongTemp, 9);
	memcpy (&id->RadioTime, TimeTemp, 7);
}

void	GPGGA (struct aprs_msg *id)
{
	unsigned char	*pnt;
	int	i;
	unsigned short int	d;
	unsigned char	tmp[10];
	unsigned char	LatTemp[8];
	unsigned char	LongTemp[9];
	unsigned char	TimeTemp[7];
	int	k;

	pnt = id->aprs_msg + 7;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 6)
		{
			TimeTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	TimeTemp[6] = 'h';
	pnt++;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 7)
		{
			LatTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	LatTemp[7] = *pnt;
	pnt += 2;

	i = 0;
	while (*pnt != ',')
	{
		if (i < 8)
		{
			LongTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	LongTemp[8] = *pnt;

	pnt += 2;
	if (*pnt == '0')
	{
		id->AprsSend = APRS_NG;
		memcpy (id->aprs_msg_save, id->aprs_msg, 256);
		return;
	}
	id->RadioGpsStatus = TRUE;
	memcpy (&id->RadioLat[0], LatTemp, 8);
	memcpy (&id->RadioLong[0], LongTemp, 9);
	memcpy (&id->RadioTime, TimeTemp, 7);

	while (*pnt != ',') pnt++;
	pnt++;
	while (*pnt != ',') pnt++;
	pnt++;
	while (*pnt != ',') pnt++;
	pnt++;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 8)
		{
			tmp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	tmp[i] = 0x00;
	d = atof ((char *)tmp);
	k = d / 0.3048 + 0.5;
	for (i = 0 ; i < 6 ; i++)
	{
		id->RadioAtitude[5 - i] = (k % 10) + '0';
		k /= 10;
	}
}

void	GPRMC (struct aprs_msg *id)
{
	int	d;
	unsigned char	tmp[10];
	int	k;
	unsigned char	*pnt;
	unsigned char	TimeTemp[7];
	unsigned char	i;

	pnt = id->aprs_msg + 7;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 6)
		{
			TimeTemp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	TimeTemp[6] = 'h';
	pnt++;
	if (*pnt == 'V')
	{
		id->AprsSend = APRS_NG;
		memcpy (id->aprs_msg_save, id->aprs_msg, 256);
		return;
	}
	id->RadioGpsStatus = TRUE;
	memcpy (&id->RadioTime, TimeTemp, 7);
	while (*pnt != ',') pnt++;
	pnt++;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 7)
		{
			id->RadioLat[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	id->RadioLat[7] = *pnt;
	pnt += 2;

	i = 0;
	while (*pnt != ',')
	{
		if (i < 8)
		{
			id->RadioLong[i] = *pnt;
			i++;
		}
		pnt++;
	}
	pnt++;
	id->RadioLong[8] = *pnt;
	pnt += 2;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 9)
		{
			tmp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	tmp[i] = 0x00;
	d = atof ((char *)tmp);
	k = d + 0,5;
	for (i = 0 ; i < 3 ; i++)
	{
		id->RadioSpeed[2 - i] = (k % 10) + '0';
		k /= 10;
	}

	pnt++;
	i = 0;
	while (*pnt != ',')
	{
		if (i < 9)
		{
			tmp[i] = *pnt;
			i++;
		}
		pnt++;
	}
	tmp[i] = 0x00;
	d = atof ((char *)tmp);
	k = d + 0,5;
	for (i = 0 ; i < 3 ; i++)
	{
		id->RadioDirection[2 - i] = (k % 10) + '0';
		k /= 10;
	}
}

void	GPVTG (struct aprs_msg *id)
{
	unsigned char	*pnt;
	unsigned char	tmp[10];
	unsigned char	DirectionTemp[3];
	unsigned char	SpeedTemp[3];
	int	i, k;
	int	d;

	pnt = id->aprs_msg + 7;

        i = 0;
        while (*pnt != ',')
        {
                if (i < 9)
                {
                        tmp[i] = *pnt;
                        i++;
                }
                pnt++;
        }
        tmp[i] = 0x00;
        d = atof ((char *)tmp);
        k = d + 0,5;
        for (i = 0 ; i < 3 ; i++)
        {
                DirectionTemp[2 - i] = (k % 10) + '0';
                k /= 10;
        }

	pnt += 3;
	while (*pnt != ',') pnt++;
	pnt += 3;
        i = 0;
        while (*pnt != ',')
        {
                if (i < 9)
                {
                        tmp[i] = *pnt;
                        i++;
                }
                pnt++;
        }
        tmp[i] = 0x00;
        d = atof ((char *)tmp);
        k = d + 0,5;
        for (i = 0 ; i < 3 ; i++)
        {
                SpeedTemp[2 - i] = (k % 10) + '0';
                k /= 10;
        }
	pnt += 3;
	while (*pnt != ',') pnt++;
	pnt += 3;
	if (*pnt == 'N') return;
	memcpy (&id->RadioDirection[0], DirectionTemp, 3);
	memcpy (&id->RadioSpeed[0], SpeedTemp, 3);
}

void	dprs_message (struct aprs_msg *id)
{
	unsigned char	*pnt;
	unsigned char	i, k;
	unsigned char	len;

	pnt = id->aprs_msg;
	i = 0;
	id->qsy_sw = FALSE;
	memset (&id->RadioCall[0], 0x00, 8);
	while (*pnt != ',')
	{
		if (*pnt == 0x00) return;
		if (i < 8)
		{
			id->RadioCall[i] = *pnt;
			i++;
		}
		pnt++;
	}
	k = 0;
	for (i = 0 ; i < 8 ; i++)
	{
		if (id->RadioCall[i] != 0x20)
		{
			id->RadioCall[k] = id->RadioCall[i];
			k++;
		} else {
			if (id->RadioCall[k-1] != '-')
			{
				id->RadioCall[k] = '-';
				k++;
			}
		}
	}

        if (id->RadioCall[k-1] == '-')
        {
                id->RadioCall[k-1] = 0x00;
                id->qsy_sw = TRUE;
        }

	if (id->RadioCall[k-1] == '-') id->RadioCall[k-1] = 0x00;

	if (k < 7) id->RadioCall[k] = 0x00;

	memset (GpsMsg, 0x00, 20);
	len = strlen ((char *)&id->aprs_msg[9]);
	if (len <= 0) return;
	if (len > 20) len = 20;
	memcpy (GpsMsg, &id->aprs_msg[9], len);
}

void	Dprs_Send(struct aprs_msg *id)
{
	extern	int	aprs_sd;
	extern	struct	sockaddr_in aprs_addr;

	unsigned char	call_id[10];
	unsigned char	call_radio_id[10];
	int	msg_len;
	int	l, n, m;
	int	k, i;
	int	posit = 0;
	time_t	atime;
	char	temp[256];
	char	tmp[10];
	double	f_temp;

	if (aprs_status < 0) return;

	memset (call_radio_id, 0x20, 10);
	for (i = 0 ; i < 8 ; i++)
	{
		if (id->RadioCall[i] == 0x00) break;
		call_radio_id[i] = id->RadioCall[i];
	}
	if (call_radio_id[0] == 0x20)
	{
		id->AprsSend = APRS_INVALID;
		memcpy (id->aprs_msg_save, id->aprs_msg, 256);
		return;
	}
        memset (tmp, 0x00, 10);
        memcpy (tmp, id->RadioLat, 8);
        f_temp = atof (&tmp[2]) / 60.;
        tmp[2] = 0x00;
        STATUS_Frm.body.status.Latitude = atoi (tmp) * 10000 + f_temp * 10000;
        if (id->RadioLat[7] == 'S') STATUS_Frm.body.status.Latitude = - STATUS_Frm.body.status.Latitude;

        memcpy (tmp, id->RadioLong, 9);
        f_temp = atof (&tmp[3]) / 60.;
        tmp[3] = 0x00;
        STATUS_Frm.body.status.Longitude = atoi (tmp) * 10000 + f_temp * 10000;
        if (id->RadioLong[8] == 'W') STATUS_Frm.body.status.Longitude = - STATUS_Frm.body.status.Longitude;

	if (!verify_sw) return;

	k = 0;
	for (i = 0 ; i < 7 ; i++)
	{
		if (client_callsign[i] != 0x20)
		{
			call_id[k] = client_callsign[i];
			k++;
		}
		else
		{
			if (k > 0)
			{
				if(call_id[k-1] != '-')
				{
					call_id[k] = '-';
					k++;
				}
			}
		}
	}
	call_id[k++] = radio_id;
	call_id[k] = 0x00;
 
	msg_len = 4;
	while (GpsMsg[msg_len] != '*') msg_len++;
	m = msg_len;
	for (i = 1 ; i <= m ; i++)
	{
		if (GpsMsg[m - i]  != 0x20) break;
		msg_len--;
	}
	msg_len -= 4;
	if (msg_len < 0) msg_len = 0;
	n = 0;
	l = 9999;
	while (memcmp(aprs_symbol[n][1], "    ", 4))
	{
		if (!memcmp(aprs_symbol[n][1], GpsMsg, 4))
		{
			l = n;
			break;
		}
		n++;
	}
	if (l == 9999) l = 29;	// car
	m = strlen (&id->RadioCall[0]);
	if (m > 8) m = 8;
	memcpy (temp, &id->RadioCall[0], m);
	posit += m;
	memcpy (&temp[posit], ">APDVAP,DSTAR*,qAR,", 19);
	posit += 19;
	memcpy (&temp[posit], call_id, k);
	posit += k;
	temp[posit++] = ':';
	//if (msg_len) temp[posit++] = '=';
	//else temp[posit++] = '!';
	if (msg_len) temp[posit++] = '@';
	else temp[posit++] = '/';
	memcpy (&temp[posit], &id->RadioTime, 7);
	posit += 7;
	memcpy (&temp[posit], &id->RadioLat[0], 8);
	posit += 8;
	temp[posit++] =  aprs_symbol[l][0][0];
	memcpy (&temp[posit] , &id->RadioLong[0], 9);
	posit += 9;
	temp[posit++] = aprs_symbol[l][0][1];
	if (id->RadioDirection[0] != 0x00)
		memcpy (&temp[posit], &id->RadioDirection[0], 3);
	else
		memcpy (&temp[posit], "000", 3);
	posit += 3;
	temp[posit++] = '/';
	if (id->RadioSpeed[0] != 0x00)
		memcpy (&temp[posit], &id->RadioSpeed[0], 3);
	else
		memcpy (&temp[posit], "000", 3);
	posit += 3;
	memcpy (&temp[posit], "/A=", 3);
	posit += 3;
	if (id->RadioAtitude[0] != 0x00)
		memcpy (&temp[posit], &id->RadioAtitude[0], 6);
	else
		memcpy (&temp[posit], "000000", 6);
	posit += 6;
	// QSY
        if (id->qsy_sw && qsy_info)
        {
                memcpy (&temp[posit], "D-STAR>", 7);
                posit += 7;
                if (node_area_rep_callsign[0] != 0x20)
                {
                	memcpy (&temp[posit], node_area_rep_callsign, 8);
                } else {
                        memcpy (&temp[posit], dvap_area_rep_callsign, 8);
                }
                posit += 8;
        }
	if (msg_len > 0)
	{
		memcpy (&temp[posit], &GpsMsg[4], msg_len);
		posit += msg_len;
	}
	memcpy (&temp[posit], "\r\n", 2);
	posit += 2;
	memcpy (id->aprs_msg_save, temp, posit);
	temp[posit] = 0x00;
        if (!send_check(call_radio_id))
        {
                id->AprsSend = APRS_SHORT;
                return;
        }
	send (aprs_sd, temp, posit, 0);
	id->AprsSend = APRS_SEND;
	aprs_cnt++;
	time(&atime);
	fprintf (log_file, "%24.24s send : %s\n", ctime(&atime), temp); 
	fflush (log_file);
	memset (&id->RadioDirection[0], 0x00, 3);
	memset (&id->RadioSpeed[0], 0x00, 3);
	memset (&id->RadioAtitude[0], 0x00, 6);
	id->RadioGpsStatus = FALSE;
}

void    gps (struct aprs_msg *msg)
{
	time_t	atime;

	if (debug)
	{
		fprintf (log_file, "gps: %s", msg->aprs_msg);
		fflush (log_file);
	}

        if (!GPS_SumCheck(msg->aprs_msg))
	{
		msg->AprsSend = APRS_ERROR;
		memcpy (msg->aprs_msg_save, msg->aprs_msg, 256);
		time(&atime);
		fprintf (log_file, "%24.24s Check Sum error : %s", ctime(&atime), msg->aprs_msg);
		fflush (log_file);
		return;
	}
        if (!strncmp (msg->aprs_msg, "$GPGLL,", 7)) GPGLL (msg);
        else if (!strncmp(msg->aprs_msg, "$GPGGA,", 7)) GPGGA (msg);
        else if (!strncmp(msg->aprs_msg, "$GPRMC,", 7)) GPRMC (msg);
        else if (!strncmp(msg->aprs_msg, "$GPVTG,", 7)) GPVTG (msg);
        //else if (!strncmp(&msg->aprs_msg, "$GPGSA,", 7)) GPGSA (msg);
        //else if (!strncmp(&msg->aprs_msg, "$GPGSV,", 7)) GPGSV (msg);
        else
        {
		dprs_message(msg);
               	Dprs_Send(msg);
        }

}


#include "dv_ap.h"

int        send_check(char call_sign[]);
unsigned short int        update_crc_dstar( unsigned short int  crc, unsigned char c );
unsigned short int                result_crc_dstar(unsigned short int crc);

extern	int	aprs_sd;
extern	struct	sockaddr_in dprs_addr;
extern	int	aprs_status;

int	GPS_A_CRC(unsigned char  string[])
{
	unsigned short int	crc_dstar_ffff, k, k0, k1, k2, k3;
	unsigned char	*pnt;
	
    	crc_dstar_ffff = 0xffff;	/* nornal value 0xffff */
	pnt = string + 10;

	while (*pnt != 0x0a)
	{
		crc_dstar_ffff = update_crc_dstar( crc_dstar_ffff, *pnt);
		pnt++;
	}

	crc_dstar_ffff = result_crc_dstar(crc_dstar_ffff);

	k0 = string[5] - '0';
	if (k0 > 16) k0 -= 7;
	k1 = string[6] - '0';
	if (k1 > 16) k1 -= 7;
	k2 = string[7] - '0';
	if (k2 > 16) k2 -= 7;
	k3 = string[8] - '0';
	if (k3 > 16) k3 -= 7;
	k1 += k0 * 16;
	k3 += k2 * 16;
    	k = k1 | (k3 << 8);

	if (k == crc_dstar_ffff) return TRUE;
	return	FALSE;
}

void	GPS_A_Send(unsigned char string[], struct aprs_msg *id)
{
	unsigned char	CallSign[10];
	int	i;
	int	tmp;
	int	len;
	int	k;
	int	n;
	char	temp[20];
	char	string_temp[256];

	id->qsy_sw = TRUE;
	memset (CallSign, 0x20, 10);
	for (i = 0 ; i < 9 ; i++)
	{
		if (string[i] == '>') break;
		CallSign[i] = string[i];
	}
	if (CallSign[0] == 0x20)
	{
		id->AprsSend = APRS_INVALID;
		memcpy (id->aprs_msg_save, id->aprs_msg, 256);
		return;	
	}
        for (i = 0 ; i < 10 ; i++)
        {
                if (string[i] == '-')
                {
                        if (isalpha(string[i+1]))
                        {
                                id->qsy_sw = FALSE;
                                break;
                        }
                }
        }

        len = strlen((char *)string);
	memset (string_temp, 0x00, 256);
	memcpy (string_temp, string, len);
        for (i = 0 ; i < len ; i++)
        {
                if ((string[i] == '!') || (string[i] == 'z') || (string[i] == 'h')
                        || (string[i] == '='))
                {
                        memset (temp, 0x00, 20);
                        n = 0;
                        for (k = i+1 ; k < len ; k++)
                        {
                                temp[n] = string[k];
                                //if ((string[k] == '/') || (string[k] == '\\'))
                                if (!(isdigit(string[k]) || (string[k] == '.')
                                        || (string[k] == 'S') || (string[k] == 'N')))
                                {
                                        temp[n] = 0x00;
                                        tmp = atof (&temp[2]) * 10000. / 60.;
                                        temp[2] = 0x00;
                                        tmp = tmp + (atoi(temp) * 10000);
                                        if (temp[7] == 'S') tmp = - tmp;
                                        STATUS_Frm.body.status.Latitude = tmp;
                                        break;
                                }
                                n++;
                        }
                        if (isdigit(string[k]))         // IC-2820 bug
                        {
                                id->AprsSend = APRS_INVALID;
                                return;
                        }
                        k++;
                        memset (temp, 0x00, 20);
                        n = 0;
                        for ( ; k < len ; k++)
                        {
                                temp[n] = string[k];
                                //if ((string[k] == '[') || (string[k] == '-') || (string[k] == '>'))
                                if (!(isdigit(string[k]) || (string[k] == '.')
                                        || (string[k] == 'W') || (string[k] == 'E')))
                                {
                                        temp[n] = 0x00;
                                        tmp = atof (&temp[3]) * 10000. / 60.;
                                        temp[3] = 0x00;
                                        tmp = tmp + (atoi(temp) * 10000);
                                        if (temp[8] == 'W') tmp = - tmp;
                                        STATUS_Frm.body.status.Longitude = tmp;
                                        break;
                                }
                                n++;
                        }
			k++;
                        if (id->qsy_sw && qsy_info)
                        {
				if (isdigit(string[k])) k += 7;
                                if (string[k] == '/')
				{
					if (!memcmp (&string[k+1], "A=", 2)) k += 9;
					else k++;
				}
                                if (len > k) memcpy (&string_temp[k+15], &string[k], len - k);
                                memcpy (&string_temp[k], "D-STAR>", 7);
        			if (node_area_rep_callsign[0] != 0x20)
        			{
                			memcpy (&string_temp[k+7], node_area_rep_callsign, 8);
        			} else {
                			memcpy (&string_temp[k+7], dvap_area_rep_callsign, 8);
        			}
                        }
                        break;
                }
        }
	if (!verify_sw) return;

	memcpy (id->aprs_msg_save, id->aprs_msg, 256);
	if (send_check(CallSign))
	{
		send (aprs_sd, string, strlen(string), 0);
		time(&cur_time);
		fprintf (log_file, "%24.24s send : %s\n", ctime(&cur_time), string);
		fflush (log_file);
		id->AprsSend =  APRS_SEND;
		aprs_cnt++;
	}
	else
	{
		id->AprsSend = APRS_SHORT;
	}
}

void    gps_a (struct aprs_msg *msg)
{
	time_t	atime;

	if (aprs_status < 0) return;

	time(&atime);
        if (debug)
	{
		fprintf (log_file, "%24.24s gps_a: %s", ctime(&atime), msg->aprs_msg);
		fflush (log_file);
	}
        if (!GPS_A_CRC(&msg->aprs_msg[0]))
	{
		msg->AprsSend = APRS_ERROR;
		memcpy (msg->aprs_msg_save, msg->aprs_msg, 256);
		fprintf (log_file, "%24.24s CRC error : %s", ctime(&atime), msg->aprs_msg);
		fflush (log_file);
		return;
	}

	GPS_A_Send (&msg->aprs_msg[10], msg);
}


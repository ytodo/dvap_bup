#include	"dv_ap.h"

void BeaconSend(void)
{
	char	id[10];
	int	comment_len;
	int	i;
	int	k;
	int	pnt, save_pnt;
	time_t	atime;

	unsigned char	RadioLat[8], RadioLong[9];

	i = abs(BeaconLat) % 10000;
	i = i * 6 + 5;
	i /= 10;
	sprintf ((char *)RadioLat, "%02d%02d.%02d" ,abs(BeaconLat)/10000, i/100, i%100);
	if (BeaconLat >= 0) RadioLat[7] = 'N';
	else RadioLat[7] = 'S';

	i = abs(BeaconLong) % 10000;
	i = i * 6 + 5;
	i /= 10;
	sprintf ((char *)RadioLong, "%03d%02d.%02d",abs(BeaconLong)/10000, i/100, i%100);
	if (BeaconLong >= 0) RadioLong[8] = 'E';
	else RadioLong[8] = 'W';
    
	k = 0;
	for (i = 0 ; i < 7 ; i++)
	{
		if (client_callsign[i] != 0x20)
		{
			id[k] = client_callsign[i];
			k++;
		}
		else
		{
			if (k > 0)
			{
				if(id[k-1] != '-')
				{
					id[k] = '-';
					k++;
				}
			}
		}
	}
	id[k++] = radio_id;
	id[k] = 0x00;

	comment_len = strlen(beacon_comment);
	if (comment_len > 63) comment_len = 63;
	pnt = 0;
	memcpy (&DprsTemp[pnt], id, k);
	pnt += k;
	memcpy (&DprsTemp[pnt], ">APDVAP,TCPIP*,qAC,",19);
	pnt += 19;
	memcpy (&DprsTemp[pnt], id, k);
	pnt += k;
	DprsTemp[pnt++] = 'S';
	memcpy (&DprsTemp[pnt], ":!", 2);
	save_pnt = pnt + 1;
	pnt += 2;
	memcpy (&DprsTemp[pnt], RadioLat, 8);
	pnt += 8;
	DprsTemp[pnt++] = 'D';
	memcpy (&DprsTemp[pnt], RadioLong, 9);
	pnt += 9;
	DprsTemp[pnt++] = '&';
	memcpy (&DprsTemp[pnt], beacon_comment, comment_len);
	pnt += comment_len;
	memcpy (&DprsTemp[pnt], "\r\n", 2);
	pnt += 2;
	send (aprs_sd, DprsTemp, pnt, 0);
	memcpy (&DprsTemp[save_pnt], "<IGATE,MSG_CNT=0,LOC_CNT=1\r\n", 28);
	save_pnt += 28;
	send (aprs_sd, DprsTemp, save_pnt, 0);
	BeaconTime += BeaconInterval;
	aprs_beacon_cnt++;
	if (debug)
	{
		time (&atime);
		fprintf (log_file, "%24.24s Beacon Sent\n", ctime(&atime));
		fflush (log_file);
	}
}

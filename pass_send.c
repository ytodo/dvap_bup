#include	"dv_ap.h"

int     gen_passcode(unsigned char callsign[]);

void	AprsUserPassSend (void)
{
	int	k, i, n;
	unsigned char call[8];

		memset (call, 0x00, 8);
		memcpy (DprsTemp, "user ", 5);
		k = 5;
		n = 0;
		for (i = 0 ; i < 7 ; i++)
		{
			if (client_callsign[i] != 0x20)
			{
				DprsTemp[k++] = client_callsign[i];
				call[n++] = client_callsign[i];
			}
			else
			{
				if (k > 0)
				{
					if(DprsTemp[k-1] != '-')
					{
						DprsTemp[k] = '-';
						k++;
					}
				}
			}
		}
		DprsTemp[k++] = radio_id;
		DprsTemp[k++] = 'S';
	
		memcpy (&DprsTemp[k], " pass ", 6);
		k += 6;
		sprintf (call, "%05d", gen_passcode(call));
		memcpy (&DprsTemp[k], call, 5);
		k += 5;
		memcpy (&DprsTemp[k], " vers DV_AP " , 12);
		k += 12;
		memcpy (&DprsTemp[k], VERSION, 5);
		k += 5;
		DprsTemp[k++] = '-';
		memcpy (&DprsTemp[k], uname_buf.machine, strlen(uname_buf.machine));
		k += strlen (uname_buf.machine);
		memcpy (&DprsTemp[k], "\r\n", 2);
		k += 2;
		send (aprs_sd, DprsTemp, k, 0); 
}


int	gen_passcode(unsigned char callsign[])
{
	int	hash;
	int	i;

	i = 0;
	hash = 0x73e2;

	while (callsign[i])
	{
		hash ^= (callsign[i] & 0xff) << 8;
		hash ^= (callsign[i+1] & 0xff);
		i += 2;
	}
	return  hash & 0x7fff;
}	


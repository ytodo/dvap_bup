#include	"dv_ap.h"

void	ReConnectSet(void);
void	AprsUserPassSend (void);
void	BeaconSend (void);
void	aprs2dstar (char string[]);

enum
{
	DPRS_OPEN_AUTOLINK_START = 0,
	DPRS_OPEN_AUTOLINK,
	DPRS_OPEN_AUTOLINK_WAIT,
	DPRS_OPEN_DONE_AUTOLINK,
	DPRS_ACCEPT,
	DPRS_LOOP,
	DPRS_NG
} DprsState = DPRS_OPEN_AUTOLINK_START;

time_t	AprsKeepAlive;
time_t	Timer;
time_t	AprsConnectTime;
time_t	AprsVerifyTime;

int     ReDprsConnectCnt = 0;
int     ReDprsConnectCount = 0;
int	aprs_status;
int	aprs_loop;

int	aprs (void)
{
	struct	addrinfo	hints, *res;
	int			err;

	time_t	atime;
	int	len;
	unsigned char	*tp;
	int	i;


	switch (DprsState)
	{
		case DPRS_LOOP:
               		time(&atime);
               		if (FD_ISSET (aprs_sd, &rfds))      // from aprs server
               		{
                       		len = recvfrom (aprs_sd, DprsTemp, sizeof(DprsTemp),
                              			 0, NULL, NULL);
                       		if (len <= 0) 
				{
					ReConnectSet();
				}
                       		else
                       		{
                               		DprsTemp[len] = 0x00;
                                        if (debug)
                                        {
                                                fprintf (log_file, "%24.24s %s", ctime(&atime), DprsTemp);
                                                fflush (log_file);
                                        }
                               		if (!memcmp (DprsTemp, "# logresp", 9))
                               		{
                                       		tp = strtok (DprsTemp, " \r\n\0");
						tp = strtok (NULL, " \r\n\0");
						tp = strtok (NULL, " \r\n\0");
						tp = strtok (NULL, " \r\n\0");		// "verified." or "unverified"
						verify_sw = FALSE;
						if (!memcmp(tp, "verified", 8)) verify_sw = TRUE;
						tp = strtok (NULL, " \r\n\0");		// "server"
						tp = strtok (NULL, " \r\n\0");		// server name
                                       		memcpy (aprs_srv , tp, strlen(tp));
						time (&atime);
						if (verify_sw)
							fprintf (log_file, "%24.24s APRS Server (%s) Verified.\n", ctime(&atime), aprs_srv);
						else
							fprintf (log_file, "%24.24s APRS Server (%s) Unverified.\n", ctime(&atime), aprs_srv);
						fflush (log_file);
                               		}
					else
					{
						if (aprs_rf_send) aprs2dstar (DprsTemp);
					}
                               		time(&AprsKeepAlive);
                               		AprsKeepAlive += 25;
                       		}
               		}
			else
			{
				if (atime >= AprsKeepAlive)
				{
					ReConnectSet();
					break;
				}
			}
                       	if (BeaconInterval && verify_sw)
			{
                       		if (atime >= BeaconTime) BeaconSend();
			
                        }
			break;

		case DPRS_NG:
			break;

		case DPRS_OPEN_AUTOLINK_START:
			DprsState = DPRS_OPEN_AUTOLINK;
			ReDprsConnectCnt = 0;
			aprs_status = -1;
			memcpy (aprs_srv, "NONE\0", 5);
			verify_sw = FALSE;
			ReDprsConnectCnt = 0;
			break;

   		case DPRS_OPEN_AUTOLINK:
			if(aprs_status < 0)
			{
                        	memset (&hints, 0x00, sizeof(hints));
                        	hints.ai_socktype = SOCK_STREAM;
                        	hints.ai_family = PF_UNSPEC;
				sprintf (PORT, "%d", aprs_port);
                        	if ((err = getaddrinfo (aprs_server, PORT, &hints, &aprs_sock)) != 0)
                        	{
					time(&atime);
                                	fprintf (log_file, "%24.24s getaddrinfo error(APRS Server) %s\n", ctime(&atime), gai_strerror(err));
					fprintf (log_file, "%24.24s %s\n", ctime(&atime), aprs_server);
                                	fflush (log_file);
					if (err == EAI_AGAIN)
					{
						close (aprs_sd);
						return TRUE;
					}
					if (AutoReLink)
					{
						DprsState = DPRS_OPEN_AUTOLINK_WAIT;
						time(&Timer);
						return TRUE;
					}
					return FALSE;
                        	}
                                err = getnameinfo (aprs_sock->ai_addr, aprs_sock->ai_addrlen,
                                        aprs_ip, sizeof(aprs_ip), NULL, 0,
                                        NI_NUMERICHOST | NI_NUMERICSERV);
                                if (err != 0)
                                {
                                        fprintf (log_file, "%24.24s getnameinfo error : %s\n",
                                                ctime(&atime), gai_strerror(err));
                                        fflush (log_file);
                                }
                                if((aprs_sd = socket(aprs_sock->ai_family, aprs_sock->ai_socktype, aprs_sock->ai_protocol)) < 0)
                                {
                                        time (&atime);
                                        fprintf (log_file, "%24.24s D-PRS TCP socket not open\n", ctime(&atime));
                                        fflush (log_file);
                                        return FALSE;
                                }

				aprs_status = connect (aprs_sd, aprs_sock->ai_addr, aprs_sock->ai_addrlen);
				if (aprs_status < 0)
				{
					time (&atime);
					fprintf (log_file, "%24.24s connect error %s\n", ctime(&atime), strerror(errno));
					fflush (log_file);
					time(&atime);
					if((atime - Timer) > 3)	/* wait 3 seconds */
					{
						if (ReDprsConnectCnt++ > 5)
						{
							DprsState = DPRS_OPEN_AUTOLINK_START;
							aprs_status = -1;
							ReDprsConnectCnt = 0;
							break;
						}
						time(&Timer);;
						break;
					}
					break;
				}
			}
			FD_SET (aprs_sd, &save_rfds);
			DprsState = DPRS_OPEN_DONE_AUTOLINK;
			ReDprsConnectCnt = 0;
			time(&Timer);
			break;

		case DPRS_OPEN_AUTOLINK_WAIT:
			time (&atime);
			if ((atime - Timer) > 30)
			{
				if (ReDprsConnectCnt++ > 20) 
				{
					fprintf (log_file, "%24.24s autolink max wait error\n", ctime(&atime));
					fflush (log_file);
					return FALSE;
				}
				DprsState = DPRS_OPEN_AUTOLINK;
			}
			break;

   		case DPRS_OPEN_DONE_AUTOLINK:
               		if (FD_ISSET (aprs_sd, &rfds))
			{
				len = recvfrom (aprs_sd, DprsTemp, sizeof(DprsTemp), 0, NULL, NULL);
				if (len > 2)
				{
					if (!memcmp(DprsTemp, "# ", 2))
					{	
						DprsState = DPRS_ACCEPT;
						time(&AprsConnectTime);
					}
					break;
				}
			}
			time(&atime);
			if((atime - Timer) > 5)	/* wait 5 seconds */
			{
				ReDprsConnectCnt++;
				if (ReDprsConnectCnt > 60)		// 5 minutes
				{
					fprintf (log_file, "%24.24s APRS Server Down\n", ctime(&atime));
					fflush (log_file);
					return FALSE;
				}
				time(&Timer);
				DprsState = DPRS_OPEN_AUTOLINK_START;
				close (aprs_sd);
			}
			break;

		case DPRS_ACCEPT:
			time (&atime);
			fprintf (log_file, "%24.24s APRS Server Connected\n", ctime(&atime));
			fflush (log_file);
			AprsUserPassSend();
			DprsState = DPRS_LOOP;
			ReDprsConnectCnt = 0;
			time(&AprsKeepAlive);
			AprsKeepAlive += 25;
			time(&AprsVerifyTime);
			AprsVerifyTime += 10;
			break;
      	}
	return TRUE;
}

void	ReConnectSet (void)
{
        time_t  atime;
        int i;

	if (AutoReLink)
	{
        	DprsState = DPRS_OPEN_AUTOLINK_START;
	}
	else
	{
		DprsState = DPRS_NG;
	}
        FD_CLR (aprs_sd, &save_rfds);
       	close (aprs_sd);
	aprs_sd = 0;
       	time(&atime);
       	fprintf (log_file, "%24.24s APRS Server (%s) Disconnected\n", ctime(&atime), aprs_server);
        fflush (log_file);
	verify_sw = FALSE;
        aprs_status = -1;
}

void	aprs_link_start(void)
{
	time_t	atime;

	DprsState = DPRS_OPEN_AUTOLINK_START;
	if (aprs_sd)
	{
        	FD_CLR (aprs_sd, &save_rfds);
        	close (aprs_sd);
	}
        time(&atime);
	fprintf (log_file, "%24.24s APRS Server (%s) Disconnected\n", ctime(&atime), aprs_server);
        fprintf (log_file, "%24.24s APRS Server ReConnect Start\n", ctime(&atime));
        fflush (log_file);
        aprs_status = -1;
}

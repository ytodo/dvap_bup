#include	"dv_ap.h"

void    status_init (char Fullname[], int port);

int	read_config(int argc, char **argv)
{
	struct	status	*status_next;
	char	buff[256];
	char	*delmi = "=\n\r\t\0";
	char	*pnt;
	char	*p;
	int	n;
	int	port;
	int	len;
	int	k;
	int	port_sw;
	char	fqdn[128];
	int	field_sw;

	FILE	*config_file;

	if (argc == 2)
	{
		config_file = fopen (argv[1], "r");
		if (!config_file)
		{
			time(&cur_time);
			fprintf (log_file, "%24.24s config file not found (%s)\n", ctime(&cur_time), argv[1]);
			fflush (log_file);
			return FALSE;
		}
	} 
	else
	{
		config_file = fopen (CONFIG_FILE,"r");
		if (!config_file)
		{
			time(&cur_time);
			fprintf (log_file, "%24.24s config file not found (%s)\n", 
						ctime(&cur_time), CONFIG_FILE);
			fflush (log_file);
			return FALSE;
		}
	}

	while (fgets (buff, 255, config_file))
	{
		if (buff[0] != '#')
		{
			n = 0;
			while (buff[n] == 0x20) n++;
			p = strtok(&buff[n], delmi);
			if (p != NULL)
			{
				pnt = strtok(NULL, delmi);
	
				if (!memcmp (p, "GATEWAY_CALL", 12))
				{
                                        memset (gateway_callsign, 0x20, 8);
                                        memcpy (gateway_callsign, pnt, strlen(pnt));
					gateway_callsign[7] = 0x20;
				}
				else if (!memcmp (p, "KEEP_ALIVE_INTERVAL", 19))
				{
					keep_alive_interval = atoi (pnt);
					if (keep_alive_interval > 0)
					{
						if (keep_alive_interval <= 10) keep_alive_interval = 60;
					}
				}
				else if (!memcmp (p, "DEBUG", 5))
				{
					debug = atoi (pnt);
				}
                                else if (!memcmp (p, "TRUST_SERVER", 12))
                                {
                                        memset (trust_name, 0x00, 128);
					memcpy (trust_name, pnt, strlen(pnt));
                                }
				else if (!memcmp (p, "NODE_ADAPTER",12))
				{
					pnt = strtok(pnt, ":");
					VenderID = htoi (pnt);
					pnt = strtok(NULL, delmi);
					ProductID = htoi(pnt);
				}
				else if (!memcmp (p, "NODE_CALL", 9))
				{
					memset (node_area_rep_callsign, 0x20, 8);
					memcpy (node_area_rep_callsign, pnt, strlen(pnt));
				}
				else if (!memcmp (p, "DVAP_TX_FREQUENCY",  17))
				{
					dvap_tx_freq = atoi(pnt);
				}
				else if (!memcmp (p, "DVAP_RX_FREQUENCY",  17))
				{
					dvap_rx_freq = atoi (pnt);
				}
				else if (!memcmp (p, "DVAP_FREQUENCY", 14))
				{
					dvap_freq = atoi (pnt);
				}
				else if (!memcmp (p, "DVAP_SQUELCH", 12))
				{
					dvap_squelch = atoi (pnt);
					if ((dvap_squelch > -45) || (dvap_squelch < -128)) dvap_squelch = 0;
				}
				else if (!memcmp (p, "DVAP_CALIBRATION",  16))
				{
					dvap_calibration = atoi (pnt);
					if ((dvap_calibration < -2000) || (dvap_calibration > 2000)) dvap_calibration = 0;
				}	
				else if (!memcmp (p, "DVAP_CALL", 9))
				{
					memset (dvap_area_rep_callsign, 0x20, 8);
					memcpy (dvap_area_rep_callsign, pnt, strlen(pnt));	
				}
                                else if (!memcmp (p, "DVAP_AUTO_CALIBRATION", 21))
                                {
                                        dvap_auto_calibration = atoi (pnt);
                                }
				else if (!memcmp (p, "IDXXPLUS_CALL", 13))
				{
					memset (IDxxPlus_area_rep_callsign, 0x20, 8);
					memcpy (IDxxPlus_area_rep_callsign, pnt, strlen(pnt));
				}
                                else if (!memcmp (p, "APRS_SERVER", 11))
                                {
                                        n = 0;
                                        port_sw = 0;
                                        port = 0;
                                        memset (aprs_server, 0x20, 128);
                                        while (*pnt)
                                        {
                                                if (port_sw)
                                                {
                                                        if (isdigit (*pnt)) port = port * 10 + *pnt - '0';
                                                }
                                                else
                                                {
                                                        if (*pnt == ':') port_sw = 1;
                                                        else
                                                        {
                                                                if (*pnt != 0x20) aprs_server[n++] = *pnt;
                                                                if (n > 127) n = 127;
                                                        }
                                                }
                                                pnt++;
                                        }
                                        if (n && (port > 0))
                                        {
                                                aprs_server[n] = 0x00;
                                                aprs_port = port;
                                        }
                                        else
                                        {
                                                time (&cur_time);
                                                fprintf (log_file, "%24.24s Invalid Format of APRS_SERVER\n",
                                                        ctime(&cur_time));
                                                fflush (log_file);
                                                aprs_port = 0;;
                                        }
                                }
                                else if (!memcmp (p, "BEACON_LAT", 10))
                                {
                                        k = 0;
                                        while (*pnt != '.')
                                        {
                                                k *= 10;
                                                k += (*pnt - '0');
                                                pnt++;
                                        }
                                        BeaconLat = k * 10000;
                                        pnt++;
                                        BeaconLat += atoi (pnt);
                                }
                                else if (!memcmp (p, "BEACON_LONG", 11))
                                {
                                        k = 0;
                                        while (*pnt != '.')
                                        {
                                                k *= 10;
                                                k += (*pnt - '0');
                                                pnt++;
                                        }
                                        BeaconLong = k * 10000;
                                        pnt++;
                                        BeaconLong += atoi (pnt);
                                }
                                else if (!memcmp (p, "BEACON_INTERVAL", 11))
                                {
                                        BeaconInterval = atoi (pnt);
                                }
                                else if (!memcmp (p, "BEACON_COMMENT", 14))
                                {
                                         memset (beacon_comment, 0x00, 64);
                                        len = strlen(pnt);
                                        if (len > 63) len = 63;
                                        memcpy (beacon_comment, pnt, len);
                                }
                                else if (!memcmp (p, "RADIO_ID", 8))
                                {
                                        radio_id = *pnt;
                                }
                                else if (!memcmp (p, "APRS_CALLSIGN", 13))
                                {
                                        memset (client_callsign, 0x20, 7);
                                        len = strlen(pnt);
                                        if (len > 7) len = 7;
                                        memcpy (client_callsign, pnt, len);
                                }
				else if (!memcmp (p, "APRS_FILTER", 11))
				{
					memcpy (aprs_filter, pnt, strlen(pnt));
				}
				else if (!memcmp (p, "APRS_RF_SEND", 12))
				{
					aprs_rf_send = atoi (pnt);
				}
                                else if (!memcmp (p, "AUTO_RELINK", 11))
                                {
                                        AutoReLink = atoi (pnt);                
                                }
                                else if (!memcmp (p, "SEND_INTERVAL", 13))
                                {
                                        aprs_send_interval =  atoi (pnt);
                                }
				else if (!memcmp (p, "UPNP", 4))
				{
					upnp_sw = atoi (pnt);
				}
				else if (!memcmp (p, "TRUST_TIMEOUT", 13))
				{
					trust_timeout = atoi (pnt);
				}
                                else if (!memcmp (p, "NIC", 3))
                                {
                                        memset (NicDevice, 0x00, IFNAMSIZ);
                                        memcpy (NicDevice, pnt, strlen(pnt));
                                }
				else if (!memcmp (p, "HTTP_PORT", 9))
				{
					http_port = atoi (pnt);
				}
                                else if (!memcmp (p, "STATUS", 7))
                                {
                                        n = 0;
                                        field_sw = 0;
                                        port = 0;
                                        memset (fqdn, 0x20, 128);
                                        while (*pnt)
                                        {
                                        	if (field_sw)
                                                {
                                                	if (isdigit (*pnt)) port = port * 10 + *pnt - '0';
                                                }
                                                else
                                                {
                                               		if (*pnt == ':') field_sw = 1;
                                                        else
                                                        {
                                                        	if (*pnt != 0x20) fqdn[n++] = *pnt;
                                                                if (n > 127) n = 127;
                                                        }
                                               	}
                                               	pnt++;
                                        }
                                        fqdn[n] = 0x00;
                                        status_init(fqdn, port);
                                }
                                else if (!memcmp (p, "ECHO_SERVER", 11))
                                {
                                        memset (echo_server, 0x20, 8);
                                        memset (echo_area_rep_callsign, 0x20, 8);
                                        pnt = strtok(pnt, ":");
                                        memcpy (echo_area_rep_callsign, pnt, 8);
                                        pnt = strtok(NULL, delmi);
                                        memcpy (echo_server, pnt, 8);
                                }
                                else if (!memcmp (p, "ECHO_POSITION_SEND_INTERVAL",  27))
                                {
                                         echo_position_send_interval = atoi(pnt);
                                }
				else
				{
					time (&cur_time);
					fprintf (log_file, "%24.24s Error on config file : %s\n",ctime(&cur_time), buff);
					fflush (log_file);
				}
			}
		}
	}

	fclose (config_file);

	return TRUE;
}

void    status_init (char FullName[], int port)
{
        dv_status.port = port;
        memcpy (dv_status.fqdn, FullName, 128);
        dv_status.packets = 0;
        memset (dv_status.userID, 0x00, 16);
        memset (dv_status.passwd, 0x00, 64);
        dv_status.status_info = NULL;
}


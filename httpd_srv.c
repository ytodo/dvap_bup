#include	<openssl/md5.h>
#include	<sys/stat.h>
#include	"dv_ap.h"

void	DateSend(int sock);

extern	time_t	AprsConnectTime;
extern	char	dvap_header[];
extern	char	dvap_inet_header[];
extern	char	node_inet_header[];

char    *wday[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char    *month[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
char	http_temp[512];

void	httpd_srv_accept(void)
{
	int	sock;
	int	yes = 1;
	time_t	atime;

	http_recv_len = sizeof(http_recv);
	sock = accept(httpd_sd, (struct sockaddr *)&http_recv, &http_recv_len);
	if (sock < 0)
	{
		time(&atime);
		fprintf (log_file, "%24.24s %s\n", ctime(&atime), strerror(errno));
		fflush (log_file);
		FD_CLR (httpd_sd, &save_rfds);
		close (httpd_sd);
		httpd_sd = 0;
		http_port= 0;
	}
	else
	{
		FD_SET (sock, &save_rfds);
	}
}

void	NotFound (int sock)
{
	send (sock, "HTTP/1.1 404 Not Found\r\n", 24, 0);
	DateSend(sock);
	send (sock, "Content-Type: text/html\r\n", 25, 0);
	send (sock, "Content-length: 248\r\n\r\n", 23, 0);
	send (sock, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/html4/strict.dtd\">\r\n", 94, 0);
	send (sock, "<html><head><title>404 Not Found</title></head>",  47, 0); 
	send (sock, "<body><h1>Not Found</h1><p>The requested URL /favicon.ico was not found on this server.</p></body></html>", 107, 0);
}

void	NotModify (int sock)
{
	send (sock, "HTTP/1.1 304 Not Modified\r\nConnection: close\r\n\r\n", 50, 0);
	//DateSend (sock);
	//send (sock, "\r\n", 2, 0);
}

void	ETagSend (int sock, char tag[])
{
        send (sock, "ETag: \"", 7, 0);
        send (sock, tag, 32, 0);
        send (sock, "\"\r\n", 3, 0);
}

void	LastModSend (int sock, time_t mod_time)
{
        gt = gmtime(&mod_time);
        sprintf (http_temp, "Last-Modified: %3.3s, %2.2d %3.3s %4.4d %02d:%02d:%02d GMT\r\n",
                        wday[gt->tm_wday],
                        gt->tm_mday,
                        month[gt->tm_mon],
                        gt->tm_year+1900,
                        gt->tm_hour,
                        gt->tm_min,
                        gt->tm_sec);
        send (sock, http_temp, strlen(http_temp), 0);
}

time_t	ModifiedTime (char string[])
{
	time_t	req_time;
	char	*tok;
	int	mon;

	tok = string;
	tok = strtok (tok, " :\0");
	gt->tm_mday = atoi(tok);
	tok = strtok (NULL, " :\0");
	for (mon = 0 ; mon < 12 ; mon++)
	{
		if (!memcmp (tok, month[mon], 3)) break;
	}
	gt->tm_mon = mon;
	tok = strtok (NULL, " :\0");
	gt->tm_year = atoi (tok) - 1900;
	tok = strtok (NULL, " :\0");
	gt->tm_hour = atoi (tok);
	tok = strtok (NULL, " :\0");
	gt->tm_min = atoi (tok);
	tok = strtok (NULL, " :\0");
	gt->tm_sec = atoi (tok);
	gt->tm_isdst = daylight;
	req_time = mktime(gt);
	req_time -= timezone;
	return req_time;
}


int	ETagGen (char file_name[], char md_string[])
{
	MD5_CTX	ctx;
	unsigned char	md[MD5_LBLOCK];
	FILE	*fp;
	int	ret;

        fp = fopen (file_name, "rb");
	if (fp == NULL)
	{
		return FALSE;
	}

        MD5_Init (&ctx);
        while (1)
        {
                ret = fread (http_temp, 1, sizeof(http_temp), fp);
                if (ret > 0) MD5_Update(&ctx, http_temp, ret);
                else break;
        }
        fclose (fp);
        MD5_Final (md, &ctx);
        sprintf (md_string, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        md[0], md[1], md[2], md[3], md[4], md[5], md[6], md[7],
                        md[8], md[9], md[10], md[11], md[12], md[13], md[14], md[15]);
	return TRUE;
}

void	FileBodySend (char file_name[], int sock)
{
	FILE	*fp;
	int	ret;

        fp = fopen (file_name, "rb");
	if (fp == NULL)
	{
		return;
	}
        while (1)
        {
                ret = fread (http_temp, 1, sizeof(http_temp), fp);
                if (ret > 0) send (sock, http_temp, ret, 0);
                else break;
        }

        fclose(fp);
}

/*
        Java Scrip File send
        file name : /opt/dv_ap/web/dv_ap.js
*/
void    js_send(int sock, char buf[] )
{
        char   md_string[33];
        int     ret;
        struct  stat    stat_buf;
        char    *tok;
        time_t  req_time;
        char    NotMod;
        extern long int timezone;
        extern  int     daylight;

        ret = stat (JS_FILE, &stat_buf);

        if (ret != 0)
        {
                NotFound (sock);
                return;
        }

        if (!ETagGen (JS_FILE, md_string)) return;

        req_time = 0;
        tok = strtok (buf, "\r\n\0");
        NotMod = FALSE;
        while (tok)     /* Checnk last modified */
        {
                if (!memcmp (tok, "If-Modified-Since: ", 19))
                {
                        req_time = ModifiedTime (tok+24);
                        if (req_time >= stat_buf.st_mtime) NotMod = TRUE;
                }
                else if (!memcmp (tok, "If-None-Match: ", 15))
                {
                        if (!memcmp (tok+16, md_string, 32))
                        {
                                NotModify (sock);
                                return;
                        }

                }
                tok = strtok (NULL, "\r\n\0");
        }
        if (NotMod)
        {
                NotModify(sock);
                return;
        }

        send (sock, "HTTP/1.1 200 OK\r\n", 16, 0);
        DateSend (sock);
        LastModSend (sock, stat_buf.st_mtime);
        ETagSend (sock, md_string);
        send (sock, "Content-Type: text/javascript\r\n", 31, 0);
        send (sock, "Cache-Control: max-age=3600\r\n", 29, 0);
        send (sock, "Connection: close\r\n", 19, 0);
        sprintf (http_temp, "Content-length: %0ld\r\n", stat_buf.st_size);
        send (sock, http_temp, strlen(http_temp), 0);
        send (sock, "\r\n", 2, 0);

        FileBodySend (JS_FILE, sock);
}

/*
        logo File send
        file name : /opt/dv_ap/web/logo.png
*/
void    logo_send(int sock, char buf[] )
{
	char	md_string[33];
        int     ret;
        struct  stat    stat_buf;
        char    *tok;
        time_t  req_time;
	char	NotMod;
        extern long int timezone;
        extern  int     daylight;

        ret = stat (LOGO_FILE, &stat_buf);

        if (ret != 0)
        {
                NotFound (sock);
                return;
        }

	if (!ETagGen (LOGO_FILE, md_string)) return;

        req_time = 0;
        tok = strtok (buf, "\r\n\0");
	NotMod = FALSE;
        while (tok)     /* Checnk last modified */
        {
                if (!memcmp (tok, "If-Modified-Since: ", 19))
                {
                        req_time = ModifiedTime (tok+24);
			if (req_time >= stat_buf.st_mtime) NotMod = TRUE;
                }
		else if (!memcmp (tok, "If-None-Match: ", 15))
		{
			if (!memcmp (tok+16, md_string, 32))
			{
				NotModify (sock);
				return;
			}
			
		}
                tok = strtok (NULL, "\r\n\0");
        }
        if (NotMod)
        {
                NotModify(sock);
                return;
        }

        send (sock, "HTTP/1.1 200 OK\r\n", 16, 0);
        DateSend (sock);
	LastModSend (sock, stat_buf.st_mtime);
	ETagSend (sock, md_string);
        send (sock, "Content-Type: image/png\r\n", 26, 0);
	send (sock, "Cache-Control: max-age=3600\r\n", 29, 0);
        send (sock, "Connection: close\r\n", 19, 0);
        sprintf (http_temp, "Content-length: %0ld\r\n", stat_buf.st_size);
        send (sock, http_temp, strlen(http_temp), 0);
        send (sock, "\r\n", 2, 0);

	FileBodySend (LOGO_FILE, sock);
}


/*
	CSS File send
	file name : /opt/dv_ap/web/dv_ap.css
*/
void	css_send(int sock, char	buf[] )
{
	char	md_string[33];

	int	ret;
	struct	stat	stat_buf;
	char	*tok;
	time_t	req_time;
	char	NotMod;
	extern long int	timezone;
	extern	int	daylight;

	ret = stat (CSS_FILE, &stat_buf);

	if (ret != 0)
	{
		NotFound (sock);
		return;
	}

	if (!ETagGen (CSS_FILE, md_string)) return;

	req_time = 0;
	tok = strtok (buf, "\r\n\0");
	NotMod = FALSE;
	while (tok)	/* Checnk last modified */
	{
                if (!memcmp (tok, "If-Modified-Since: ", 19))
                {
                        req_time = ModifiedTime (tok+24);
                        if (req_time >= stat_buf.st_mtime) NotMod = TRUE;
                }
                else if (!memcmp (tok, "If-None-Match: ", 15))
                {
                        if (!memcmp (tok+16, md_string, 32))
			{
				NotModify(sock);
                        	return;
			}
                }
		tok = strtok (NULL, "\r\n\0");
	}
	if (NotMod)
	{
		NotModify(sock);
		return;
	}
			
	send (sock, "HTTP/1.1 200 OK\r\n", 16, 0);
	DateSend (sock);
	LastModSend (sock, stat_buf.st_mtime);
	ETagSend (sock, md_string);
	send (sock, "Content-Type: text/css\r\n", 24, 0);
	send (sock, "Cache-Control: max-age=3600\r\n", 29, 0);
	send (sock, "Connection: close\r\n", 19, 0);
	sprintf (http_temp, "Content-length: %0ld\r\n", stat_buf.st_size);
	send (sock, http_temp, strlen(http_temp), 0);
	send (sock, "\r\n", 2, 0);

	FileBodySend (CSS_FILE, sock);
}

void	DateSend(int sock)
{
	time_t	atime;

	time(&atime);
        gt = gmtime(&atime);
        sprintf (http_temp, "Date: %3.3s, %2.2d %3.3s %4.4d %02d:%02d:%02d GMT\r\n",
                        wday[gt->tm_wday],
                        gt->tm_mday,
                        month[gt->tm_mon],
                        gt->tm_year+1900,
                        gt->tm_hour,
                        gt->tm_min,
                        gt->tm_sec);
        send (sock, http_temp, strlen(http_temp), 0);
}

void	chunked_send(char string[], int sock)
{
	char	chunk[7];
	unsigned int	len;

	len = strlen(string);

	sprintf (chunk, "%03x\r\n", len);
	send (sock, chunk, 5, 0);
	send (sock, string, len, 0);
	send (sock, "\r\n", 2, 0);
}

void	httpd_srv_recv(int sock)
{
	struct	forward		*fwd_pnt;
	struct	status		*sta_pnt;
	time_t	atime;
	extern	time_t	start_time;
	time_t	dtime;
	int	len;
	int	d, h, m, s;
	char	temp[512];
	int	i;
	char	header;
	struct  SendCheck *send_check_next;
	char	time_string[30];
	char	inet_time_string[30];
	char	call_string[16];
	char	inet_call_string[16];

	if ((len = recv (sock, temp, sizeof(temp) - 1, 0)) <= 0)
	{
		FD_CLR (sock, &save_rfds);
		close (sock);
		sock = 0;
		return;
	}
	temp[len] = 0x00;
	if (len < 10) return;

	if (!memcmp (temp, "GET /favicon.ico", 16))
	{
		NotFound(sock);
		return;
	}
	else if (!memcmp (temp, "GET /dv_ap.css", 14))
	{
		css_send(sock, temp );
		return;
	}
        else if (!memcmp (temp, "GET /logo.png", 13))
        {
                logo_send(sock, temp );
                return;
        }
        else if (!memcmp (temp, "GET /dv_ap.js", 13))
        {
                js_send(sock, temp );
                return;
        }

	else if (!memcmp (temp, "GET /index.htm", 14) 
		|| !memcmp (temp, "GET / HTTP", 10))
	{  
		send (sock,"HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n", 42, 0);
		DateSend (sock);
		sprintf (temp, "Server: dv_ap %5.5s\r\n", PACKAGE_VERSION);
		send (sock, temp, strlen(temp), 0);
		send (sock, "Cache-Control: no-cache\r\n", 25, 0);
		send (sock, "Transfer-Encoding: chunked\r\n\r\n",  30, 0);
		time(&atime);
		chunked_send ("<!DOCTYPE HTML>\r\n\0",  sock);
		if (node_area_rep_callsign[0] != 0x20)
		{
			sprintf (temp, "<html><head><meta http-equiv=\"refresh\" content=\"5\"><title>D-STAR dv_ap Status %7.7s</title>\r\n", node_area_rep_callsign);
		}
		if (dvap_area_rep_callsign[0] != 0x20)
		{
			sprintf (temp, "<html><head><meta http-equiv=\"refresh\" content=\"5\"><title>D-STAR dv_ap Status %7.7s</title>\r\n", dvap_area_rep_callsign);
                }
		chunked_send(temp, sock);
		sprintf (temp, "<script type=\"text/javascript\" src=\"dv_ap.js\"></script>\r\n");
		chunked_send(temp, sock);
		sprintf (temp, "<link rel=\"stylesheet\" type=\"text/css\" href=\"/dv_ap.css\" /></head>\r\n");
		chunked_send (temp, sock);

		sprintf (temp, "<body><h3>D-STAR dv_ap Status V%5.5s at %24.24s</h3>\r\n",PACKAGE_VERSION, ctime(&atime));
		chunked_send(temp, sock);
		chunked_send ("<img src=\"logo.png\" align=\"left\" border=\"0\"><br>\r\n\0", sock);

		dtime = atime - start_time;
		d = dtime / 86400;
		dtime %= 86400;
		h = dtime / 3600;
		dtime %= 3600;
		m = dtime / 60;
		s = dtime % 60;
		sprintf (temp, "<table><tr><td>Started</td><td>%24.24s</td></tr><tr><td>Up time</td><td>\r\n", ctime(&start_time));
		chunked_send(temp, sock);
		if (d)
		{
			sprintf (temp, "<div Align=right>%dd%2dh%2dm%2ds</div></td></tr>\r\n", d, h, m, s);
		}
		else if (h)
		{
			sprintf (temp, "<div Align=right>%2dh%2dm%2ds</div></td></tr>\r\n", h, m, s);
		}
		else if (m)
		{
	        	sprintf (temp, "<div align=right>%dm%2ds</div></td></tr>\r\n", m, s);
		}
		else 
		{
        		sprintf (temp, "<div align=right>%2ds</div></td></tr>\r\n", s);
		}
		chunked_send(temp, sock);
		chunked_send ("</table>\r\n\0", sock);


		/* dv_ap Information */
		sprintf (temp, "<h3>dv_ap Status</h3><table><tr><th>Acc. Call</th><th>Gateway</th><th>My Callsign</th><th>Ur Callsign</th>\r\n");
		chunked_send (temp, sock);
		sprintf (temp, "<th>Short Message</th><th>Last Access Time</th><th>Packets</th><th>D-PRS</th></tr>\r\n");
		chunked_send (temp, sock);

		if (node_area_rep_callsign[0] != 0x20)
		{
			memset (&time_string, 0x20, 30);
			memset (&inet_time_string, 0x20, 30);
			if (node_recv_time)
				sprintf (time_string, "%24.24s", ctime(&node_recv_time));
			if (node_inet_recv_time)
				sprintf (inet_time_string, "%24.24s", ctime(&node_inet_recv_time));
			if (!memcmp (node_save_hdr.MyCall2, "    ", 4))
				sprintf (call_string, "%8.8s     ", node_save_hdr.MyCall);
			else
				sprintf (call_string, "%8.8s/%4.4s", node_save_hdr.MyCall, node_save_hdr.MyCall2);
                        if (!memcmp (&node_inet_header[35], "    ", 4))
                                sprintf (inet_call_string, "%8.8s     ", &node_inet_header[27]);
                        else
                                sprintf (inet_call_string, "%8.8s/%4.4s", &node_inet_header[27], &node_inet_header[35]);
			sprintf (temp,"<tr><td>%8.8s</td><td>%8.8s</td><td>%13.13s<br>%13.13s</td><td>%8.8s<br>%8.8s</td><td>%20.20s<br>%20.20s</td><td>%24.24s<br>%24.24s</td><td><center>%ld/%ld<br>%ld/%ld</center></td>\r\n",
			node_area_rep_callsign, gateway_callsign,
			call_string, 
			inet_call_string,
			node_save_hdr.YourCall, &node_inet_header[19],
			node_msg.short_msg,
			node_inet_msg.short_msg,
			time_string,
			inet_time_string,
			node_rig_pkt_cnt, node_rig_pkt_total_cnt,
			node_inet_pkt_cnt, node_inet_pkt_total_cnt);
			chunked_send (temp, sock);
                        if (node_msg.AprsSend == APRS_SEND) sprintf (temp, "<td><a href=\"#\"><center><b>Send</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", node_msg.aprs_msg_save);
                        else if (node_msg.AprsSend == APRS_SHORT) sprintf (temp, "<td><a href=\"#\"><center><b>Short</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", node_msg.aprs_msg_save);
                        else if (node_msg.AprsSend == APRS_NG)    sprintf (temp, "<td><a href=\"#\"><cneter><b>No Data</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", node_msg.aprs_msg_save);
                        else if (node_msg.AprsSend == APRS_ERROR) sprintf (temp, "<td><a href=\"#\"><center><b>Error</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", node_msg.aprs_msg_save);
                        else sprintf (temp, "<td> </td></tr>\r\n");
                        chunked_send(temp, sock);
		}
		if (dvap_area_rep_callsign[0] != 0x20)
		{
			memset (&time_string, 0x20, 30);
			memset (&inet_time_string, 0x20, 30);
			if (dvap_recv_time)
				sprintf (time_string, "%24.24s", ctime (&dvap_recv_time));
			if (dvap_inet_recv_time)
				sprintf (inet_time_string, "%24.24s", ctime (&dvap_inet_recv_time));
                        if (!memcmp (&dvap_header[41], "    ", 4))
                                sprintf (call_string, "%8.8s     ", &dvap_header[33]);
                        else
                                sprintf (call_string, "%8.8s/%4.4s", &dvap_header[33], &dvap_header[41]);
                        if (!memcmp (&dvap_inet_header[35], "    ", 4))
                                sprintf (inet_call_string, "%8.8s     ", &dvap_inet_header[27]);
                        else                                                                                                                      sprintf (inet_call_string, "%8.8s/%4.4s", &dvap_inet_header[27], &dvap_inet_header[35]);
			sprintf (temp,"<tr><td>%8.8s</td><td>%8.8s</td><td>%13.13s<br>%13.13s</td><td>%8.8s<br>%8.8s</td><td>%20.20s<br>%20.20s</td><td>%24.24s<br>%24.24s</td><td><center>%ld/%ld<br>%ld/%ld</center></td>\r\n",
			dvap_area_rep_callsign, gateway_callsign,
			call_string,
			inet_call_string,
			&dvap_header[25], &dvap_inet_header[19],
			dvap_msg.short_msg,
			dvap_inet_msg.short_msg,
			time_string,
			inet_time_string,
			dvap_rig_pkt_cnt, dvap_rig_pkt_total_cnt,
			dvap_inet_pkt_cnt, dvap_inet_pkt_total_cnt);
			chunked_send (temp, sock);
                        if (dvap_msg.AprsSend == APRS_SEND) sprintf (temp, "<td><a href=\"#\"><center><b>Send</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", dvap_msg.aprs_msg_save);
                        else if (dvap_msg.AprsSend == APRS_SHORT) sprintf (temp, "<td><a href=\"#\"><center><b>Short</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", dvap_msg.aprs_msg_save);
                        else if (dvap_msg.AprsSend == APRS_NG)    sprintf (temp, "<td><a href=\"#\"><cneter><b>No Data</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", dvap_msg.aprs_msg_save);
                        else if (dvap_msg.AprsSend == APRS_ERROR) sprintf (temp, "<td><a href=\"#\"><center><b>Error</b></center><span class=\"msg_body\">%s</span></a></td></tr>\r\n", dvap_msg.aprs_msg_save);
                        else sprintf (temp, "<td> </td></tr>\r\n");
                        chunked_send(temp, sock);
		}
		chunked_send ("</table><br>\r\n\0", sock);

		/* APRS server information */
		chunked_send ("<h3>APRS Server</h3>\r\n\0", sock);

		sprintf (temp, "<table><tr><td><a href=http://%s:14501>%s</a> \r\n",
			aprs_ip, aprs_server);
		chunked_send (temp, sock);
		if (verify_sw) chunked_send ("Verified.   \r\n\0", sock);
		else chunked_send ("Unverified. \r\n\0", sock);

		sprintf (temp, "</td><td>%s:%0d (%s:%0d)</td><td><b>Up</b> \r\n", 
			aprs_server, aprs_port,
			aprs_ip, aprs_port);
		chunked_send (temp, sock);
       		if (d)
       		{
               		sprintf (temp, "%dd%2dh%2dm%2ds</td>\r\n", d, h, m, s);
       		}
       		else if (h)
       		{
               		sprintf (temp, "%2dh%2dm%2ds</td>\r\n", h, m, s);
       		}
       		else if (m)
       		{
               		sprintf (temp, "%dm%2ds</td>\r\n", m, s);
       		}
       		else
       		{
               		sprintf (temp, "%2ds</td>\r\n", s);
       		}
		chunked_send (temp, sock);
	}
		sprintf (temp, "<td>(%ld/%ld packets)\r\n", aprs_cnt, aprs_beacon_cnt);
		chunked_send (temp, sock);
		chunked_send ("</td></tr></table>\r\n\0", sock);

		/* D-PRS Beacon timer */
                sprintf (temp, "<h3>D-PRS Message Send Status</h3><table><tr><td><b>Beacon Timer</b></td><td>%d Sec.</td>\r\n", BeaconInterval);
                chunked_send (temp, sock);
		/* D-PRS Interval timer */
		sprintf (temp, "<td><b>Interval Timer</b></td><td>%d Sec.</td></tr></table>\r\n", aprs_send_interval);
		chunked_send (temp, sock);
		header = FALSE;
		time (&atime);

		send_check_next = send_check_pnt;
		while (send_check_next)
		{
                	if (send_check_next->SendTime <= atime) send_check_next->CallSign[0] = 0x00;
                	if (send_check_next->CallSign[0] != 0x00)
                	{
				if (!header)
				{
	        			sprintf (temp, "<table><tr><th>Call Sign</th><th>Send Time</th><th>Remaining time</th></tr>\r\n");
        				chunked_send (temp, sock);
					header = TRUE;
				}
				sprintf (temp, "<tr><td>%10.10s</td><td>%24.24s</td><td><center>%ld</center></td></tr>\r\n",
                        		send_check_next->CallSign, ctime(&send_check_next->SendTime), (send_check_next->SendTime - atime));
				chunked_send (temp, sock); 
                	}
			send_check_next = send_check_next->next;
        	}
		if (header) chunked_send ("</table>\r\n\0", sock);


		/* dv_ap information */
		chunked_send ("<h3>dv_ap information</h3>\r\n\0", sock);

                sprintf (temp, "<table><tr><td>Trust server: %s (%s)</td> \r\n",
                        trust_name, inet_ntoa(trust_ip));
                chunked_send (temp, sock);

		sprintf (temp, "<td>Trust timeout: %d mSec.</td> \r\n", trust_timeout);
		chunked_send (temp, sock);
		
		chunked_send ("</tr></table>\r\n\0", sock);

                /* upnp information */
               	chunked_send ("<h3>upnp information</h3>\r\n\0", sock);

               	if (upnp_sw) sprintf (temp, "<table><tr><td>own ip: %s (gw: %s)</td><td>port: %d UDP</td><td>Open</td> \r\n", inet_ntoa (OwnDvApIP), upnp_ip_addr,  gateway_port);
		else sprintf (temp, "<table><tr><td>Close</td> \r\n");
               	chunked_send (temp, sock);
               	chunked_send ("</tr></table>\r\n\0", sock);

		/* end close */
		chunked_send ("<br><b>(C) Copyright 2016-2017 JARL D-STAR Committee</b>\r\n\0", sock);
		chunked_send ("</body></html>\r\n\0", sock);


		/* Treminate (Last) for Chunked */
		send (sock, "0\r\n\r\n", 5, 0);
	
}


void	httpd_init (void)
{
	int	err;

        // httpd
        if (http_port > 0)
        {
                memset (&hints, 0x00, sizeof(hints));
                hints.ai_socktype = SOCK_STREAM;
                hints.ai_family = PF_UNSPEC;
                hints.ai_flags = AI_PASSIVE;
                sprintf (PORT, "%d", http_port);
                if ((err = getaddrinfo (NULL, PORT, &hints, &http_serv)) != 0)
                {
                        fprintf (log_file, "%24.24s getaddrinfo error (http port:%d) %s\n",
                                ctime(&cur_time), http_port, gai_strerror(err));
                        http_port = 0;
                        fflush (log_file);
                }
                else
                {
                        if ((httpd_sd = socket (http_serv->ai_family, http_serv->ai_socktype, http_serv->ai_protocol)) < 0)
                        {
                                time(&cur_time);
                                fprintf (log_file, "%24.24s HTTP TCP Socket not open. Already used this port:%d\n", ctime(&cur_time), http_port);
                                fflush (log_file);
                                http_port = 0;
                                freeaddrinfo (http_serv);
                        }
                        else
                        {
				int	yes = 1;
                                if (setsockopt (httpd_sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes)))
                                {
                                        time(&cur_time);
                                        fprintf (log_file, "%24.24s error setsock opt on HTTP_PORT\n", ctime(&cur_time));
                                        fflush (log_file);
                                        http_port = 0;
                                        freeaddrinfo (http_serv);
                                        close (httpd_sd);
                                        httpd_sd = 0;
                                }
                                else
                                {
                                        if (bind (httpd_sd, http_serv->ai_addr, http_serv->ai_addrlen))
                                        {
                                                time (&cur_time);
                                                fprintf (log_file, "%24.24s error bind on HTTP_PORT %s\n", ctime(&cur_time), strerror(errno));
                                                fflush (log_file);
                                                http_port = 0;
                                        }
                                        else
                                        {
                                                if (listen (httpd_sd, 5))
                                                {
                                                        time (&cur_time);
                                                        fprintf (log_file, "%24.24s error listen on HTTP_PORT %s\n",
                                                                                ctime(&cur_time), strerror(errno));
                                                        http_port = 0;
                                                        freeaddrinfo (http_serv);
                                                        fflush (log_file);
                                                }
                                                else
                                                {
                                                        FD_SET (httpd_sd, &save_rfds);
                                                        fprintf (log_file, "%24.24s HTTP Port %d open.\n",
                                                                        ctime(&cur_time), http_port);
                                                        fflush (log_file);
                                                }
                                        }
                                }

                        }
                }
                if (http_port) gt = malloc (sizeof (struct tm));
        }

}


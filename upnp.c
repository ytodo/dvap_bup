#include	"dv_ap.h"

void    send_upnp_http(void);
int	open_upnp_http(void);
void    send_upnp_port_info(void);
void	send_upnp_port_add(void);
void	send_upnp_port_del(void);
void	getOwnIp (void);

enum {
	UPNP_IDLE = 0,
	UPNP_SEND_MSEARCH_GATEWAY,
	UPNP_RECV_MSEARCH_GATEWAY,
	UPNP_OPEN_HTTP,
	UPNP_SEND_HTTP,
	UPNP_RECV_HTTP,
	UPNP_OPEN_PORTMAP_INFO,
	UPNP_REQ_PORTMAP_INFO,
	UPNP_RECV_PORTMAP_INFO,
	UPNP_OPEN_PORTMAP_ADD,
	UPNP_REQ_PORTMAP_ADD,
	UPNP_RECV_PORTMAP_ADD,
	UPNP_OPEN_PORTMAP_DEL,
	UPNP_REQ_PORTMAP_DEL,
	UPNP_RECV_PORTMAP_DEL
} upnp_state = UPNP_IDLE;

char	upnp_ip_addr_temp[64];
int	upnp_port;
char	upnp_str[64];
char	controlURL[128];
char	upnp_urn[128];

void	send_msearch_gateway(void)
{
	char    upnp_msg_gateway[] =
		{"M-SEARCH * HTTP/1.1\r\n" \
		"HOST: 239.255.255.250:1900\r\n" \
		"ST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n" \
		"MAN: \"ssdp:discover\"\r\n" \
		"MX: 2\r\n\r\n"
		};

	time(&cur_time);

	sendto(upnp_udp_sd, &upnp_msg_gateway, sizeof(upnp_msg_gateway), 0,
                upnp_sock->ai_addr, upnp_sock->ai_addrlen);
	if (debug)
	{
		fprintf (log_file, "%24.24s M-SEARCH send (gateway)\n", 				ctime(&cur_time));
		if (debug >= 2)
		{
			fprintf (log_file, "%s\n", upnp_msg_gateway);
		}
		fflush (log_file);
	}
}

void	read_upnp_udp(void)
{
	char    *delmi = "\n\r\t\0";
	char	*pnt;
	char	*ip_addr;
	char	*port;
	int	k;

        int     len;
        in_upnp_addr_len = sizeof(struct sockaddr_storage);

        if((len = recvfrom(upnp_udp_sd, &upnp_buf, sizeof(upnp_buf), 0,
                (struct sockaddr *)&in_upnp_addr, &in_upnp_addr_len)) < 0)
        {
                time(&cur_time);
                fprintf (log_file, "%24.24s recvfrom error %s\n",
                        ctime(&cur_time), strerror(errno));
                fflush (log_file);
		FD_CLR (upnp_udp_sd, &save_rfds);
		close (upnp_udp_sd);
		upnp_udp_sd = 0;
                return ;
        }

	upnp_buf[len] = 0x00;
	if (debug >= 2)
	{
		fprintf (log_file, "%s\n", upnp_buf);
		fflush (log_file);
	}
	pnt = strtok(upnp_buf, delmi);
	while (pnt)
	{
		for (k = 0 ; k < 9 ; k++)
		{
			*pnt = toupper (*pnt);
			pnt++;
		}
		pnt -= 9;
		if (!memcmp (pnt, "LOCATION:",9))
		{
			while (memcmp (pnt, "//", 2)) pnt++;
			pnt += 2;
			upnp_port = 0;
			ip_addr = pnt;
			while (*pnt != ':') pnt++;
			*pnt = 0x00;
			memset (upnp_ip_addr, 0x00, 64);
			k = strlen(ip_addr);
			if ((k > 0) && (k < 64))
			{
				memcpy (upnp_ip_addr, ip_addr, k);
				pnt++;
				port = pnt;
				while (*pnt != '/') pnt++;
				*pnt = 0x00;
				upnp_port = atoi(port);
				pnt++;
				memset (upnp_str, 0x00, 64);
				memcpy (upnp_str, pnt, strlen(pnt));
				upnp_state = UPNP_OPEN_HTTP;
				if (k && upnp_port) return;
			}
		}
		pnt = strtok (NULL, delmi);
	}
	time (&cur_time);
	fprintf (log_file, "%24.24s m-search not found Location\n", 
		ctime(&cur_time));
	fflush (log_file);
}

void	upnp (void)
{
	switch (upnp_state)
	{
		case UPNP_IDLE:
			break;

		case UPNP_SEND_MSEARCH_GATEWAY:
			send_msearch_gateway();
			upnp_state = UPNP_RECV_MSEARCH_GATEWAY;
			time(&upnp_send_time);
			break;

		case UPNP_RECV_MSEARCH_GATEWAY:
			time (&cur_time);
			if ((cur_time - upnp_send_time) > 30)
			{
				upnp_state = UPNP_IDLE;
				time(&cur_time);
				fprintf (log_file, "%24.24s m-search (gateway) time out\n", 
					ctime(&cur_time));
				fflush (log_file);
			}
			break;

		case UPNP_OPEN_HTTP:
			if (!open_upnp_http()) break;
			upnp_state = UPNP_SEND_HTTP;
			break;
	
		case UPNP_SEND_HTTP:
			send_upnp_http();
			upnp_state = UPNP_RECV_HTTP;
			time(&upnp_send_time);
			break;

		case UPNP_RECV_HTTP:
			time (&cur_time);
			if ((cur_time - upnp_send_time) > 5)
			{
				if (upnp_http_sd > 0)
				{
                                	FD_CLR (upnp_http_sd, &save_rfds);
                                	close (upnp_http_sd);
                                	upnp_http_sd = 0;
				}
				upnp_state = UPNP_IDLE;
			}
			break;
	
		case UPNP_OPEN_PORTMAP_INFO:
			if (!open_upnp_http()) break;
			upnp_state = UPNP_REQ_PORTMAP_INFO;
			break;

		case UPNP_REQ_PORTMAP_INFO:
			send_upnp_port_info();
			upnp_state =  UPNP_RECV_PORTMAP_INFO;
			time(&upnp_send_time);
			break;

        	case UPNP_RECV_PORTMAP_INFO:
			time (&cur_time);
			if ((cur_time - upnp_send_time) > 5)
			{
                                if (upnp_http_sd > 0)
				{
					FD_CLR (upnp_http_sd, &save_rfds);
                                	close (upnp_http_sd);
                                	upnp_http_sd = 0;
				}
				upnp_state = UPNP_IDLE;
			}
			break;

		case UPNP_OPEN_PORTMAP_ADD:
			if (!open_upnp_http()) break;
			upnp_state = UPNP_REQ_PORTMAP_ADD;
			break;

                case UPNP_REQ_PORTMAP_ADD:
                        send_upnp_port_add();
                        upnp_state =  UPNP_RECV_PORTMAP_ADD;
                        time(&upnp_send_time);
                        break;

                case UPNP_RECV_PORTMAP_ADD:
                        time (&cur_time);
                        if ((cur_time - upnp_send_time) > 5)
                        {
				if (upnp_http_sd > 0)
				{
                                	FD_CLR (upnp_http_sd, &save_rfds);
                                	close (upnp_http_sd);
                                	upnp_http_sd = 0;
				}
                                upnp_state = UPNP_IDLE;
                        }
			break;

		case UPNP_OPEN_PORTMAP_DEL:
			if (!open_upnp_http()) break;
			upnp_state = UPNP_REQ_PORTMAP_DEL;
			time(&upnp_send_time);
			break;

		case UPNP_REQ_PORTMAP_DEL:
			send_upnp_port_del();
			upnp_state =  UPNP_RECV_PORTMAP_DEL;
			time(&upnp_send_time);
			break;

		case UPNP_RECV_PORTMAP_DEL:
			time (&cur_time);
			if ((cur_time - upnp_send_time) > 5)
			{
				if (upnp_http_sd > 0)
				{
					FD_CLR (upnp_http_sd, &save_rfds);
					close (upnp_http_sd);
					upnp_http_sd = 0;
				}
				upnp_state = UPNP_IDLE;
			}
			break;
	}


}

void	upnp_msearch_set (void)
{
	upnp_state = UPNP_SEND_MSEARCH_GATEWAY;;
}

void	upnp_portmap_del(void)
{
	upnp_state = UPNP_OPEN_PORTMAP_DEL;
	upnp_add_sw = FALSE;
	while (1)
	{
		upnp();
		if (upnp_state == UPNP_RECV_PORTMAP_DEL) break;
	}
}

int	open_upnp_http(void)
{
	struct sockaddr_in server;
	char *deststr;
	unsigned int **addrptr;
	time_t	atime;
	int	yes = 1;

	if (upnp_ip_addr[0] == 0x00)  return FALSE;

	upnp_http_sd = socket(AF_INET, SOCK_STREAM, 0);
	if (upnp_http_sd < 0) {
		time(&atime);
		fprintf (log_file, "%24.24s socket error\n", ctime(&atime));
		fflush (log_file);
		return FALSE;
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(upnp_port);

	server.sin_addr.s_addr = inet_addr(upnp_ip_addr);
	if (server.sin_addr.s_addr == 0xffffffff) {
		struct hostent *host;

		host = gethostbyname(upnp_ip_addr);
		if (host == NULL) {
			time(&atime);
			if (h_errno == HOST_NOT_FOUND) {
				fprintf(log_file, "%24.24s host not found : %s\n", ctime(&atime), upnp_ip_addr);
			} else {
				fprintf(log_file, "%24.24s %s : %s\n", ctime(&atime), hstrerror(h_errno), upnp_ip_addr);
			}
			fflush (log_file);
			return FALSE;
		}

		addrptr = (unsigned int **)host->h_addr_list;

		while (*addrptr != NULL) {
			server.sin_addr.s_addr = *(*addrptr);

		 	if (connect(upnp_http_sd,
				(struct sockaddr *)&server,
				sizeof(server)) == 0) {
				break;
		 	}

		 	addrptr++;
		}

	 	if (*addrptr == NULL) {
			time(&atime);
			fprintf (log_file, "%24.24s connect error\n", ctime(&atime));
			fflush (log_file);
			return FALSE;
		}
	} else {
		if (connect(upnp_http_sd,
        		(struct sockaddr *)&server, sizeof(server)) != 0) {
			time (&atime);
			fprintf (log_file, "%24.24s connect error\n", ctime(&atime));
			fflush (log_file);
			return FALSE;
		}
	}
	setsockopt (upnp_http_sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));	
	FD_SET (upnp_http_sd, &save_rfds);
	upnp_buf_pnt = 0;
	upnp_add_sw = FALSE;
	return TRUE;
}

void    send_upnp_http(void)
{
	char	buf[256];

 	memset(buf, 0x00, sizeof(buf));
	sprintf(buf, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: Close\r\n\r\n", upnp_str, upnp_ip_addr);

	send(upnp_http_sd, buf, strlen(buf), 0);
	if (debug >= 2)
	{
		fprintf (log_file, "%s\n", buf);
		fflush (log_file);
	}
}

void	read_upnp_http(void)
{
	int	n;
	int	k;
	int	kk;
	int	kkk;
	int	len;
	int	sw;
	int	urn_sw;
	int	URL_sw;
	time_t	atime;
	char	ClientIpAddr[16];
	int	client_sw;

	upnp_buf_pnt = read(upnp_http_sd, 
		upnp_buf, sizeof(upnp_buf));
	if (upnp_buf_pnt <= 0) 
	{
        	FD_CLR (upnp_http_sd, &save_rfds);
        	close (upnp_http_sd);
        	upnp_http_sd = 0;
		return;
	}
	len = 0;
	while (upnp_buf_pnt > 0)
	{
		len += upnp_buf_pnt;
		upnp_buf_pnt = read (upnp_http_sd,
			&upnp_buf[len], sizeof (upnp_buf));
	}

	sw = FALSE;
	client_sw = FALSE;
	urn_sw = FALSE;
	upnp_buf[len] = 0x00;
	if (debug >= 2)
	{
		fprintf (log_file, "%s\n", upnp_buf);
		fflush (log_file);
	}
	URL_sw = FALSE;	
	len = strlen (upnp_buf) - 47;
	for (k = 0 ; k < len ; k++) 
	{
		if (upnp_state == UPNP_RECV_HTTP)
		{
			if (!memcmp (&upnp_buf[k], 
				"urn:schemas-upnp-org:service:WANPPPConnection:1", 47))  
			{
				sw = TRUE;
				time(&atime);
				memset (upnp_urn, 0x00, sizeof(upnp_urn));
				memcpy (upnp_urn, &upnp_buf[k], 47);
				if (debug) 
				{
					fprintf (log_file, "%24.24s %s\n", ctime(&atime), upnp_urn);
					fflush (log_file);
				}
				urn_sw = TRUE;
			}
			else if (!memcmp (&upnp_buf[k], 
				"urn:schemas-upnp-org:service:WANIPConnection:1", 46))
			{
				sw = TRUE;
				time(&atime);
				memset (upnp_urn, 0x00, sizeof(upnp_urn));
				memcpy (upnp_urn, &upnp_buf[k], 46);
				if (debug)
				{
					fprintf (log_file, "%24.24s %s\n", ctime(&atime), upnp_urn);
					fflush (log_file);
				}
				urn_sw = TRUE;
			}

			else if (!memcmp (&upnp_buf[k], "<URLBase>", 9))
			{
				for (kk = k + 16 ; kk < len ; kk++)
				{
					if (upnp_buf[kk] == ':')
					{
						upnp_buf[kk] = 0x00;
						break;
					}
				}
				memset (upnp_ip_addr, 0x00, sizeof(upnp_ip_addr));
				memcpy (upnp_ip_addr, &upnp_buf[k+16], strlen (&upnp_buf[k+16]));
				for (kkk = kk + 1; kkk < len ; kkk++)
				{
					if ((upnp_buf[kkk] == '/') && (upnp_buf[kkk] == '<'))
					{
						upnp_buf[kkk] = 0x00;
						break;
					}
				} 
				upnp_port = atoi(&upnp_buf[kk+1]);
				if (debug)
				{
					time (&atime);
					fprintf (log_file, "%24.24s URLBase %s:%d\n", 
						ctime(&atime), upnp_ip_addr, upnp_port);
					fflush (log_file);
				}
			}

			if (sw)
			{
				if (!memcmp (&upnp_buf[k], "<controlURL>", 12))
				{
					for (kk = k + 12 ; kk < len ; kk++)
					{
						if (upnp_buf[kk] == '<')
						{
							upnp_buf[kk] = 0x00;
							break;
						}
					}
					sw = FALSE;
					time(&atime);
					memset (controlURL, 0x00, sizeof(controlURL));
					memcpy (controlURL, &upnp_buf[k+12], strlen(&upnp_buf[k+12]));
					URL_sw = TRUE;
					if (debug)
					{
						fprintf (log_file, "%24.24s %s\n", ctime(&atime), controlURL);
						fflush (log_file);
					}
				}
			}
		}
                if ((upnp_state == UPNP_RECV_PORTMAP_INFO) && !memcmp (&upnp_buf[k], "<NewInternalClient>", 19))
                {
                        for (kk = k + 19 ; kk < len ; kk++)
                        {
                                if (upnp_buf[kk] == '<')
                                {
                                        upnp_buf[kk] = 0x00;
                                        break;
                                }
                        }
                        memset (ClientIpAddr, 0x00, sizeof(ClientIpAddr));
                        memcpy (ClientIpAddr, &upnp_buf[k+19], strlen(&upnp_buf[k+19]));
                        client_sw = TRUE;
			if (debug >= 2)
			{
				time (&atime);
				fprintf (log_file, "%24.24s IP address (router) %s\n", ctime (&atime), ClientIpAddr);
				fflush (log_file);
			}
                }	
	}

	FD_CLR (upnp_http_sd, &save_rfds);	
	close (upnp_http_sd);
	upnp_http_sd = 0;
	switch (upnp_state)
	{
		case UPNP_RECV_HTTP:
			if (urn_sw)
			{
				upnp_state = UPNP_OPEN_PORTMAP_INFO;
			}
			else
			{
				upnp_state = UPNP_IDLE;
			}
			break;

		case UPNP_RECV_PORTMAP_INFO:
			if (!client_sw)
			{
				upnp_state = UPNP_OPEN_PORTMAP_ADD;
			}
			else
			{
				getOwnIp();
				if (!memcmp (ClientIpAddr, inet_ntoa (OwnDvApIP), strlen(ClientIpAddr)))
				{
					upnp_state = UPNP_IDLE;
				}
				else
				{
					upnp_add_sw = TRUE;
					upnp_state = UPNP_OPEN_PORTMAP_DEL;
				}
			}
			break;

		case UPNP_RECV_PORTMAP_DEL:
			if (upnp_add_sw) upnp_state = UPNP_OPEN_PORTMAP_ADD;
			else upnp_state = UPNP_IDLE;
			
			break;
	}
}

void	send_upnp_port_add(void)
{
	char buf[1024];
	char header[512];
	int	length;
	int	header_l;

 	memset(buf, 0x00, sizeof(buf));
	memcpy (buf,  "<?xml version=\"1.0\"?>\r\n", 23); 
	length = 23;
	memcpy (&buf[length],  "<s:Envelope xmlns:s:=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n", 126);
	length += 126; 
	memcpy (&buf[length], "<s:Body>\r\n", 10);
	length += 10;
	sprintf (&buf[length], "<m:AddPortMapping xmlns:m=\"%s\">\r\n", upnp_urn);
	length = strlen(buf);
	memcpy (&buf[length], "<NewRemoteHost></NewRemoteHost>\r\n", 33);
	length += 33;
	sprintf (&buf[length], "<NewExternalPort>%d</NewExternalPort>\r\n", gateway_port);
	length = strlen(buf);
	memcpy (&buf[length], "<NewProtocol>UDP</NewProtocol>\r\n", 32);
	length += 32;
	sprintf  (&buf[length], "<NewInternalPort>%d</NewInternalPort>\r\n", gateway_port);
	length = strlen(buf);
	sprintf (&buf[length], "<NewInternalClient>%s</NewInternalClient>\r\n", 
		inet_ntoa (OwnDvApIP));
	length = strlen(buf);
	memcpy (&buf[length], "<NewEnabled>1</NewEnabled>\r\n", 28);
	length += 28;
	memcpy (&buf[length], "<NewPortMappingDescription>dv_ap_upnp</NewPortMappingDescription>\r\n", 67);
	length += 67;
	memcpy (&buf[length], "<NewLeaseDuration>0</NewLeaseDuration>\r\n", 40);
	length += 40;
	memcpy (&buf[length], "</m:AddPortMapping>\r\n", 21);
	length += 21;
	memcpy (&buf[length], "</s:Body>\r\n", 11);
	length += 11;
	memcpy (&buf[length], "</s:Envelope>\r\n", 15);
	length += 15;

	memset (header, 0x00, sizeof(header));
        sprintf(header, "POST %s HTTP/1.1\r\nHost: %s:%d\r\n",
                controlURL, upnp_ip_addr, upnp_port);

        header_l = strlen(header);;

        sprintf (&header[header_l], "Content-Length: %d\r\nContent-Type: text/xml\r\nConnection: Close\r\n", length);
        header_l = strlen(header);

        sprintf (&header[header_l], "SOAPAction: \"%s#AddPortMapping\"\r\n\r\n", upnp_urn);
        header_l = strlen(header);
	send (upnp_http_sd, header, header_l, 0);

	send (upnp_http_sd, buf, length, 0);

	if (debug >= 2)
	{
		fprintf (log_file, "%s\n", header);
		fprintf (log_file, "%s\n", buf);
		fflush (log_file);
	}
}

void	send_upnp_port_del(void)
{
	char buf[1024];
	char header[512];
	int	length;
	int	header_l;

 	memset(buf, 0x00, sizeof(buf));
	memcpy (buf,  "<?xml version=\"1.0\"?>\r\n", 23); 
	length = 23;
	memcpy (&buf[length],  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n", 126);
	length += 126; 
	memcpy (&buf[length], "<s:Body>\r\n", 10);
	length += 10;
	sprintf (&buf[length], "<m:DeletePortMapping xmlns:m=\"%s\">\r\n", upnp_urn);
	length = strlen(buf);
	memcpy (&buf[length], "<NewRemoteHost></NewRemoteHost>\r\n", 33);
	length += 33;
	sprintf (&buf[length], "<NewExternalPort>%d</NewExternalPort>\r\n", gateway_port);
	length = strlen(buf);
	memcpy (&buf[length], "<NewProtocol>UDP</NewProtocol>\r\n", 32);
	length += 32;
	memcpy (&buf[length], "</m:DeletePortMapping>\r\n", 21);
	length += 21;
	memcpy (&buf[length], "</s:Body>\r\n", 11);
	length += 11;
	memcpy (&buf[length], "</s:Envelope>\r\n", 15);
	length += 15;

	memset (header, 0x00, sizeof(header));
        sprintf(header, "POST %s HTTP/1.1\r\nHost: %s:%d\r\n",
                controlURL, upnp_ip_addr, upnp_port);

        header_l = strlen(header);;

        sprintf (&header[header_l], "Content-Length: %d\r\nContent-Type: text/xml\r\nConnection: Close\r\n", length);
        header_l = strlen(header);

        sprintf (&header[header_l], "SOAPAction: \"%s#DeletePortMapping\"\r\n\r\n", upnp_urn);
        header_l = strlen(header);
	send (upnp_http_sd, header, header_l, 0);

	send (upnp_http_sd, buf, length, 0);

        if (debug >= 2)
        {
                fprintf (log_file, "%s\n", header);
                fprintf (log_file, "%s\n", buf);
                fflush (log_file);
        }
}


void	send_upnp_port_info(void)
{
	char buf[1024];
	char header[512];
	int	length;
	int	header_l;

 	memset(buf, 0x00, sizeof(buf));
	memcpy (buf,  "<?xml version=\"1.0\"?>\r\n", 23); 
	length = 23;
	memcpy (&buf[length],  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n", 126);
	length += 126; 
	memcpy (&buf[length], "<s:Body>\r\n", 10);
	length += 10;
	sprintf (&buf[length], "<m:GetSpecificPortMappingEntry xmlns:m=\"%s\">\r\n", upnp_urn);
	length = strlen(buf);
        memcpy (&buf[length], "<NewRemoteHost></NewRemoteHost>\r\n", 33);
        length += 33;
        sprintf (&buf[length], "<NewExternalPort>%d</NewExternalPort>\r\n", gateway_port);
        length = strlen(buf);
        memcpy (&buf[length], "<NewProtocol>UDP</NewProtocol>\r\n", 32);
        length += 32;
	memcpy (&buf[length], "</m:GetSpecificPortMappingEntry>\r\n", 34);
	length += 34;
	memcpy (&buf[length], "</s:Body>\r\n", 11);
	length += 11;
	memcpy (&buf[length], "</s:Envelope>\r\n", 15);
	length += 15;

	memset (header, 0x00, sizeof(header));
        sprintf(header, "POST %s HTTP/1.1\r\nHost: %s:%d\r\n",
                controlURL, upnp_ip_addr, upnp_port);

        header_l = strlen(header);

        sprintf (&header[header_l], "Content-Length: %d\r\nContent-Type: text/xml\r\nConnection: Close\r\n", length);
        header_l = strlen(header);

        sprintf (&header[header_l], "SOAPAction: \"%s#GetSpecificPortMappingEntry\"\r\n\r\n", upnp_urn);
        header_l = strlen(header);
	send (upnp_http_sd, header, header_l, 0);

	send (upnp_http_sd, buf, length, 0);

	if (debug >= 2)
	{
		fprintf (log_file, "%s\n", header);
		fprintf (log_file, "%s\n", buf);
		fflush (log_file);
	}
}


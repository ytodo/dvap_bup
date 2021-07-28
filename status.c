#include	"dv_ap.h"

void	status_logoff_send(void)
{
	struct	STATUS_frame	StatusFrm;

	memset (&StatusFrm, 0x00, 100);
	memcpy (StatusFrm.StatusID, "DSTRST", 6);
	memcpy (StatusFrm.Type, "01", 2);
	memset (&StatusFrm.ip_addr, 0x00, 16);
	memcpy (&STATUS_Frm.ip_addr, &Gw_IP, 4);   /* IP V4 */
	#if __WORDSIZE == 64
	time (&StatusFrm.EntryUpdateTime);
	#else
	time (&StatusFrm.EntryUpdateTime);
	memset (&StatusFrm.dummy_t, 0x00, 4);
	#endif
	memcpy (StatusFrm.body.logoff.dv_ap_callsign, client_callsign, 8);
	sendto (dv_status.status_sd, &StatusFrm, 100, 0,
		dv_status.status_info->ai_addr,
		dv_status.status_info->ai_addrlen);
	/* re_send */
        sendto (dv_status.status_sd, &StatusFrm, 100, 0,
        	dv_status.status_info->ai_addr,
        	dv_status.status_info->ai_addrlen);
}

void	status_keep_alive_send(void)
{
	struct	STATUS_frame	StatusFrm;

        if (dv_status.port)
        {
		memset (&StatusFrm, 0x00, 100);
                memcpy (StatusFrm.StatusID, "DSTRST", 6);
                memcpy (StatusFrm.Type, "99", 2);
                memset (&StatusFrm.ip_addr, 0x00, 16);
                #if __WORDSIZE == 64
                time (&StatusFrm.EntryUpdateTime);
                #else
                time (&StatusFrm.EntryUpdateTime);
                memset (&StatusFrm.dummy_t, 0x00, 4);
                #endif
		if (node_area_rep_callsign[0] != 0x20)
		{
			memcpy (StatusFrm.body.keep_alive.ModuleName, node_area_rep_callsign, 8);
		}
		if (dvap_area_rep_callsign[0] != 0x20)
		{
			memcpy (StatusFrm.body.keep_alive.ModuleName, dvap_area_rep_callsign, 8);
		}
		if (IDxxPlus_area_rep_callsign[0] != 0x20)
		{
			memcpy (StatusFrm.body.keep_alive.ModuleName, IDxxPlus_area_rep_callsign, 8);
		}
		memcpy (StatusFrm.body.keep_alive.gateway_callsign, gateway_callsign, 8);
		memcpy (StatusFrm.body.keep_alive.Version, PACKAGE_STRING, sizeof (PACKAGE_STRING));
               	sendto (dv_status.status_sd, &StatusFrm, 100, 0,
                	dv_status.status_info->ai_addr,
                        dv_status.status_info->ai_addrlen);
                       	dv_status.packets++;
		time (&status_keep_alive);
        }
}

void	status_send_ptton (void)
{
	memcpy (STATUS_Frm.StatusID, "DSTRST", 6);
        memcpy (STATUS_Frm.Type, "05", 2);
       	if (node_area_rep_callsign[0] != 0x20)
       	{
       		memcpy (STATUS_Frm.body.status.RPT2Call, &node_pkt.dstar.b_bone.dstar_udp.rf_header.RPT2Call, 36);
        }
        if (dvap_area_rep_callsign[0] != 0x20)
        {
        	memcpy (STATUS_Frm.body.status.RPT2Call, &dvap_pkt.dstar.b_bone.dstar_udp.rf_header.RPT2Call, 36);
        }
        memset (&STATUS_Frm.ip_addr, 0x00, 16);
	memcpy (&STATUS_Frm.ip_addr, &Gw_IP, 4);   /* IP V4 */
        #if __WORDSIZE == 64
        time (&STATUS_Frm.EntryUpdateTime);
        #else
        time (&STATUS_Frm.EntryUpdateTime);
        memset (&STATUS_Frm.dummy_t, 0x00, 4);
        #endif
        memset (&STATUS_Frm.body.status.ShortMessage, 0x20, 20);
        //memset (&STATUS_Frm.body.status.Latitude, 0x00, 16);
        STATUS_Frm.body.status.Latitude = 3600000;
        STATUS_Frm.body.status.Longitude = 3600000;
        memset (&STATUS_Save, 0x00, 100);
        STATUS_Frm.port = 0;
        sendto (dv_status.status_sd, &STATUS_Frm, 100, 0,
               	dv_status.status_info->ai_addr, dv_status.status_info->ai_addrlen);
        dv_status.packets++;
        memcpy (&STATUS_Save, &STATUS_Frm, 100);
}

void	status_send_pttoff (void)
{
        memcpy (&STATUS_Frm.StatusID, "DSTRST", 6);
        memcpy (&STATUS_Frm.Type, "06", 2);
        memset (&STATUS_Frm.ip_addr, 0x00, 16);
	memcpy (&STATUS_Frm.ip_addr, &Gw_IP, 4);   /* IP V4 */
        #if __WORDSIZE == 64
        time (&STATUS_Frm.EntryUpdateTime);
        #else
        time (&STATUS_Frm.EntryUpdateTime);
        memset (&STATUS_Frm.dummy_t, 0x00, 4);
        #endif
       	sendto (dv_status.status_sd, &STATUS_Frm, 100, 0,
       		dv_status.status_info->ai_addr,
                dv_status.status_info->ai_addrlen);
                dv_status.packets++;
        memcpy (&STATUS_Save, &STATUS_Frm, 100);
}

void	status_send_update (struct dv_packet *pnt)
{
	if (pnt->dstar.b_bone.b_b.seq == 0)
        {
        	memcpy (&STATUS_Frm.StatusID, "DSTRST", 6);
                memcpy (&STATUS_Frm.Type, "07", 2);
                memset (&STATUS_Frm.ip_addr, 0x00, 16);
		memcpy (&STATUS_Frm.ip_addr, &Gw_IP, 4);   /* IP V4 */
                #if __WORDSIZE == 64
                time (&STATUS_Frm.EntryUpdateTime);
                #else
                time (&STATUS_Frm.EntryUpdateTime);
                memset (&STATUS_Frm.dummy_t, 0x00, 4);
                #endif
                if (memcmp (&STATUS_Frm.ip_addr, &STATUS_Save.ip_addr, 84))
                {
                       	sendto (dv_status.status_sd, &STATUS_Frm, 100, 0,
                             dv_status.status_info->ai_addr,
                             dv_status.status_info->ai_addrlen);
                        dv_status.packets++;
                        memcpy (&STATUS_Save, &STATUS_Frm, 100);
                }
	}
}

void	status_short_message (char msg[])
{
	if (node_area_rep_callsign[0] != 0x20)
	{
		memcpy (STATUS_Frm.body.status.ShortMessage, msg, 20);
	}
	if (dvap_area_rep_callsign[0] != 0x20)
	{
		memcpy (STATUS_Frm.body.status.ShortMessage, msg, 20);
	}
}

void	read_status_udp (void)
{
        int     len;
        status_addr_len = sizeof(struct sockaddr_storage);

        if((len = recvfrom(dv_status.status_sd, &upnp_buf, sizeof(upnp_buf), 0,
                (struct sockaddr *)&status_addr, &status_addr_len)) < 0)
        {
                time(&cur_time);
                fprintf (log_file, "%24.24s recvfrom error %s\n",
                        ctime(&cur_time), strerror(errno));
                fflush (log_file);
                FD_CLR (dv_status.status_sd, &save_rfds);
                close (dv_status.status_sd);
                dv_status.status_sd = 0;
                return ;
        }
}


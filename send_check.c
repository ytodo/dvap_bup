#include "dv_ap.h"

int	send_check(char	call_sign[])
{
	time_t	atime;
	int	i;
	struct	SendCheck *send_check_next;
	struct	SendCheck *send_check_save;

	time(&atime);

	send_check_next = send_check_pnt;

	while (send_check_next)
	{
		if (!memcmp (call_sign, send_check_next->CallSign, 10))
		{
			if (atime >= send_check_next->SendTime)
			{	
				time(&send_check_next->SendTime);
				send_check_next->SendTime += aprs_send_interval;
				return TRUE;
			}
			else
			{
				fprintf (log_file, "%24.24s Short interval %8.8s (Remaining %ld Sec.)\n", 
						ctime(&atime), call_sign, send_check_next->SendTime - atime);
				fflush (log_file);
				return FALSE;
			}
		}
		send_check_next = send_check_next->next;
	}
	
	send_check_next = malloc (sizeof (struct SendCheck));
	if (send_check_next == NULL)
	{
		fprintf (log_file, "malloc error in send_check\n");
		fflush (log_file);
		return FALSE;
	}
	memcpy (send_check_next->CallSign, call_sign, 10);
	time(&send_check_next->SendTime);
	send_check_next->SendTime += aprs_send_interval;
	send_check_next->next = send_check_pnt;
	send_check_pnt = send_check_next;
	
	send_check_next = send_check_pnt;
	send_check_save = NULL;
	while (send_check_next)
	{
		if (atime >= send_check_next->SendTime)
		{
			if (send_check_save == NULL)
			{
				send_check_pnt = send_check_next->next;
			}
			else
			{
				send_check_save->next = send_check_next->next;
			}
			free (send_check_next);
			break;
		}
		send_check_save = send_check_next;
		send_check_next = send_check_next->next;	
	}
	return TRUE;	
}


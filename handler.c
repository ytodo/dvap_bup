#include "dv_ap.h"

void	handler (int sig)
{

	if (sig == SIGTERM) sig_term = TRUE;
	else if (sig == SIGINT)  sig_term = TRUE;
}

int	handler_init(void)
{
	time (&cur_time);
        if (signal(SIGTERM, handler) == SIG_ERR)
        {
                fprintf (log_file, "%24.24s signal (SIGTERM) error\n", ctime(&cur_time));
                fflush (log_file);
                return FALSE;
        }
        if (signal(SIGINT, handler) == SIG_ERR)
        {
                fprintf (log_file, "%24.24s signal (SIGINT) error\n", ctime(&cur_time));
                fflush (log_file);
                return FALSE;
        }

	return TRUE;
}

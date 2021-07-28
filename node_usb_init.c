#include	"dv_ap.h"

void	node_usb_init(void)
{
	int	ret;

	if (node_area_rep_callsign[0] == 0x20) 
	{
		node_sw = FALSE;
		return;
	}

	udev = NULL;
	dev_found = FALSE;
	usb_init();
	usb_find_busses();
	usb_find_devices();

        for (bus = usb_get_busses(); bus && !dev_found; bus = bus->next) {
                for (dev = bus->devices; dev && !dev_found; dev = dev->next) {
                  if ((dev->descriptor.idVendor == VenderID) 
			&& (dev->descriptor.idProduct == ProductID))
                  {
                        udev = usb_open(dev);
			dev_found = TRUE;
                        break;
                  }
                }
        }

        if (!dev_found)
	{
		if (node_sw)
		{
			time (&cur_time);
               		fprintf(log_file, 
				"\n%24.24s Please check the USB Cable! or Node Adapter!\n", 
				ctime(&cur_time));
			fflush (log_file);
		}
		node_term();
                return;
        }

        if (usb_set_configuration (udev, dev->config->bConfigurationValue) < 0)
	{
		time (&cur_time);
		fprintf(log_file, "\n%24.24s Configuratime Error for Node Adapter!\n", ctime(&cur_time));
		fflush (log_file);
		node_close();
		return;
	}

        if (debug)
        {
                time (&cur_time);
                fprintf (log_file, "%24.24s Node Adapter VenderID:0x%4.4x ProductID:0x%4.4x\n",
                        ctime(&cur_time), VenderID, ProductID);
                fflush (log_file);
        }

	if (debug) print_PICinfo();

	usb_read_cnt = 0;
	node_sw = TRUE;
	return;
}


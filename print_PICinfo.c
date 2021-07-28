#include	"dv_ap.h"

void    print_PICinfo (void)
{
	char	pic_buff[32];
        int     ret;
        int     i;
	char	status;
	char	mode;
	char	mode2;
	unsigned char	delay;

	time (&cur_time);
        fprintf (log_file, "%24.24s NODE PIC version: ", ctime(&cur_time));
        ret = 8;
        while (ret == 8)
        {
                ret = usb_control_msg(udev, 0xC0, GET_VERSION, 0, 0, pic_buff, 8, 200);
                for (i = 0 ; i < ret ; i++)
                {
                        fprintf (log_file, "%c",pic_buff[i]);
                }
        }
        fprintf (log_file, "\n");
                ret = usb_control_msg(udev, 0xC0, GET_USERID, 0, 0, pic_buff, 8, 200);
        if (ret == 8)
        {
                pic_buff[8] = 0x20;
                for (i = 0 ; i < 8 ; i++)
                {
                        if (pic_buff[i] == 0x20)
                        {
                                pic_buff[i] = '.';
				pic_buff[i+1] = 0x00;
                                break;
                        }
                }
                fprintf (log_file, "%24.24s NODE This PIC program is licensed to %s\n", 
			ctime(&cur_time), pic_buff);
        }

        ret = usb_control_msg(udev, 0xC0, GET_SERIAL_NO, 0, 0, pic_buff, 8, 200);
        if (ret == 8)
        {
                pic_buff[8] = 0x20;
                for (i = 7 ; i > 0 ; i--)
                {
                        if (pic_buff[i] != 0x20)
                        {
                                pic_buff[i+1] = '.';
				pic_buff[i+2] = 0x00;
                                break;
                        }
                }
                fprintf (log_file, "%24.24s NODE Serial Number :  %s\n", ctime(&cur_time), pic_buff);
        }

	usb_control_msg(udev, 0xC0, GET_DelayTime, 0, 0, (char *)&delay, 1, 100);
	fprintf (log_file, "%24.24s NODE Delay time %d mSec.\n", ctime(&cur_time), delay*10);

	usb_control_msg(udev, 0xC0, GET_INVERT_STATUS, 0, 0, &status, 1, 100);

	fprintf (log_file, "%24.24s NODE TX Invert ", ctime(&cur_time));
	printOnOff (status & TX_INV);

	fprintf (log_file, "%24.24s NODE RX Invert ", ctime(&cur_time));
	printOnOff (status & RX_INV);

	fprintf (log_file, "%24.24s NODE Auto RX Invert ", ctime(&cur_time));
	printOnOff (status & AutoRXDetect);

	usb_control_msg(udev, 0xC0, GET_MODE, 0, 0, &mode, 1, 100);
	fprintf (log_file, "%24.24s NODE CRC Check ", ctime(&cur_time));
	printOnOff (mode & CRC_SW);

	fprintf (log_file, "%24.24s NODE COS Check ", ctime(&cur_time));
	printOnOff (mode & COS_SW);

	usb_control_msg(udev, 0xC0, GET_MODE2, 0, 0, &mode2, 1, 100);
	fprintf (log_file, "%24.24s NODE Header Gen. ", ctime(&cur_time));
	printOnOff (mode2 & HeaderGen);

	fprintf (log_file, "%24.24s NODE Alter Header ", ctime(&cur_time));
	printOnOff (mode2 & AlterHeader);


	fflush (log_file);
        return;
}


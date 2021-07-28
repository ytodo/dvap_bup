#include "crc.h"

/*
	CRC reslut routine
 */

unsigned short int		result_crc_dstar(unsigned short int crc)
{

    unsigned short int tmp;

      crc = ~crc;
      tmp = crc;
      crc = (crc << 8) | (tmp >> 8 & 0xff);

    return crc;

}  /* result_crc_dstar */


/*
	CRC update routine
 */
unsigned short int	update_crc_dstar( unsigned short int  crc, unsigned char c )
{
    unsigned short int tmp, short_c;

    short_c  = 0xff &  c;
	
    tmp = (crc & 0xff) ^ short_c;
    crc = (crc >> 8)  ^ crc_tabccitt[tmp];
    return crc;

}  /* update_crc_dstar */

unsigned short int crc_calc (unsigned char string[], int length)
{
        unsigned short int        temp;
        int     n;

        temp = 0xffff;

        for (n = 0 ; n < length ; n++)
        {
                temp = update_crc_dstar(temp, string[n]);
        }

        return result_crc_dstar (temp);
}

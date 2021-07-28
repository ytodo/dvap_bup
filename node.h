/*  Header file for Node adapter */
/*              Satoshi Yasuda   */

//#include <libusb-1.0/libusb.h>

//libusb_context	*node_ctx;
//libusb_device	**node_devs;

#define SET_AD_INIT             0x00
#define SET_TimeOut             0x01
#define SET_DelayTime           0x02
#define SET_KeepAlive           0x03
#define SET_RESYNC_ERROR_BITS   0x04
#define SET_PTT                 0x05
#define SET_COS                 0x06
#define SET_CRC_CHECK           0x07
#define SET_LastFrame           0x0a
#define SET_SD_CONV             0x0b
#define SET_RAW                 0x0c
#define SET_DEBUG               0x0d
#define SET_HalfFull            0x0e
#define SET_AD_RESET            0x0f
#define PUT_DATA                0x10
#define GET_DATA                0x11
#define GET_HEADER              0x21
#define GET_AD_STATUS           0x30
#define SET_MyCALL              0x40
#define SET_MyCALL2             0x41
#define SET_YourCALL            0x42
#define SET_RPT1CALL            0x43
#define SET_RPT2CALL            0x44
#define SET_FLAGS               0x45
#define SET_MyRPTCALL           0x46
#define SET_TXCALL              0x47
#define GET_REMAINSPACE         0x50
#define GET_TimeOut             0x51
#define GET_DelayTime           0x52
#define GET_KeepAlive           0x53
#define GET_MyRPTCALL           0x55
#define GET_MODE                0x56
#define	GET_MODE2		0x57
#define	GET_INVERT_STATUS	0xd0
#define	SET_RX_INVERT		0xd1
#define	SET_TX_INVERT		0xd2
#define	SET_AutoRXDetect	0xd3
#define FW_UPDATE               0xFC
#define GET_SERIAL_NO           0xFD
#define GET_USERID              0xFE
#define GET_VERSION             0xFF

#define GET_SN_VALUE            0x70    /* CMX589A S/N   V05.xx only */



#define ON      1
#define OFF     0

/* Mode definition      */
#define CRC_SW          0x02
#define COS_SW          0x04
#define LastFrame_SW    0x08

/* Mode2 defineition 	*/
#define	HeaderGen	0x01
#define	HeaderGenType	0x02
#define	AlterHeader	0x80


/* AD_STATUS definition */
#define READ_RF_HEADER  0x01
#define COS_OnOff       0x02
#define CRC_ERROR       0x04
#define LastFrameRead   0x08
#define HeaderDecodeDone        0x10
#define PTT_OnOff       0x20
#define	RxBufferEmpty	0x40
#define	NoHeader	0x80

/* Inverter Switch */
#define	TX_INV		0x01	
#define RX_INV		0x02
#define	AutoRXDetect	0x80

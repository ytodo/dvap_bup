#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<string.h>
#include	<sys/time.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<usb.h>
#include	<sys/utsname.h>
#include	<signal.h>
#include	<sys/select.h>
#include	<netinet/tcp.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<errno.h>
#include	<netdb.h>
#include	<time.h>
#include	<ctype.h>
#include	<fcntl.h>
#include	<termios.h>
#include	<linux/tty.h>
#include	<sys/ioctl.h>
#include	<net/if.h>
#include	<sys/select.h>
#include	"config.h"
#include	"node.h"
#include	"dstar.h"
#include	"packet.h"

#define		CONFIG_FILE	"/opt/dstar/conf/dv_ap.conf"
#define		ACC_FILE	"/opt/dstar/conf/acc.conf"
#define		LOG_FILE	"/var/log/dv_ap.log"
#define		PID_FILE	"/var/run/dv_ap.pid"

#define CSS_FILE        "/opt/dstar/web/dstar.css"
#define LOGO_FILE       "/opt/dstar/web/logo.png"
#define JS_FILE         "/opt/dstar/web/dstar.js"

#define		TRUE	1
#define		FALSE	0
#define		RECHECK	15

#define		ALLOW	1
#define		DENY	0
#define		APRS	2

#define		DVAP_DEVICE	"/dev/IDxxPlus"	/* changed /dev/dvap -> /dev.IDxxPlus */
#define		DVAP_SPEED	B230400

#define         IDxxPlus_DEVICE "/dev/IDxxPlus"
#define         IDxxPlus_SPEED  B38400

#define		LOGD_PORT	30000
#define		TRUST_PORT	30001
#define		GATEWAY_PORT	40000
#define		STATUS_PORT	21050
#define		TRUST_TIMEOUT	500

//#define		NicDeviceInitValue	"eth0"

#define APRS_SEND       0x01
#define APRS_SHORT      0x02
#define APRS_SKIP       0x03
#define ARPS_AUTO       0x04
#define APRS_NG         0x05
#define APRS_INVALID    0x06
#define APRS_ERROR      0x07


void	print_PICinfo(void);
void    header_read_from_rig(void);
int	voice_read_from_rig(void);
void	last_frame_send(void);
void    header_send(struct dv_header header);
void    voice_send(char voice[]);
void	last_send(char	voice[]);
int	cos_check(void);
int     htoi (const char *s);
void	send_response(void);
void	header_send_set_from_rig(void);
void	printOnOff( char sw);
void    putFifo (int len, struct dv_packet pkt);
int     getFifo (int *len, struct dv_packet *pkt);
void    aprs_putFifo (int len, char pkt[]);
int     aprs_getFifo (int *len, char pkt[]);
void	node_usb_init(void);
void	dvap_open(void);
void	dvap_close(void);
void	node_close (void);
void	node_term(void);
void    dv_pkt_set(struct dv_packet *hdr);
void	node_header_send (char header[]);
void	node_voice_send (char voice[]);
void	node_last_send (char voice[]);
void	dvap_header_send (char header[]);
void	dvap_voice_send (char voice[]);
void	dvap_last_send (char voice[]);

time_t	start_time;
time_t	keep_alive;
int	keep_alive_interval;

usb_dev_handle *udev;
union
{
	struct	dv_header	hdr;
	char	buffer[41];
} usb;

struct	dv_header	node_save_hdr;

char	status;
int	CrcSW, Notice;
int	HeaderLength;
int	voice_pnt;
char	seq;
char	IDxxPlus_seq;

struct	dv_packet	dv_pkt;
struct	dv_packet	node_pkt;
struct	dv_packet	dvap_pkt;
struct	dv_packet	IDxxPlus_pkt;
struct	dv_packet	reply_dv_pkt;

struct	dv_packet	node_pkt_header;
struct	dv_packet	dvap_pkt_header;
struct	dv_packet	IDxxPlus_pkt_header;

struct	dv_packet	echo_dv_pkt;
struct	dv_packet	echo_posit;

int	in_sd;
int	upnp_udp_sd;
int	upnp_http_sd;

int	logd_sd;

int	httpd_sd;

struct	sockaddr_storage	http_recv;
socklen_t	http_recv_len;	

char	voice_save[24];

char	node_area_rep_callsign[8];
char	dvap_area_rep_callsign[8];
char	gateway_callsign[8];
char	echo_server[8];
char	echo_area_rep_callsign[8];
char	rep_ip_addr[20];
FILE	*log_file;
FILE	*pid_file;
FILE	*acc_file;

int	debug;

struct utsname      uname_buf;
time_t	cur_time;
int	sig_term;
char	trust_name[128];
unsigned int	gateway_port;
int	gateway_src_port;
int	trust_port;
int	logd_port;
int	http_port;
int	node_last_frame_sw;
int	dvap_last_frame_sw;
int	IDxxPlus_last_frame_sw;
int	send_null_sw;
struct	dv_header	node_gw_resp;
struct	dv_header	dvap_gw_resp;
struct	dv_header	IDxxPlus_gw_resp;
int	node_gw_resp_sw;
int	dvap_gw_resp_sw;
int	IDxxPlus_gw_resp_sw;
struct	timeval	dvap_in_time;
struct	timeval aprs_SendTime;
int	aprs_ptt_onoff;
int	aprs_rf_send;

struct	addrinfo	hints;
struct	addrinfo	*gateway_in;
struct	addrinfo	*gateway_out;
struct	addrinfo	*trust_sock;
struct	addrinfo	*upnp_sock;
struct	addrinfo	*upnp_bind_sock;
struct	addrinfo	*logd_sock;
struct	addrinfo	*aprs_sock;
struct	addrinfo	*http_serv;
char	PORT[8];

struct	sockaddr_storage	in_addr;
socklen_t	in_addr_len;

struct	sockaddr_storage	in_upnp_addr;
socklen_t	in_upnp_addr_len;
char    upnp_ip_addr[64];

struct  sockaddr_storage        status_addr;
socklen_t       status_addr_len;
char    status_ip_addr[64];

char		node_save_frame_id[2];
char		dvap_save_frame_id[2];
char		IDxxPlus_save_frame_id[2];
char		dvap_squelch_status;

struct	sockaddr_storage	mon_addr;
socklen_t			mon_addr_len;

struct	sockaddr_storage	upnp_addr;
socklen_t			upnp_addr_len;

struct  usb_bus *bus;
struct  usb_device      *dev;
int     dev_found;

union
{
	char	buf[2048];
	struct	dv_packet	buf_pkt;
} buf_pkt;

/* for pselect */
struct	timespec	tv;
fd_set  rfds;
fd_set	save_rfds;
sigset_t	sigset;
sigset_t	save_sigset;

struct	timeval	NodeCosOffTime;
struct	timeval DvapCosOffTime;
struct	timeval IDxxPlusCosOffTime;
struct	timeval	NodeActiveTime;
struct	timeval	DvapActiveTime;
struct	timeval IDxxPlusActiveTime;
struct	timeval	Node_InTime;
int	node_NoRespReply_sw;
int	dvap_NoRespReply_sw;
int	IDxxPlus_NoRespReply_sw;
struct	dv_header	node_NoResp;
struct	dv_header	dvap_NoResp;
int	usb_read_cnt;
int	loop_cnt;

long	int	node_rig_pkt_cnt;
long	int	dvap_rig_pkt_cnt;
long	int	IDxxPlus_rig_pkt_cnt;
long	int	node_inet_pkt_cnt;
long	int	dvap_inet_pkt_cnt;
long	int	IDxxPlus_inet_pkt_cnt;
long	int	echo_pkt_cnt;

long    int     node_rig_pkt_total_cnt;
long    int     dvap_rig_pkt_total_cnt;
long	int	IDxxPlus_rig_pkt_total_cnt;
long    int     node_inet_pkt_total_cnt;
long    int     dvap_inet_pkt_total_cnt;
long	int	IDxxPlus_inet_pkt_total_cnt;

/* node adapter */
int	VenderID;
int	ProductID;

/* misc switch */


struct	FifoPkt
{
	struct	FifoPkt	*next;
	int	length;
	struct	dv_packet	pkt;
};

struct	aprsFifoPkt
{
	struct aprsFifoPkt	*next;
	int	length;
	char	data[41];
};

struct	FifoPkt *Rp;
struct	FifoPkt *Wp;

struct  aprsFifoPkt *aprs_Rp;
struct  aprsFifoPkt *aprs_Wp;

int	init_sw;
int	dvap_voice_send_sw;
int	node_voice_send_sw;
int	IDxxPlus_voice_send_sw;
int	rep_position_send_sw;

int	dvap_fd;

time_t	dvap_keep_alive;

int	dvap_buff_pnt;
struct	timeval	dvap_InTime;

int	node_sw;
int	dvap_sw;
long	int	dvap_freq;
long	int	dvap_rx_freq;
long	int	dvap_tx_freq;
signed	char 	dvap_squelch;
short	int	dvap_calibration;
int	dvap_auto_calibration;
int	dvap_auto_calibration_set;
time_t	re_check;

int	get_position_sw;

unsigned char	gateway_position[42];
struct	timeval	req_time;

/* DPRS */
int	aprs_sd;
char	aprs_server[128];
char	aprs_srv[16];
long int	aprs_cnt;
long int	aprs_beacon_cnt;
int	verify_sw;
int	aprs_port;
int	AutoReLink;
int	BeaconInterval;
time_t	BeaconTime;
int	aprs_send_interval;
char	DprsTemp[2048];
long int	BeaconLat;
long int	BeaconLong;
char	radio_id;
char	client_callsign[8];
char	beacon_comment[64];
char	aprs_filter[256];


struct	inet_short_msg
{
	unsigned char	mini_header;
	char		temp[5];
	char		short_msg[20];
};

struct	inet_short_msg	dvap_inet_msg;
struct	inet_short_msg	node_inet_msg;
struct	inet_short_msg	IDxxPlus_inet_msg;

struct	aprs_msg
{
	int	msg_pnt;
	int	qsy_sw;
	char	aprs_msg[256];
	char	aprs_msg_save[256];
	char	short_msg[20];
	char	RadioGpsStatus;
        unsigned char   RadioLat[8];
        unsigned char   RadioLong[9];
        unsigned char   RadioCall[8];
        unsigned char   RadioMsg[20];
        unsigned char   RadioAtitude[6];
        unsigned char   RadioSpeed[3];
        unsigned char   RadioDirection[3];
        unsigned char   RadioTime[7];
	unsigned char	AprsSend;
	struct	
	{
		unsigned char mini_header;
		char	temp[5];
	} tmp;
};

struct	aprs_msg	node_msg;
struct	aprs_msg	dvap_msg;

unsigned char   GpsMsg[20];

struct SendCheck
{
	struct	SendCheck	*next;
        unsigned        char    CallSign[10];
        time_t                  SendTime;
};

struct SendCheck	*send_check_pnt;


char	NicDevice[IFNAMSIZ];
struct	in_addr OwnDvApIP;

int	upnp_sw;
char	upnp_buf[4096];
int	upnp_buf_pnt;
time_t	upnp_send_time;
int	upnp_add_sw;
int	upnp_start_pnt;
int	upnp_check_cnt;

int	trust_timeout;	

struct	tm	*gt;
char	aprs_ip[NI_MAXHOST];

time_t	node_recv_time;
time_t	dvap_recv_time;
time_t	node_inet_recv_time;
time_t	dvap_inet_recv_time;

char	Gw_IP[4];

struct	in_addr	trust_ip;

/* Jitter buffer for Echo server */
int	jitter_out_pnt;
int	jitter_cnt;
char	jitter[21][12];

/* Jitter buffer */
struct  echo
{
        struct  echo    *next;
        long    int     in_cnt;
        long    int     out_cnt;
        struct  timeval send_time;
        struct  timeval EchoInTime;
        FILE    *tmp_file;
        int     echo_state;
        int     jitter_out_pnt;
        int     jitter_cnt;
        int     seq;
        char    jitter[21][12];
        char    callsign[8];
        char    frame_id[2];
        char    recv_seq;
        char    msg[20];
        char    mini_header;
        char    msg_tmp[5];
};

struct  echo    *echo_pnt;

int     send_echo_skip;

time_t  echo_position_send_interval;
time_t  echo_position_send_time;

/* status */
struct  STATUS_frame    STATUS_Frm;
struct  STATUS_frame    STATUS_Save;

time_t          status_keep_alive;
int	status_port;

struct  status
{
        char                    fqdn[128];
        unsigned char           userID[16];
        unsigned char           passwd[64];
        in_port_t               port;
        int                     status_sd;
        struct  addrinfo        *status_info;
        long int                packets;
};

struct	status	dv_status;

struct STATUS_Login_frame
{
        unsigned char   StatusID[6];    /* "DSTRST" */
        unsigned char   Type[2];        /* 00 */
#if __WORDSIZE == 64
        time_t          EntryUpdateTime;
#else
        time_t          EntryUpdateTime;
        time_t          dummy_t;
#endif
        unsigned char   UserID[16];
        unsigned char   Passwd[64];
        unsigned char   reserve[4];
};

int	status_start;

int	qsy_info;

/* IDxxPlus */
struct  FifoPkt *IDxxPlusRp;
struct  FifoPkt *IDxxPlusWp;

int	IDxxPlus_fd;
int	IDxxPlusFifo_cnt;
int	IDxxPlus_send_sw;
time_t	IDxxPlus_init_time;
time_t	IDxxPlus_alive_recv;
struct	termios save_attr;
int	IDxxPlus_buff_pnt;
int	IDxxPlus_sw;
char	IDxxPlus_area_rep_callsign[8];

int	IDxxPlus_frame_seq;
struct	timeval	IDxxPlus_send_time;


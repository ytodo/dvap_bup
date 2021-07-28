/*  Header file for D-STAR      */
/*              Satoshi Yasuda  */
/*              7m3tjz/ad6gz    */

struct  dv_header
{
	char	flags[3];
	char	RPT2Call[8];
	char	RPT1Call[8];
	char	YourCall[8];
	char	MyCall[8];
	char	MyCall2[4];
	char	CRC[2];
};

struct	voice_data
{
	unsigned char	voice_segment[9];
	unsigned char	data_segment[3];
	unsigned char	lastframe[3];
};

struct	back_bone
{
	unsigned char	id;
	unsigned char	dest_repeater_id;
	unsigned char	send_repeater_id;
	unsigned char	send_terminal_id;
	unsigned char	frame_id[2];
	unsigned char	seq;
};

struct	posit
{
	char	call1[8];
	char	call2[8];
};

struct	b_bone
{
	struct	back_bone b_b;
	union
	{
		struct	dv_header rf_header;
		struct	voice_data voice_d;
	} dstar_udp;
};	

struct	dv_packet
{
	unsigned char	id[4];
	unsigned char	pkt_type;	/* 0x10 header 0x20 voice */
	unsigned char	hole_punch;
	unsigned char	filler[2];
	union
	{
		struct	b_bone	b_bone;
		struct	posit	posit;
	} dstar;
};

struct	aprs_send_data
{
	union
	{
		struct	dv_header	hdr;
		struct	voice_data	voice;
	} aprs_send_d;
};

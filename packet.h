struct	in4_addr
{
	in_addr_t	v4_address;
	char		dummy[12];
};
	
struct STATUS_frame
{
        unsigned char   StatusID[6];    /* "DSTRST" */
        unsigned char   Type[2];
#if __WORDSIZE == 64
	time_t		EntryUpdateTime;
#else
	time_t		EntryUpdateTime;
	time_t		dummy_t;
#endif
	union
	{
		struct	in6_addr ip_addr_v6;
		struct	in4_addr ip_addr_v4;
	} ip_addr;	
	in_port_t	port;
	unsigned char	reserve[2];
	union
	{
		struct
		{	
			unsigned char	RPT2Call[8];
			unsigned char	RPT1Call[8];
			unsigned char	UrCall[8];
			unsigned char	MyCall[8];
			unsigned char	MyCall2[4];
			unsigned char	ShortMessage[20];
			int		Latitude;
			int		Longitude;
		} status;
		struct
		{
			unsigned char	ModuleName[8];
			unsigned char	gateway_callsign[8];
			unsigned char	Version[48];
		} keep_alive;
		struct
		{
			unsigned char	dv_ap_callsign[8];
			unsigned char	reserve[56];
		} logoff;
	} body;
};

union
{
	unsigned char	Connect[5];
	unsigned char	KeepAlive[3];
	struct
	{
		unsigned char	id[4];
		unsigned char	callsign[8];
		unsigned char	pin[8];
		unsigned char	version[8];
	} verifyReq;
	struct	
	{
		unsigned char	id[4];
		unsigned char	OK[2];
		unsigned char	RW[2];
	} verifyResp;

} packet;


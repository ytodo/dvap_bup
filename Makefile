#########################################################################
#  Makefile for compiling dv_ap
#
#  Arranged by je3hcz based on 7m3tjz's original
#########################################################################

# Program Name and Object file names
bin_PROGRAMS    = dv_ap
dv_ap_OBJECTS   = main.o aprs2dstar.o aprs.o beacon.o crc.o dvap.o \
                  dv_utils.o echo_server.o from_inet.o from_node.o \
                  gps_a.o gps.o handler.o httpd_srv.o IDxxPlus.o init.o \
                  main.o node_usb_init.o pass_send.o print_PICinfo.o \
                  read_config.o reply_busy.o send_check.o send_inet.o \
                  send_node.o send_resp.o status.o upnp.o

# Redefine MACROs
CC              = gcc
dv_ap_LIBS      = -lusb -lcrypto

# Define extention of Suffix Rules
.SUFFIXES   : .c .o

# Rule of compiling programs
$(bin_PROGRAMS) : $(dv_ap_OBJECTS)
	$(CC) $(dv_ap_LIBS) -o $(bin_PROGRAMS) $^

# Suffix Rule
.c.o    :
	$(CC) -c $<

# Target of Delete files
.PHONY  : clean
clean   :
	$(RM) $(bin_PROGRAMS) $(dv_ap_OBJECTS)

# Dependency of Header Files
main.o          : dv_ap.h
aprs.o          : dv_ap.h
beacon.o        : dv_ap.h
crc.o           : crc.h
dvap.o          : dv_ap.h
dv_utils.o      : dv_ap.h
echo_server.o   : dv_ap.h
from_inet.o     : dv_ap.h
from_node.o     : dv_ap.h
gps_a.o         : dv_ap.h
gps.o           : dv_ap.h dprs_symbol.h
handler.o       : dv_ap.h
httpd_srv.o     : dv_ap.h config.h
IDxxPlus.o      : dv_ap.h
init.o          : dv_ap.h
main.o          : dv_ap.h
node_usb_init.o : dv_ap.h
pass_send.o     : dv_ap.h
print_PICinfo.o : dv_ap.h
read_config.o   : dv_ap.h
reply_busy.o    : dv_ap.h
send_check.o    : dv_ap.h
send_inet.o     : dv_ap.h
send_node.o     : dv_ap.h
send_resp.o     : dv_ap.h
status_send.o   : dv_ap.h config.h
upnp.o          : dv_ap.h


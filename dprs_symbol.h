unsigned char aprs_symbol[][2][4] = {
{"/!  ","BB  "}, //   Police, Sheriff           
{"/\"  ","BC  "}, //   reserved  (was rain)      
{"/#  ","BD  "}, //   DIGI (white center)       
{"/$  ","BE  "}, //   PHONE                     
{"/%  ","BF  "}, //   DX CLUSTER
{"/&  ","BG  "}, //   HF GATEway
{"/'  ","BH  "}, //   Small AIRCRAFT (SSID = 7)
{"/(  ","BI  "}, //   Mobile Satellite Station
{"/)  ","BJ  "}, //   Wheelchair (handicapped)
{"/*  ","BK  "}, //   SnowMobile
{"/+  ","BL  "}, //   Red Cross
{"/,  ","BM  "}, //   Boy Scouts
{"/-  ","BN  "}, //   House QTH (VHF)
{"/.  ","BO  "}, //   X
{"//  ","BP  "}, //   Red Dot
{"/0  ","P0  "}, //   # circle (obsolete)
{"/1  ","P1  "}, //   TBD (these were numbered)
{"/2  ","P2  "}, //   TBD (circles like pool)
{"/3  ","P3  "}, //   TBD (balls.  But with)
{"/4  ","P4  "}, //   TBD (overlays, we can)
{"/5  ","P5  "}, //   TBD (put all #'s on one)
{"/6  ","P6  "}, //   TBD (So 1-9 are available)
{"/7  ","P7  "}, //   TBD (for new uses?)
{"/8  ","P8  "}, //   TBD (They are often used)
{"/9  ","P9  "}, //   TBD (as mobiles at events)
{"/:  ","MR  "}, //   FIRE
{"/;  ","MS  "}, //   Campground (Portable ops)
{"/<  ","MT  "}, //   Motorcycle     (SSID =10)
{"/=  ","MU  "}, //   RAILROAD ENGINE
{"/>  ","MV  "}, //   CAR            (SSID = 9)
{"/?  ","MW  "}, //   SERVER for Files
{"/@  ","MX  "}, //   HC FUTURE predict (dot)
{"/A  ","PA  "}, //   Aid Station
{"/B  ","PB  "}, //   BBS or PBBS
{"/C  ","PC  "}, //   Canoe
{"/D  ","PD  "}, // 
{"/E  ","PE  "}, //   EYEBALL (Eye catcher!)
{"/F  ","PF  "}, //   Farm Vehicle (tractor)
{"/G  ","PG  "}, //   Grid Square (6 digit)
{"/H  ","PH  "}, //   HOTEL (blue bed symbol)
{"/I  ","PI  "}, //   TcpIp on air network stn
{"/J  ","PJ  "}, // 
{"/K  ","PK  "}, //   School
{"/L  ","PL  "}, //   PC user (Jan 03)
{"/M  ","PM  "}, //   MacAPRS
{"/N  ","PN  "}, //   NTS Station
{"/O  ","PO  "}, //   BALLOON        (SSID =11)
{"/P  ","PP  "}, //   Police
{"/Q  ","PQ  "}, //   TBD
{"/R  ","PR  "}, //   REC. VEHICLE   (SSID =13)
{"/S  ","PS  "}, //   SHUTTLE
{"/T  ","PT  "}, //   SSTV
{"/U  ","PU  "}, //   BUS            (SSID = 2)
{"/V  ","PV  "}, //   ATV
{"/W  ","PW  "}, //   National WX Service Site
{"/X  ","PX  "}, //   HELO           (SSID = 6)
{"/Y  ","PY  "}, //   YACHT (sail)   (SSID = 5)
{"/Z  ","PZ  "}, //   WinAPRS
{"/[  ","HS  "}, //   Human/Person (HT)
{"/\\  ","HT  "}, //   TRIANGLE(DF station)
{"/]  ","HU  "}, //   MAIL/PostOffice(was PBBS)
{"/^  ","HV  "}, //   LARGE AIRCRAFT
{"/_  ","HW  "}, //   WEATHER Station (blue)
{"/`  ","HX  "}, //   Dish Antenna
{"/a  ","LA  "}, //   AMBULANCE     (SSID = 1)
{"/b  ","LB  "}, //   BIKE          (SSID = 4)
{"/c  ","LC  "}, //   Incident Command Post
{"/d  ","LD  "}, //   Fire dept
{"/e  ","LE  "}, //   HORSE (equestrian)
{"/f  ","LF  "}, //   FIRE TRUCK    (SSID = 3)
{"/g  ","LG  "}, //   Glider
{"/h  ","LH  "}, //   HOSPITAL
{"/i  ","LI  "}, //   IOTA (islands on the air)
{"/j  ","LJ  "}, //   JEEP          (SSID-12)
{"/k  ","LK  "}, //   TRUCK         (SSID = 14)
{"/l  ","LL  "}, //   Laptop (Jan 03)  (Feb 07)
{"/m  ","LM  "}, //   Mic-E Repeater
{"/n  ","LN  "}, //   Node (black bulls-eye)
{"/o  ","LO  "}, //   EOC
{"/p  ","LP  "}, //   ROVER (puppy, or dog)
{"/q  ","LQ  "}, //   GRID SQ shown above 128 m
{"/r  ","LR  "}, //   Repeater         (Feb 07)
{"/s  ","LS  "}, //   SHIP (pwr boat)  (SSID-8)
{"/t  ","LT  "}, //   TRUCK STOP
{"/u  ","LU  "}, //   TRUCK (18 wheeler)
{"/v  ","LV  "}, //   VAN           (SSID = 15)
{"/w  ","LW  "}, //   WATER station
{"/x  ","LX  "}, //   xAPRS (Unix)
{"/y  ","LY  "}, //   YAGI @ QTH
{"/z  ","LZ  "}, //   TBD
{"/{  ","J1  "}, // 
{"/|  ","J2  "}, //   TNC Stream Switch
{"/}  ","J3  "}, // 
{"/~  ","J4  "}, //   TNC Stream Switch
{"\\!  ","OB  "}, //   EMERGENCY (!)            
{"\\\"  ","OC  "}, //   reserved
{"\\#  ","OD# "}, //  OVERLAY DIGI (green star)
{"\\$  ","OEO "}, //  Bank or ATM  (green box) 
{"\\%  ","OFO "}, //  Power Plant with overlay
{"\\&  ","OG# "}, //  I=Igte R=RX T=1hopTX 2=2hopTX
{"\\'  ","OHO "}, //  Crash (& now Incident sites)
{"\\(  ","OI  "}, //   CLOUDY (other clouds w ovrly)
{"\\)  ","OJO "}, //  Firenet MEO, MODIS Earth Obs.
{"\\*  ","OK  "}, //   SNOW (& future ovrly codes)
{"\\+  ","OL  "}, //   Church
{"\\,  ","OM  "}, //   Girl Scouts
{"\\-  ","ONO "}, //  House (H=HF) (O = Op Present)
{"\\.  ","OO  "}, //   Ambiguous (Big Question mark)
{"\\/  ","OP  "}, //   Waypoint Destination
{"\\0  ","A0# "}, //  CIRCLE (E/I/W=IRLP/Echolink/WIRES)
{"\\1  ","A1  "}, // 
{"\\2  ","A2  "}, // 
{"\\3  ","A3  "}, // 
{"\\4  ","A4  "}, // 
{"\\5  ","A5  "}, // 
{"\\6  ","A6  "}, // 
{"\\7  ","A7  "}, // 
{"\\8  ","A8O "}, //  802.11 or other network node
{"\\9  ","A9  "}, //   Gas Station (blue pump)  
{"\\:  ","NR  "}, //   Hail (& future ovrly codes)                    
{"\\;  ","NSO "}, //  Park/Picnic + overlay events        
{"\\<  ","NTO "}, //  ADVISORY (one WX flag)
{"\\=  ","NUO "}, //  APRStt Touchtone (DTMF users)
{"\\>  ","NV# "}, //  OVERLAYED CAR
{"\\?  ","NW  "}, //   INFO Kiosk  (Blue box with ?)
{"\\@  ","NX  "}, //   HURICANE/Trop-Storm
{"\\A  ","AA# "}, //  overlayBOX DTMF & RFID & XO
{"\\B  ","AB  "}, //   Blwng Snow (& future codes)
{"\\C  ","AC  "}, //   Coast Guard          
{"\\D  ","AD  "}, //   Drizzle (proposed APRStt)
{"\\E  ","AE  "}, //   Smoke (& other vis codes)
{"\\F  ","AF  "}, //   Freezng rain (&future codes)
{"\\G  ","AG  "}, //   Snow Shwr (& future ovrlys)
{"\\H  ","AHO "}, //  \Haze (& Overlay Hazards)
{"\\I  ","AI  "}, //   Rain Shower  
{"\\J  ","AJ  "}, //   Lightening (& future ovrlys)
{"\\K  ","AK  "}, //   Kenwood HT (W)              
{"\\L  ","AL  "}, //   Lighthouse                     
{"\\M  ","AMO "}, //  MARS (A=Army,N=Navy,F=AF) 
{"\\N  ","AN  "}, //   Navigation Buoy          
{"\\O  ","AO  "}, //   Rocket (new June 2004)
{"\\P  ","AP  "}, //   Parking                    
{"\\Q  ","AQ  "}, //   QUAKE                       
{"\\R  ","ARO "}, //  Restaurant                   
{"\\S  ","AS  "}, //   Satellite/Pacsat
{"\\T  ","AT  "}, //   Thunderstorm        
{"\\U  ","AU  "}, //   SUNNY                       
{"\\V  ","AV  "}, //   VORTAC Nav Aid              
{"\\W  ","AW# "}, //  # NWS site (NWS options)
{"\\X  ","AX  "}, //   Pharmacy Rx (Apothicary)
{"\\Y  ","AYO "}, //  Radios and devices
{"\\Z  ","AZ  "}, // 
{"\\[  ","DSO "}, //  W.Cloud (& humans w Ovrly)                 
{"\\\\  ","DTO "}, //  New overlayable GPS symbol
{"\\]  ","DU  "}, // 
{"\\^  ","DV# "}, //  # Aircraft (shows heading)
{"\\_  ","DW# "}, //  # WX site (green digi)
{"\\`  ","DX  "}, //   Rain (all types w ovrly)    
{"\\a  ","SA#O"}, //  ARRL, ARES, WinLINK
{"\\b  ","SB  "}, //   Blwng Dst/Snd (& others)
{"\\c  ","SC#O"}, //  CD triangle RACES/SATERN/etc
{"\\d  ","SD  "}, //   DX spot by callsign
{"\\e  ","SE  "}, //   Sleet (& future ovrly codes)
{"\\f  ","SF  "}, //   Funnel Cloud                
{"\\g  ","SG  "}, //   Gale Flags                     
{"\\h  ","SHO "}, //  Store. or HAMFST Hh=HAM store
{"\\i  ","SI# "}, //  BOX or points of Interest
{"\\j  ","SJ  "}, //   WorkZone (Steam Shovel)
{"\\k  ","SKO "}, //  Special Vehicle SUV,ATV,4x4
{"\\l  ","SL  "}, //   Areas      (box,circles,etc)
{"\\m  ","SM  "}, //   Value Sign (3 digit display)   
{"\\n  ","SN# "}, //  OVERLAY TRIANGLE         
{"\\o  ","SO  "}, //   small circle                    
{"\\p  ","SP  "}, //   Prtly Cldy (& future ovrlys)            
{"\\q  ","SQ  "}, // 
{"\\r  ","SR  "}, //   Restrooms                
{"\\s  ","SS# "}, //  OVERLAY SHIP/boat (top view)
{"\\t  ","ST  "}, //   Tornado                  
{"\\u  ","SU# "}, //  OVERLAYED TRUCK
{"\\v  ","SV# "}, //  OVERLAYED Van
{"\\w  ","SW  "}, //   Flooding                 
{"\\x  ","SX  "}, //   Wreck or Obstruction ->X<-                                
{"\\y  ","SY  "}, //   Skywarn
{"\\z  ","SZ# "}, //  OVERLAYED Shelter 
{"\\{  ","Q1  "}, //   Fog (& future ovrly codes)
{"\\|  ","Q2  "}, //   TNC Stream Switch
{"\\}  ","Q3  "}, // 
{"\\~  ","Q4  "}, //   TNC Stream Switch
{"    ","    "}  //   Terminate 
};



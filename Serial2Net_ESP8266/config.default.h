
//#define USE_WDT
//#define Debug
//#define ntp_intervall 18000     //18 sek. Abfrageintervall not used hier
// IP Address ------------------------------------------------------------------
//#define STATIC_IP   // comment  to enable DHCP

#ifdef STATIC_IP
#define IP_ADDRESS      "10.1.7.41"
#define GATEWAY_ADDRESS "10.1.11.1"
#define NET_MASK        "255.255.0.0"
#endif

// Wifi credentials ------------------------------------------------------------
//#define WIFI_SSID       "HansNet"
//#define WIFI_PASSWORD   "181060_250669"
// wifi credentials wird ersetzt urch Wifi-Manager !!!!
// Server / Client Settings ----------------------------------------------------
#define TCP_LISTEN_PORT 9999
#define BAUD_RATE       115200   //RFLink default speed
#define BUFFER_SIZE     128     // serial end ethernet buffer size

// PINS ------------------------------------------------------------------------
#define WIFI_LED        16  //not used
#define CONNECTION_LED  4   //nLink GPIO4 builtin Led
//#define STATE           4   //State + Builtin-Led ist derselbe Pin !
#define TX_LED          12  //not used
#define RX_LED          13  //not used

/* Configuration of NTP */
#define MY_NTP_SERVER "fritz.box"           
#define MY_TZ "CET-1CEST,M3.5.0/02,M10.5.0/03" //Zeitzone Europa/Wien etc.
//#define MY_TZ "CET-1CEST,M3.5.0,M10.5.0/3"
#define GMZ_2STD_OFFSET 7200                         //+2 Std UTC
#define GMZ_1STD_OFFSET 3600                         //+1 Std UTC
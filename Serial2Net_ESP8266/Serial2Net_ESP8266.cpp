/*
	Serial2Net ESP8266
	https://github.com/soif/Serial2Net_ESP8266
	Copyright 2017 François Déchery

	** Description **********************************************************
	Briges a Serial Port to/from a (Wifi attached) LAN using a ESP8266 board

	** Inpired by ***********************************************************
	* ESP8266 Ser2net by Daniel Parnell
	https://github.com/dparnell/esp8266-ser2net/blob/master/esp8266_ser2net.ino

	* WiFiTelnetToSerial by Hristo Gochkov.
	https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266WiFi/examples/WiFiTelnetToSerial/WiFiTelnetToSerial.ino
*/

// Use your Own Config #########################################################
#include "config.default.h"

// Includes ###################################################################
#include <ESP8266WiFi.h>
#include <time.h>                   // time() ctime()
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <coredecls.h> 				// optional settimeofday_cb() callback to check on server

// Defines #####################################################################
#define MAX_SRV_CLIENTS 		4

/* Globals */
time_t now;                         // this is the epoch
struct tm tm;                       // the structure tm holds time information in a more convient way
uint32_t time_y2k;					// Ab 1 Jänner 2000 Y2K UTC Time
#define NTP_OFFSET 3155673600LL		// für Berechnung Y2K time_t32 = time_t64 - Unix_OFFSET
#define UNIX_OFFSET 946684800LL		// für Berechnung Y2K time_t32 = time_t64 - Unix_OFFSET
// Variables ###################################################################
int last_srv_clients_count=0;

WiFiServer server(TCP_LISTEN_PORT);
WiFiClient serverClients[MAX_SRV_CLIENTS];

void connect_to_wifi();
#ifdef Debug
void showTime();
#endif
void send_tm();
void send_ntp();
void time_is_set(bool from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/);

/*Disable hier ->functions are declared as "weak" functions. You only need to define them and if they are available in your sketch the library will call them !!
uint32_t sntp_update_delay_MS_rfc_not_less_than_15000 () {  // SNTP-Aktualisierungsverzögerung ändern, ohne diese Funktion wird jede Stunde die Zeit geholt.
  return ntp_intervall;                                     // NTP Server Abfrage für diese Demo aller 18 Sekunden
}
*/

uint32_t sntp_startup_delay_MS_rfc_not_less_than_60000 ()
{
  randomSeed(A0);
  return random(60000);//~1Minute
}

// #############################################################################
// Main ########################################################################
// #############################################################################

// ----------------------------------------------------------------------------
void setup(void){
	// Start UART
	Serial.begin(BAUD_RATE);

	//Callback
	settimeofday_cb(time_is_set); 		// optional: callback if time was sent
//Ntp time
	configTime(MY_TZ, MY_NTP_SERVER); 	// --> Here is the IMPORTANT ONE LINER needed in your sketch!

#ifdef USE_WDT
	wdt_enable(1000);
#endif
	//Init State + LED Pin
	digitalWrite(CONNECTION_LED, HIGH);	//Led off
	pinMode(CONNECTION_LED, OUTPUT); 	// GPIO4 as Outputpin

	// Connect to WiFi network
	connect_to_wifi();
	
	// Start server (TCP-Bridge)
	server.begin();
	server.setNoDelay(true);

	#ifdef Debug
	showTime();
	#endif
	digitalWrite(CONNECTION_LED,LOW);	//Led ON 
	/*Avr macht delay >100ms, dann Uart-Init, damit die Connect-Meldungen umgangen werden*/	
}

// ----------------------------------------------------------------------------
void loop(void){

#ifdef USE_WDT
	wdt_reset();
#endif

	// Check Wifi connection -----------------
	if(WiFi.status() != WL_CONNECTED) {
		// we've lost the connection, so we need to reconnect
		for(byte i = 0; i < MAX_SRV_CLIENTS; i++){
			if(serverClients[i]){
				serverClients[i].stop();
			}
		}
		connect_to_wifi();
	}

	// Check if there are any new clients ---------
	uint8_t i;
    if (server.hasClient()){
		for(i = 0; i < MAX_SRV_CLIENTS; i++){
			//find free/disconnected spot
			if (!serverClients[i] || !serverClients[i].connected()){
				if(serverClients[i]){
					serverClients[i].stop();
				}
         		serverClients[i] = server.available();
				//Serial1.print("New client: "); Serial1.print(i);
				continue;
        	}
		}
		// No free/disconnected spot so reject
		WiFiClient serverClient = server.available();
		serverClient.stop();
    }

	//blink according to clients connected ---------
	int srv_clients_count=0;
	for(i = 0; i < MAX_SRV_CLIENTS; i++){
		if (serverClients[i] && serverClients[i].connected()){
			srv_clients_count++;
		}
	}

	if(srv_clients_count != last_srv_clients_count){
		last_srv_clients_count=srv_clients_count;
		//UpdateBlinkPattern(srv_clients_count);
	}

    // check clients for data ------------------------
    for(i = 0; i < MAX_SRV_CLIENTS; i++){
		if (serverClients[i] && serverClients[i].connected()){
        	if(serverClients[i].available()){
    			//get data from the telnet client and push it to the UART
    			while(serverClients[i].available()) {
			  		//led_tx.pulse();
			  		Serial.write(serverClients[i].read());
			  		//led_tx.update();
				}
			}
		}
    }

    // check UART for data --------------------------
    if(Serial.available()){
	      size_t len = Serial.available();
	      uint8_t sbuf[len];
		  Serial.readBytes(sbuf, len);
		  //push UART data to all connected telnet clients
		  for(i = 0; i < MAX_SRV_CLIENTS; i++){
			  if (serverClients[i] && serverClients[i].connected()){
				  //led_rx.pulse();
				  serverClients[i].write(sbuf, len);
				  //led_tx.update();
				  delay(1);
			  }
    	}
    }
}


// Functions ###################################################################

// ----------------------------------------------------------------------------
void connect_to_wifi() {
	WiFi.persistent(false);      //verhindert das bei jedem Start (SSID/Password) in Flash geschrieben wird !
	// WiFiManager
  	// Local intialization. Once its business is done, there is no need to keep it around
  	WiFiManager wifiManager;
	// fetches ssid and pass from eeprom and tries to connect
  	// if it does not connect it starts an access point with the specified name
  	// here  "AutoConnectAP"
  	// and goes into a blocking loop awaiting configuration
  	wifiManager.autoConnect("RolladenAP"); 

#ifdef STATIC_IP
	IPAddress ip_address = parse_ip_address(IP_ADDRESS);
	IPAddress gateway_address = parse_ip_address(GATEWAY_ADDRESS);
	IPAddress netmask = parse_ip_address(NET_MASK);
	WiFi.config(ip_address, gateway_address, netmask);
#endif
	// Wait for WIFI connection
	while (WiFi.status() != WL_CONNECTED) {
#ifdef USE_WDT
		wdt_reset();
#endif
		delay(100);
	}
}

//Callback from Ntp
void time_is_set(bool from_sntp /* <= this optional parameter can be used with ESP8266 Core 3.0.0*/) 
{
  	digitalWrite(CONNECTION_LED, HIGH);		//Led off sonst glimmereffekt bei Disable
	send_ntp();								//send time_t ntp
}

#ifdef Debug
void showTime() {
  time(&now);                       // read the current time
  localtime_r(&now, &tm);           // update the structure tm with the current time
  Serial.print("year:");
  Serial.print(tm.tm_year + 1900);  // years since 1900
  Serial.print("\tmonth:");
  Serial.print(tm.tm_mon + 1);      // January = 0 (!)
  Serial.print("\tday:");
  Serial.print(tm.tm_mday);         // day of month
  Serial.println();
  Serial.print("\thour:");
  Serial.print(tm.tm_hour);         // hours since midnight  0-23
  Serial.print("\tmin:");
  Serial.print(tm.tm_min);          // minutes after the hour  0-59
  Serial.print("\tsec:");
  Serial.print(tm.tm_sec);          // seconds after the minute  0-61*
  Serial.println();
  Serial.print("\twday");
  Serial.print(tm.tm_wday);         // days since Sunday 0-6
  if (tm.tm_isdst == 1)             // Daylight Saving Time flag
    Serial.print("\tDST");
  else
    Serial.print("\tstandard");
  Serial.println();
}
#endif
//send Timestamp(avr-libc compatibel) to Mcu
void send_tm(void)
{	//Reihenfolga ala avr-libc tm
	byte lowb, highb;
	int temp;
	Serial.println("tm_struct:");
	time(&now);                       // read the current time
  	localtime_r(&now, &tm);           // update the structure tm with the current time
	Serial.write(tm.tm_sec);
	Serial.write(tm.tm_min);
	Serial.write(tm.tm_hour);
	Serial.write(tm.tm_mday);
	Serial.write(tm.tm_wday);
	Serial.write(tm.tm_mon + 1);	// January = 0 (!)
	temp = tm.tm_year + 1900;		// Serial.write(tm.tm_year) years since 1900
	highb = temp >> 8;
	lowb = temp;  
	Serial.write(highb);
	Serial.write(lowb);
	temp = tm.tm_yday;				//Serial.write(tm.tm_yday);
	highb = temp >> 8;
	lowb = temp;
	Serial.write(highb);
	Serial.write(lowb);
	temp = tm.tm_isdst; 			//Serial.write(tm.tm_isdst);
	highb = temp >> 8;
	lowb = temp;
	Serial.write(highb);
	Serial.write(lowb);
}

//NTP Send
void send_ntp(void)
{
	time(&now);                       		// read the current ntp time
	localtime_r(&now, &tm);             	// update the structure tm with the current time
	/*Bei Bedarf GMZ-Offset senden*/
	//if(tm.tm_isdst == 1)now += GMZ_2STD_OFFSET;
	//else if(tm.tm_isdst == 0)now += GMZ_1STD_OFFSET;
	time_y2k = now - UNIX_OFFSET;			//set Y2K UTC 32bit 
	Serial.write("y2k ");					//send key Y2K-UTC mit Leerzeichen
	Serial.print(time_y2k);					//send value in ascii + \r\n
	Serial.write(":");						//send Trennmarker
	Serial.println(tm.tm_isdst);			//send Sommer/Winterzeit in ascii + \r\n

	//Debug
	/*
	if(tm.tm_isdst > 0)
	{
		//Serial.write("Sommerzeit\r\n");	//Debugausgabe
		time_y2k += GMZ_2STD_OFFSET;				//+2Std Offset
	}
	if(tm.tm_isdst == 0)
	{
		//Serial.write("Winterzeit\r\n");	//Debugausgabe
		time_y2k += GMZ_1STD_OFFSET;				//+1Std Offset
	}
	//if(tm.tm_isdst < 0)Serial.write("Keine isdst\r\n");	
	*/	
	//send Key:Value Paar (y2k 123456789:isdst)
	
	//Serial.write(":");				//Trennzeichen
	//Serial.println(tm.tm_isdst);	//send Sommerzeitstatus

	//Serial.write("utc ");		//send key utc mit Leerzeichen
	//Serial.println(now);		//send value in ascii + \r\n
	//Serial.write(asctime(&tm));
	//Serial.write(ctime(&now));
}
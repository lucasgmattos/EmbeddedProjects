/*
 * network.c
 *
 *  Created on: 30 de jan de 2019
 *      Author: lucas
 */


/* Includes */
#include "globals.h"
#include "stm32f10x.h"
#include "stm32f1xx_it.h"
#include "network.h"
#include "usart.h"
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <wifipass.h>

/* Private variables */
volatile uint8_t localip[4] = {0,0,0,0};
volatile uint8_t publicip[4] = {0,0,0,0};
volatile int updateclock = 1;
volatile int networkcheck = 0;


// setup the ESP8266 peripheral connected to UART interface
ErrorStatus setupNetwork()
{
	Usart1Init();	//Initialize USART for ESP8266

	char c[100] = "";	// character buffer for command responses from ESP8266
	sendLine("AT+RST\r\n"); // Reset ESP8266
	delay(6000);
	char flush[100];	// character buffer to flush memory dump from AT+RST command
	while(getStream(flush,sizeof(flush))==ERROR);	// flush memory dump AT+RST command

	sendLine("ATE0\r\n"); // disable Echo
	delay(500);
	do {
		getUntilch(c,'\n',sizeof(c));
		if(startsWith("FAIL",c)>0)
		{
			return ERROR;
		}
	}while(startsWith("OK",c)<=0); 	//checks if ECHO disabling is successful (non-standard "send_and_get_response" mothod)


	if (startNetwork()==ERROR) return ERROR;	// call for network Start

	return SUCCESS;
}

// Network startup procedure
// Connects to WiFi hotspot, enables multiple connection ports, setup the WebServer port 333
//
ErrorStatus startNetwork()
{
	char connectioncmd[100]; // buffer for WiFi connection command
	sprintf(connectioncmd,"AT+CWJAP=\"%s\",\"%s\"\r\n",SSID,PASSWORD); // WiFi SSID and PASSWORD from wifipass.h
	if (!send_and_get_response_std(connectioncmd,'\n',"OK","FAIL",6000)) return ERROR; // connects to WiFi Hotspot

	if (!send_and_get_response_std("AT+CIPMUX=1\r\n",'\n',"OK","ERROR",500)) return ERROR; // Enables multiple connection ports

	if (!send_and_get_response_std("AT+CIPSERVER=1,333\r\n",'\n',"OK","ERROR",500)) return ERROR; // setup Webserver on port 333

	getIPAddress(); // tries to get local and public IP Addresses once

	return SUCCESS;
}

// Standard routine for command parsing to ESP8266. Specific cases may use an adaptation of this function.
// input parameters require previous knowledge and testing for each ESP8266 AT command behaviour
ErrorStatus send_and_get_response_std(char *sendstr, char limiterchar, char *successstr, char *failurestr, uint16_t delaytime)
{
	char c[100];	// character buffer for command responses from ESP8266

	sendLine(sendstr);	// sends input command line to esp8266
	delay(delaytime);	// wait time for ESP8266 processing and response acquisition from USART

	// loop to read line-by-line response from ESP8266
	do {
		getUntilch(c, limiterchar ,sizeof(c)); // fetch USART buffer from ESP8266 until specified character
		if(startsWith(failurestr,c)>0)
		{
			return ERROR;	// breaks loop and returns ERROR if specified negative response is sent by ESP8266
		}
	} while(startsWith(successstr,c)==0);	// iterate until success message is returned

	return SUCCESS; // if loop exits successfully, then positive return is given
}

// check if beginning of input string (str) matches a specified string (pre)
int startsWith(const char *pre, const char *str)
{
	size_t lenpre = strlen(pre), lenstr = strlen(str);
	if(lenstr==0)
	{
		return -1; // returns -1 if input string is empty
	}
	if(lenstr < lenpre)
	{
		return 0;
	}
	else
	{
		if (strncmp(pre, str, lenpre) == 0)
		{
			return 1; // returns 1 if comparison is successful
		}
		else
		{
			return 0; // returns 0 if comparison is unsucessful
		}
	}
}

// get current time and date from NTP server
ErrorStatus getNTPTime()
{
	char c[100] = ""; // character buffer for command responses from ESP8266
	char packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

	sendLine("AT+CIPCLOSE=4\r\n"); // close connection slot 4, freeing way for NTP connection
	delay(100);
	getUntilch(c,'\0',sizeof(c)); // fetches USART RX buffer until its end after connection close

	if (!send_and_get_response_std("AT+CIPSTART=4,\"UDP\",\"200.160.7.186\",123\r\n",'\n',"OK","ERROR",500)) return ERROR; // open UDP connection to NTP server

	// NTP request packet
	packetBuffer[0] = 0x1B;   // LI, Version, Mode
	packetBuffer[1] = 0;     // Stratum, or type of clock
	packetBuffer[2] = 6;     // Polling Interval
	packetBuffer[3] = 0xE3;  // Peer Clock Precision

	if (!send_and_get_response_std("AT+CIPSEND=4,48\r\n",'\n',"OK","ERROR",1000)) return ERROR; // call for data output through NTP connection slot

	sendData(packetBuffer, sizeof(packetBuffer)); // send request packet

	delay(1000);
	int i;
	i = getUntilch(c,':',sizeof(c));
	if(c[i-1]!=':') return ERROR; 	// incoming data in ESP8266 is delimited by ":", if that`s not detected return ERROR
	delay(500);
	getStream(packetBuffer, sizeof(packetBuffer)); // get response from NTP server

	getEpoch(packetBuffer); // convert and store epoch time

	if (!send_and_get_response_std("AT+CIPCLOSE=4\r\n",'\n',"OK","ERROR",500)) return ERROR; // close NTP connection slot

	return SUCCESS; // if function reaches this point, NTP time was successfully acquired
}

// standard function to send a command line to ESP8266 USART intarface
void sendLine(char *data)
{
	int len = strlen(data);
	for (int n = 0; n < len; n++)
	{
		Usart1Put(*data++ & (uint16_t)0x01FF);	// send data to USART TX interface
		delay(1);
	}
}

// standard function to send specified amount of data (*data). length of characters to be sent mus be an input parameter (len)
void sendData(char *data, int len)
{
	for (int n = 0; n < len; n++)
	{
		Usart1Put(*data++ & (uint16_t)0x01FF); // send data to USART TX interface
		delay(1);
	}
}

// get a data stream (*data) from USART RX interface until a specified length (end).
ErrorStatus getStream(char *data, int end)
{
	uint8_t c;
	int n = 0; // counter to detect if the data Stream returned has a length compatible to input value (end)
	do {

		if(Usart1Get(&c)==ERROR) break; // if buffer failed to return a character, return ERROR

		*data++ = c; // if character was received store in next *data position
		n++;
	} while (n<end);

	if (n==end)
	{
		return ERROR; // if data stream received from USART has length different from expected, return ERROR
	}
	else
	{
		return SUCCESS; // if data stream received from USART has length equal from expected, return SUCCESS
	}
}

// standard function to read USART RX interface data until specified character (limiter). Size (end) of character buffer (*data) must be provided.
int getUntilch(char *data, char limiter, int end)
{
	uint8_t c; // 8-byte char buffer
	int n = 0; // counter for size of data captured from USART RX interface, in case it needs to be checked by caller routine.
	// loop to get data until limiter or maximum data length is reached
	do {

		Usart1Get(&c); // captures character from USART RX interface

		if(c!='\0')
		{
			*data++ = (char)c;
			n++;
		}
		else
		{
			break; // if null data is captured from USART RX interface, break
		}
	} while (c!=limiter && n<end);

	*data++ = '\0'; // sets last character to /0 (string termination) when fetching is completed

	return n;
}

// conversion routine for data received from NTP server to epoch time format
void getEpoch(char *packet)
{
	unsigned long highWord, lowWord;
	highWord = ((*(packet+40) << 8) | *(packet+41));
	lowWord = ((*(packet+42) << 8) | *(packet+43));
	clockepoch = (highWord << 16 | lowWord) - 2208988800 + TIMEZONE_SHIFT; // subtracting seconds from 1900 and adding timezone shift
}

// routine to check Network connection health
ErrorStatus checkNetwork()
{
	char c[100];

	sendLine("AT+CWJAP?\r\n"); // command to check joined access point
	delay(500);
	// loop to get response to CWJAP command until OK is received
	do {
		getUntilch(c,'\n',sizeof(c));
		if(startsWith("No AP",c)>0)
		{
			return ERROR; // if "No AP" is get as response, no connection is stablished, return ERROR
		}
		else if(startsWith("+CWJAP:",c)>0)
		{
			return SUCCESS; // if +CWJAP: is the beginning of response, then some AP connection is active, return success
		}
	}while(startsWith("OK",c)==0);

	return ERROR; // if this point is reached, then checking failed, return ERROR
}

// Webserver webpage reponse routine
ErrorStatus sendWebPage(char pageno, char conn_number)
{
	char msg[30]; // character buffer for ESP8266 commands to be sent
	char httpheader[300]; // buffer for http headers of outgoing webpages
	char htmlpage[2500]; // buffer for webpage outgoing webpage content
	int htmlpage_contentlength; // html page size counter, used for ESP8266 commands
	if(pageno==1) // if Home page is requested
	{
		sprintf(htmlpage,
				"<!DOCTYPE html>"
				"<html>"
				"<head>"
				"<link rel=\"icon\" href=\"data:image/x-icon;base64,AAABAAEAEBAAAAEAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/0HO4P+eWST/nlkk/55ZJIwAAAAAAAAAAJ5ZJIxBzuD/nlkk/55ZJP9BzuD/nlkk/0HO4P9BzuD/Qc7g/55ZJP9BzuD/nlkk/55ZJP+eWSSMAAAAAAAAAACeWSSMQc7g/55ZJP9BzuD/nlkk/55ZJP9BzuD/nlkk/55ZJP+eWST/Qc7g/0HO4P9BzuD/nlkkjAAAAAAAAAAAnlkkjEHO4P9BzuD/Qc7g/55ZJP+eWST/Qc7g/0HO4P9BzuD/nlkk/0HO4P+eWST/Qc7g/55ZJIwAAAAAAAAAAJ5ZJIxBzuD/nlkk/55ZJP9BzuD/nlkk/0HO4P+eWST/Qc7g/55ZJP9BzuD/Qc7g/0HO4P+eWSSMAAAAAAAAAACeWSSMQc7g/55ZJP+eWST/Qc7g/55ZJP9BzuD/Qc7g/0HO4P+eWST/nlkk/55ZJP+eWST/nlkkjAAAAAieWSSMnlkkjEHO4P9BzuD/Qc7g/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJIyeWSSMnlkk/55ZJP8AAAB2nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIyeWST/nlkk/wAAAHaeWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAnlkkjJ5ZJP+eWST/AAAAdp5ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAAAAAAAAAAACeWSSMnlkk/55ZJP8AAAB2nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAAAAAAAAAAAAAAAAAAAAAAJ5ZJIyeWST/nlkk/wAAAHaeWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJP+eWST/AAAAdgAAAHaeWST/nlkk/55ZJIwAAAAIAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACeWSSMnlkk/55ZJP+eWST/nlkk/55ZJIwAAAAIAAAACAAAAAgAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJ5ZJIyeWST/nlkk/55ZJIwAAAAIAAAACAAAAAgAAAAIAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJIwAAAAAAAAACAAAAAgAAAAIAAAACAAAAAAAAAAAgAEAAIABAACAAQAAgAEAAIABAACAAQAAAAAAACAEAAAQCAAAiBEAAMQjAADiRwAA8Y8AAPgfAAD8PwAA/n8AAA==\" rel=\"icon\" type=\"image/x-icon\" />"
				"</head>"
				"<body>"
				"<h1>RepHome</h1>"
				"<form method=\"POST\">"
				"<p>Identify yourself (max 10 characters):</p>"
				"<input type=\"text\" name=\"nameid\" maxlength=\"10\">"
				"<p>Send a message back to your flatmates (max 45 characters):</p>"
				"<input type=\"text\" name=\"message\"  maxlength=\"45\">"
				"<input type=\"hidden\" name = \"null\" value=\"OK\">"
				"<input type=\"submit\">"
				"</form>"
				"</body>"
				"</html>");
		htmlpage_contentlength = strlen(htmlpage);
	}
	else if(pageno==2) // if webpage to be sent is positive response to received message
	{
		sprintf(htmlpage,
				"<!DOCTYPE html>"
				"<html>"
				"<head>"
				"<link rel=\"icon\" href=\"data:image/x-icon;base64,AAABAAEAEBAAAAEAIABoBAAAFgAAACgAAAAQAAAAIAAAAAEAIAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/0HO4P+eWST/nlkk/55ZJIwAAAAAAAAAAJ5ZJIxBzuD/nlkk/55ZJP9BzuD/nlkk/0HO4P9BzuD/Qc7g/55ZJP9BzuD/nlkk/55ZJP+eWSSMAAAAAAAAAACeWSSMQc7g/55ZJP9BzuD/nlkk/55ZJP9BzuD/nlkk/55ZJP+eWST/Qc7g/0HO4P9BzuD/nlkkjAAAAAAAAAAAnlkkjEHO4P9BzuD/Qc7g/55ZJP+eWST/Qc7g/0HO4P9BzuD/nlkk/0HO4P+eWST/Qc7g/55ZJIwAAAAAAAAAAJ5ZJIxBzuD/nlkk/55ZJP9BzuD/nlkk/0HO4P+eWST/Qc7g/55ZJP9BzuD/Qc7g/0HO4P+eWSSMAAAAAAAAAACeWSSMQc7g/55ZJP+eWST/Qc7g/55ZJP9BzuD/Qc7g/0HO4P+eWST/nlkk/55ZJP+eWST/nlkkjAAAAAieWSSMnlkkjEHO4P9BzuD/Qc7g/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJIyeWSSMnlkk/55ZJP8AAAB2nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIyeWST/nlkk/wAAAHaeWST/nlkk/55ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAnlkkjJ5ZJP+eWST/AAAAdp5ZJP+eWST/nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAAAAAAAAAAACeWSSMnlkk/55ZJP8AAAB2nlkk/55ZJP+eWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAAAAAAAAAAAAAAAAAAAAAAAJ5ZJIyeWST/nlkk/wAAAHaeWST/nlkk/wAAAHaeWST/nlkk/55ZJIwAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJP+eWST/AAAAdgAAAHaeWST/nlkk/55ZJIwAAAAIAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAACeWSSMnlkk/55ZJP+eWST/nlkk/55ZJIwAAAAIAAAACAAAAAgAAAAIAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAJ5ZJIyeWST/nlkk/55ZJIwAAAAIAAAACAAAAAgAAAAIAAAACAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAnlkkjJ5ZJIwAAAAAAAAACAAAAAgAAAAIAAAACAAAAAAAAAAAgAEAAIABAACAAQAAgAEAAIABAACAAQAAAAAAACAEAAAQCAAAiBEAAMQjAADiRwAA8Y8AAPgfAAD8PwAA/n8AAA==\" rel=\"icon\" type=\"image/x-icon\" />"
				"</head>"
				"<body>"
				"<h1>RepHome</h1>"
				"<p>Message sent!</p>"
				"<a href=\".\">Back</a>"
				"</body>"
				"</html>");
		htmlpage_contentlength = strlen(htmlpage);
	}

	// standard http header
	sprintf(httpheader,
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %d\r\n"
			"Connection: keep-alive\r\n\r\n"
			,htmlpage_contentlength);


	// count http header length for AT command input
	int countcommands = 0;
	for(int i=0;i<strlen(httpheader);i++)
	{
		if(*(httpheader+i)=='\r' || *(httpheader+i)=='\n') countcommands++;
	}

	sprintf(msg,"AT+CIPSEND=%c,%d\r\n",conn_number,strlen(httpheader)); // format AT+CIPSEND to call for outgoing data through connection port that requested the webpage
	sendLine(msg);
	delay(200);
	sendLine(httpheader); // send HTTP header
	delay(200);

	// count html webpage length for AT command input
	countcommands = 0;
	for(int i=0;i<strlen(htmlpage);i++)
	{
		if(*(htmlpage+i)=='\r' || *(htmlpage+i)=='\n') countcommands++;
	}

	sprintf(msg,"AT+CIPSEND=%c,%d\r\n",conn_number,strlen(htmlpage)); // format AT+CIPSEND to call for outgoing data through connection port that requested the webpage
	sendLine(msg);
	delay(200);
	sendLine(htmlpage); // send HTML webpage
	delay(200);

	//	sprintf(msg,"AT+CIPCLOSE=%c\r\n",conn_number); // close connection
	//	sendLine(msg);
	//	delay(200);

	return SUCCESS;

}

// routine to get local and public IP Addresses
void getIPAddress()
{
	char c[300] = "";

	sendLine("AT+CIPSTA?\r\n"); // command to ESP8266 asking for WiFi connection details
	delay(100);

	getUntilch(c,'"',sizeof(c));
	if(startsWith("+CIPSTA:ip:",c)>0) // find line containing local IP Address
	{

		// store local IP address into global variable
		int l = 0;
		getUntilch(c,'.',sizeof(c)); // fetch characters until next IP . delimiter
		int lc = strlen(c);
		// converts ascii to number format
		for(int i = 0; i < lc-1; i++)
		{
			localip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
		}
		l++;
		getUntilch(c,'.',sizeof(c));
		lc = strlen(c);
		for(int i = 0; i < lc-1; i++)
		{
			localip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
		}
		l++;
		getUntilch(c,'.',sizeof(c));
		lc = strlen(c);
		for(int i = 0; i < lc-1; i++)
		{
			localip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
		}
		l++;
		getUntilch(c,'"',sizeof(c));
		lc = strlen(c);
		for(int i = 0; i < lc-1; i++)
		{
			localip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
		}
	}

	sendLine("AT+CIPSTART=4,\"TCP\",\"api.ipify.org\",80\r\n"); // opens connection port to ipify server to get public IP Address
	delay(500);
	// get response from AT+CIPSTART command
	do {
		getUntilch(c,'\n',sizeof(c));
		if(startsWith("ERROR",c)>0)
		{
			return;
		}
	} while(startsWith("OK",c)==0);

	char get_publicip_cmd[] = "GET / HTTP/1.0\r\nHost: api.ipify.org\r\n\r\n"; // HTTP command to request public IP Address

	char cmd[50];
	sprintf(cmd,"AT+CIPSEND=4,%d\r\n",strlen(get_publicip_cmd)); // format AT+ICPSEND command to ESP8266

	sendLine(cmd);
	delay(200);
	// get response from AT+CIPSEND command
	do {
		getUntilch(c,'\n',sizeof(c));
		if(startsWith("ERROR",c)>0)
		{
			return;
		}
	}while(startsWith("OK",c)==0);

	sendLine(get_publicip_cmd); // send request to ipify

	delay(1000);

	// gets Content-length part of HTTP response in order to track the position of the IP address among ipify response
	do
	{
		getUntilch(c,'\n',sizeof(c));
	}while(startsWith("Content-Length",c)==0);

	int contentlength = asciiToint(c[strlen(c)-3]) + asciiToint(c[strlen(c)-4])*10; // convert content-length to number format

	// previously known breaklines in ipify response
	getUntilch(c,'\n',sizeof(c));
	getUntilch(c,'\n',sizeof(c));
	getUntilch(c,'.',sizeof(c));


	// convert IP address from ascii format to number format and store in public IP Address global variable
	int lc = strlen(c);
	int l = 0;
	for(int i = 0; i < lc-1; i++)
	{
		publicip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
	}
	l++;
	contentlength -= lc;
	getUntilch(c,'.',sizeof(c));
	lc = strlen(c);
	for(int i = 0; i < lc-1; i++)
	{
		publicip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
	}
	l++;
	contentlength -= lc;
	getUntilch(c,'.',sizeof(c));
	lc = strlen(c);
	for(int i = 0; i < lc-1; i++)
	{
		publicip[l] += (uint8_t) asciiToint(c[lc-2-i])*pow(10,i);
	}
	l++;
	contentlength -= lc;
	getUntilch(c,'\n',sizeof(c));
	lc = contentlength;
	for(int i = 0; i < lc; i++)
	{
		publicip[l] += (uint8_t) asciiToint(c[lc-1-i])*pow(10,i);
	}
}

// standard routine to conver ascii character to number format
int asciiToint(char asciichar)
{
	return (int)((asciichar)-48);
}

// routine to be placed in while loop of main to monitor incoming requests from Clients to the WebServer
void monitorNetwork()
{
	char c[300] = ""; 	// char buffer for WebServer handling
	char connection_number;		// connection number for incoming request to WebServer

	if(updateclock) if (getNTPTime()==SUCCESS) updateclock = 0; // if clock update timeout is reached, update time. If unsucessful previous attempt, try again

	// if WiFi connection health check timeout is reached, check it.
	if(networkcheck)
	{
		if (checkNetwork()==ERROR)
		{
			if(startNetwork()==SUCCESS) networkcheck = 0;
		}
		else
		{
			networkcheck = 0;
		}
	}

	delay(100); // delay for monitor routine, avoids incomplete buffer reading when a client sends data at a random time
	getUntilch(c,'\n',sizeof(c)); // read next incoming line

	if(startsWith("WIFI DISCONNECT",c)>0) while(startNetwork()==ERROR); // check if ESP8266 actively informs WiFi connection lost

	// if incoming data is detected by ESP8266, start reading it
	if(startsWith("+IPD",(c))>0)
	{
		connection_number = *(c+5); // get connection slot number from +IPD
		delay(200);
		int findframestart = 0;
		// loop to move buffer pointer to start of incoming data delimiter :
		while(c[findframestart]!=':')
		{
			findframestart++;
		}
		findframestart++;


		if(startsWith("GET",c+findframestart)>0)
		{
			sendWebPage(1,connection_number); // if client sends a GET http request, respond with Homepage
		}
		else if(startsWith("POST",c+findframestart)>0) // if client sends a POST http request, captures message data
		{
			// find nameid variable (User identification) in POST
			do
			{
				getUntilch(c,'\n',sizeof(c));
			} while(startsWith("nameid=",c)==0);

			int i = 0; // pointer to the character position being read from c, AND urimessage
			char urimessage[MESSAGE_LENGTH]; // buffer for incoming data from POST

			// gets User identification until POST variable delimiter &
			while (c[i+7]!='&') // adds 7 to skip "nameid=" characters from http POST
			{
				urimessage[i] = c[i+7]; // gets raw URI data from http POST
				i++;
			}

			// adds a ": " between User Identification and message
			urimessage[i++] = ':';
			urimessage[i++] = ' ';

			// navigate to next http POST delimiter & to capture message
			while (c[i+14]!='&') // adds +7 (total 14) to skip "message=" characters from http POST (should be 8 but there`s a i++ above)
			{
				urimessage[i] = c[i+14]; // gets raw URI data from http POST
				i++;
			}
			urimessage[i++] = '\0'; // places string delimiter at the end of urimessage

			urldecode2(messages[currentmessage],urimessage); // decode URI to convert special characters eventually received

			currentscreen = currentmessage+1; // automatically moves to currently received message screen ( adds 1 because currentmessage starts in 0 and current currentscreen starts in 1 for message screens)

			if(currentmessage==(MESSAGE_SCREENS-1))
			{
				currentmessage = 0; // if current message is the last, return to 0
			}
			else
			{
				currentmessage++; // increments current message tracker
			}

			switchscreen = 0; // disables switchscreen flag to maintain currently received message for maximum timeout period

			sendWebPage(2,connection_number); // send back to client "Message sent!" response
		}
	}

}

// routine to convert uri special characters into ascii
void urldecode2(volatile char *dst, const char *src)
{
	char a, b;
	while (*src)
	{
		if ((*src == '%') &&
				((a = src[1]) && (b = src[2])) &&
				(isxdigit(a) && isxdigit(b)))
		{
			if (a >= 'a')
				a -= 'a'-'A';
			if (a >= 'A')
				a -= ('A' - 10);
			else
				a -= '0';
			if (b >= 'a')
				b -= 'a'-'A';
			if (b >= 'A')
				b -= ('A' - 10);
			else
				b -= '0';
			*dst++ = 16*a+b;
			src+=3;
		}
		else if (*src == '+')
		{
			*dst++ = ' ';
			src++;
		}
		else
		{
			*dst++ = *src++;
		}
	}
	*dst++ = '\0';
}

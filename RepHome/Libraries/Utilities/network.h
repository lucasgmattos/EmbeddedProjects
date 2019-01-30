/*
 * network.h
 *
 *  Created on: 30 de jan de 2019
 *      Author: lucas
 */

#ifndef UTILITIES_NETWORK_H_
#define UTILITIES_NETWORK_H_

/* Includes */

/* Defines */
#define NTP_PACKET_SIZE 	48
#define TIMEZONE_SHIFT		-7200


/* Private function prototypes */
ErrorStatus setupNetwork();
ErrorStatus startNetwork();
ErrorStatus checkNetwork();
void getIPAddress();
ErrorStatus send_and_get_response_std(char *sendstr, char limiterchar, char *successstr, char *failurestr, uint16_t delaytime);
int startsWith(const char *pre, const char *str);
ErrorStatus getNTPTime();
void sendLine(char *data);
void sendData(char *data, int len);
ErrorStatus getStream(char *data, int end);
int getUntilch(char *data, char limiter, int end);
void getEpoch(char *packet);
int asciiToint(char asciichar);
void monitorNetwork();
void urldecode2(volatile char *dst, const char *src);

ErrorStatus sendWebPage(char pageno, char conn_number);

#endif /* UTILITIES_NETWORK_H_ */

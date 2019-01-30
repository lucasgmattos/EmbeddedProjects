/*
 * globals.h
 *
 *      Author: Lucas Mattos
 * 		Container for the global variables declarations.
 * 		The file that defines each variable is listed on the grouping below.
 */



#ifndef UTILITIES_GLOBALS_H_
#define UTILITIES_GLOBALS_H_

#define MESSAGE_LENGTH		65 	// maximum message length given screen size
#define MESSAGE_SCREENS		4	// maximum number of message screens

#include <stdint.h>

/* screen.c */
// Clock
volatile extern unsigned long clockepoch; // epoch time for clock
volatile extern int clock_year; // year
volatile extern int clock_month; // month
volatile extern int clock_day;	// month day
volatile extern int clock_hours; // hour
volatile extern int clock_minutes; // minutes
volatile extern int clock_seconds; // seconds
volatile extern char clocktick; // ticker inbetween hours and minutes (switches from ":" to " ")
// Screen control
volatile extern int switchscreen; // counter for automatic screen switch
volatile extern int currentscreen; // current screen indicator (0=boot 1,2,3,4=messages 5=IP Addresses)
volatile extern int currentmessage; // circular pointer to current message slot to be written
// Messages
volatile extern char messages[MESSAGE_SCREENS][MESSAGE_LENGTH]; // messages array

/* strm321xx_it.c */
// Timer
volatile extern uint64_t ticks; // ticks from system clock

/* network.c */
// Network
volatile extern uint8_t localip[4]; // local IP address
volatile extern uint8_t publicip[4]; //public IP address
volatile extern int updateclock; // Flag for periodic NTP clock refresh
volatile extern int networkcheck; // Flag for periodic network connection status detection. Stays 1 if no connection is detected

#endif /* UTILITIES_GLOBALS_H_ */

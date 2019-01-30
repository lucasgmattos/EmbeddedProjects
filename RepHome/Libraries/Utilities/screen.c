/*
 * screen.c
 *
 *  Created on: 30 de jan de 2019
 *      Author: lucas
 */

/* Includes */
#include "globals.h"
#include "screen.h"
#include "stm32f10x.h"
#include "stm32f1xx_it.h"
#include "network.h"
#include <stdio.h>
#include <string.h>


// Clock
volatile unsigned long clockepoch = 0;
volatile int clock_year = 2018;
volatile int clock_month = 1;
volatile int clock_day = 1;
volatile int clock_hours = 0;
volatile int clock_minutes = 0;
volatile int clock_seconds = 0;
volatile char clocktick = ':';
// Screen control
volatile int switchscreen = 0;
volatile int currentscreen = 0;
volatile int currentmessage = 0;
// Messages
volatile char messages[MESSAGE_SCREENS][MESSAGE_LENGTH];

//setup display during startup
void setupDisplay()
{
	u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R0, u8x8_byte_4wire_sw_spi, u8g2_gpio_and_delay_stm32); //configure GPIO for specific HW
	u8g2_InitDisplay(&u8g2); // send init sequence to the display, display is in sleep mode after this
	u8g2_SetPowerSave(&u8g2, 0); // wake up display
	u8g2_ClearBuffer(&u8g2); //clear data buffer inside display
	u8g2_SetContrast(&u8g2, 250); //set contrast
}

// ug82 configurator of GPIO for SPI data exchange with specific HW
uint8_t u8g2_gpio_and_delay_stm32(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
	GPIO_InitTypeDef GPIO_InitStruct;

	switch(msg){
	//Initialize SPI peripheral
	case U8X8_MSG_GPIO_AND_DELAY_INIT:

		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
		/* SPI SCK, MOSI GPIO pin configuration  */
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		//RES Pin
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		//DC Pin
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_1;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		//CS Pin
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_2;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_Init(GPIOA, &GPIO_InitStruct);

		break;

		//Function which implements a delay, arg_int contains the amount of ms
	case U8X8_MSG_DELAY_MILLI:
		//delay_system_ticks((uint32_t)arg_int);
		delay((uint32_t)arg_int);
		break;

		//Function which delays 10us **Not Implemented
	case U8X8_MSG_DELAY_10MICRO:

		break;
		//Function which delays 100ns **Not Implemented
	case U8X8_MSG_DELAY_100NANO:

		break;
		//Function to define the logic level of the clockline
	case U8X8_MSG_GPIO_SPI_CLOCK:
		if (arg_int) GPIO_WriteBit(GPIOA, GPIO_Pin_7, SET);
		else GPIO_WriteBit(GPIOA, GPIO_Pin_7, RESET);

		break;
		//Function to define the logic level of the data line to the display
	case U8X8_MSG_GPIO_SPI_DATA:
		if (arg_int) GPIO_WriteBit(GPIOA, GPIO_Pin_5, SET);
		else GPIO_WriteBit(GPIOA, GPIO_Pin_5, RESET);

		break;
		// Function to define the logic level of the CS line
	case U8X8_MSG_GPIO_CS:
		if (arg_int) GPIO_WriteBit(GPIOA, GPIO_Pin_2, SET);
		else GPIO_WriteBit(GPIOA, GPIO_Pin_2, RESET);

		break;
		//Function to define the logic level of the Data/ Command line
	case U8X8_MSG_GPIO_DC:
		if (arg_int) GPIO_WriteBit(GPIOA, GPIO_Pin_1, SET);
		else GPIO_WriteBit(GPIOA, GPIO_Pin_1, RESET);

		break;
		//Function to define the logic level of the RESET line
	case U8X8_MSG_GPIO_RESET:
		if (arg_int) GPIO_WriteBit(GPIOA, GPIO_Pin_0, SET);
		else GPIO_WriteBit(GPIOA, GPIO_Pin_0, RESET);

		break;
	default:
		return 0; //A message was received which is not implemented, return 0 to indicate an error
	}

	return 1; // command processed successfully.
}

// routine to update screen with chosen screen number (screen)
void showScreen(int screen)
{

	if(screen==0) // boot screen
	{
		u8g2_ClearBuffer(&u8g2); // Clear display buffer
		u8g2_SetFont(&u8g2, u8g2_font_bubble_tr); // set font for DrawStr function
		u8g2_DrawStr(&u8g2, 0, 20, "Rep"); // Write string in specified position
		u8g2_DrawStr(&u8g2, 0, 40, "Home");

	}
	else if(screen>0 && screen<=4) // routine to configure message screens
	{
		char firstline[30] = ""; // buffer for header of message screen (date/time and current message)
		calcDate(clockepoch);
		sprintf(firstline, "%.2d/%.2d  %.2d%c%.2d", clock_day, clock_month, clock_hours, clocktick, clock_minutes); // write header text
		u8g2_ClearBuffer(&u8g2);
		u8g2_SetFont(&u8g2, u8g2_font_fub11_tf);
		u8g2_DrawStr(&u8g2, 7, 11, firstline); // draw date and time on header
		char sn[2]; // screen number string
		sn[0] = (char)(screen+48); // convert screen number to ascii
		sn[1] = '\0'; // set string delimiter
		u8g2_DrawStr(&u8g2, 117, 11, sn); // draw mesage number on header
		u8g2_DrawLine(&u8g2, 0, 13, 127, 13); // horizontal header delimitation line
		u8g2_DrawLine(&u8g2, 115, 13, 115, 0); // vertical header delimitation line (between date/time and message number)
		u8g2_SetFont(&u8g2, u8g2_font_helvR10_tf);


		// the chosen screen has a maximum of vertical size of 4 lines for the chosen font (u8g2_font_helvR10_tf)
		// the following sequence writes the characters breaking line as soon as the limit of the screen is reached

		int blchar = 0; // pointer to current character navigated in message
		int lastblchar = 0; // pointer to character that was last before the last breakline in screen
 		int line = 0; // line counter
		char printmsgline[30]; // character buffer for message writing

		while (line<4) // iterate no more than the lines limit
		{
			//write characters in buffer while there`s stil space for it
			do
			{
				printmsgline[blchar - lastblchar] = *(messages[screen - 1] + blchar);
				printmsgline[blchar - lastblchar + 1] = '\0';
				blchar++;
			} while (u8g2_GetStrWidth(&u8g2, printmsgline) < u8g2_GetDisplayWidth(&u8g2) - 8 && strlen(messages[screen - 1]+blchar)>0);
			//if it`s not the last line, then place a "-" if the character prior to the breakline is not a space (for word separation indication)
			if (line<3)
			{
				if (printmsgline[blchar]!=' ')
				{
					printmsgline[blchar+1] = '-';
					printmsgline[blchar+1] = '\0';
				}
			}
			u8g2_DrawStr(&u8g2, 0, 27+(11*line), printmsgline); //draw current line
			lastblchar = blchar; //refresh last character prior to breakline with current pointer
			line++; // jump line
		}
	}
	// if current screen is IP addresses screen, write them
	else if (screen==5)
	{
		u8g2_ClearBuffer(&u8g2);
		u8g2_SetFont(&u8g2, u8g2_font_helvR10_tf);
		u8g2_DrawStr(&u8g2, 0, 12, "Local Address:");
		char localipaddress[20];
		sprintf(localipaddress,"%d.%d.%d.%d",localip[0],localip[1],localip[2],localip[3]);
		u8g2_DrawStr(&u8g2, 0, 27, localipaddress);
		u8g2_DrawStr(&u8g2, 0, 47, "Public Address:");
		char publicipaddress[20];
		sprintf(publicipaddress,"%d.%d.%d.%d",publicip[0],publicip[1],publicip[2],publicip[3]);
		u8g2_DrawStr(&u8g2, 0, 62, publicipaddress);

	}

	u8g2_SendBuffer(&u8g2); // send buffer to screen from information written above

}

// routine to calculate current date and time from epoch
void calcDate(unsigned long epoch)
{
	int seconds, minutes, hours, days, dayOfWeek, month, year;

	seconds = epoch;

	/* calculate minutes */
	minutes  = seconds / 60;
	seconds -= minutes * 60;
	/* calculate hours */
	hours    = minutes / 60;
	minutes -= hours   * 60;
	/* calculate days */
	days     = hours   / 24;
	hours   -= days    * 24;

	/* Unix time starts in 1970 on a Thursday */
	year      = 1970;
	dayOfWeek = 4;

	while(1)
	{
		char leapYear;
		if(year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
		{
			leapYear = 1;
		}
		else
		{
			leapYear = 0;
		}
		uint16_t daysInYear = leapYear ? 366 : 365;
		if (days >= daysInYear)
		{
			dayOfWeek += leapYear ? 2 : 1;
			days      -= daysInYear;
			if (dayOfWeek >= 7)
				dayOfWeek -= 7;
			++year;
		}
		else
		{
			//tm->tm_yday = days;
			dayOfWeek  += days;
			dayOfWeek  %= 7;

			/* calculate the month and day */
			static const uint8_t daysInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
			for(month = 1; month < 13; ++month)
			{
				uint8_t dim = daysInMonth[month-1];

				/* add a day to feburary if this is a leap year */
				if (month == 2 && leapYear)
					++dim;

				if (days >= dim)
					days -= dim;
				else
					break;
			}
			break;
		}
	}


	clock_seconds  = (uint8_t)seconds;
	clock_minutes  = (uint8_t)minutes;
	clock_hours = (uint8_t)hours;
	clock_day = (uint8_t)days + 1;
	clock_month  = (uint8_t)month;
	clock_year = (uint16_t)year;
	//tm->tm_wday = dayOfWeek;
}


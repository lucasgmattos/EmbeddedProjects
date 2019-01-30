/******************************************************************************
File:     main.c
Author:   Lucas Mattos
 ******************************************************************************/

/* Includes */
#include "globals.h"
#include "stm32f10x.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "screen.h"
#include "network.h"
#include "timer.h"
#include "button.h"

/* Private typedef */

/* Private define  */

/* Private macro */

/* Private variables */

/* Private function prototypes */

/* Private functions */

/**
 **===========================================================================
 **
 **  Abstract: main program
 **
 **===========================================================================
 */
int main(void)
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);		// setup Interrupts priority
	setupSystick((uint16_t)TICK_FREQ);
	setupDisplay();
	setupClockTimer();
	while(setupNetwork()!=SUCCESS); // while loop on setupNetwork() function to force ESP8266 reset if ERROR is returned during the procedure
	if (getNTPTime()==SUCCESS) updateclock = 0;	// disables updateclock flag if time is successfully fetched from NTP server
	setupButton();

	currentscreen = 1; 	// switch from boot screen (#0) to screen #1 (Message no 1)

	// infinite loop for checking WebServer activity and execute network/clock refeshes
	while(1)
	{
		monitorNetwork();
	}
}

/******************************************************************************
File:     screen.h
Author:   Lucas Mattos
******************************************************************************/

#ifndef UTILITIES_SCREEN_H_
#define UTILITIES_SCREEN_H_

/* Defines */


/* Includes */
#include "u8g2.h"

/* Private typedef */
u8g2_t u8g2;

/* Private macro */

/* Private variables */



/* Private function prototypes */
void setupDisplay();
uint8_t u8g2_gpio_and_delay_stm32(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr);
void showScreen(int screen);
void calcDate(unsigned long epoch);




#endif /* UTILITIES_SCREEN_H_ */

/*
 * timer.h
 *
 *  Created on: 30 de jan de 2019
 *      Author: lucas
 */

#ifndef UTILITIES_TIMER_H_
#define UTILITIES_TIMER_H_

#define TICK_FREQ			1000 //timer frequency
#define CLK_PRESC			360 // timer prescaler
#define CLK_CTR				50000 // counter limit

void setupSystick(uint16_t f_tick);
void setupClockTimer();

#endif /* UTILITIES_TIMER_H_ */

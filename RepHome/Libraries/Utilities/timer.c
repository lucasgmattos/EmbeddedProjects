/*
 * timer.c
 *
 *      Author: Lucas Mattos
 */

#include "globals.h"
#include "timer.h"
#include "stm32f10x.h"

#define TICK_FREQ			1000 //timer frequency
#define CLK_PRESC			360 // timer prescaler
#define CLK_CTR				50000 // counter limit

//setup Systick, used on delay
void setupSystick(uint16_t f_tick)
{
	// Typedef for Reset clock counter configuration
	RCC_ClocksTypeDef RCC_Clocks;
	// capture current RCC frequency
	RCC_GetClocksFreq(&RCC_Clocks);
	//setup Systick based on RCC and intended value (f_tick)
	(void) SysTick_Config(RCC_Clocks.HCLK_Frequency / f_tick);
	// setup Systick interrupt priority
	NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(2,1,0));
}

// setup timer for screen refresh and network timeouts. Triggers interrupt every 1/4 second
void setupClockTimer()
{
	// Enable Timer 4 from APB1
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	// Typedef for timer configuration
	TIM_TimeBaseInitTypeDef TIM_str;
	// default values for Timer
	TIM_TimeBaseStructInit(&TIM_str);
	// setup timer period as counter limit
	TIM_str.TIM_Period = CLK_CTR;
	// setup timer prescaler
	TIM_str.TIM_Prescaler = CLK_PRESC - 1;
	// setup timer counting mode
	TIM_str.TIM_CounterMode = TIM_CounterMode_Up;
	// setup timer clock division mode
	TIM_str.TIM_ClockDivision = TIM_CKD_DIV1;
	// setup repetition counter
	TIM_str.TIM_RepetitionCounter = 0;
	// initialize counter with previously setup parameters
	TIM_TimeBaseInit (TIM4, &TIM_str);

	// Typedef for NVIC configuration
	NVIC_InitTypeDef nvicStructure;
	// configurint Timer4 IRQ in NVIC
	nvicStructure.NVIC_IRQChannel = TIM4_IRQn;
	// setup preemption priority
	nvicStructure.NVIC_IRQChannelPreemptionPriority = 1;
	// setup subpriority
	nvicStructure.NVIC_IRQChannelSubPriority = 1;
	// setup NVIC Channels enable
	nvicStructure.NVIC_IRQChannelCmd = ENABLE;
	// initialize NVIC with previously setup parameters
	NVIC_Init(&nvicStructure);

	// Enable Timer 4
	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM4, ENABLE);

}

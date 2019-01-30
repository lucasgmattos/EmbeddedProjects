/*
 * button.c
 *
 *      Author: Lucas Mattos
 */

#include "button.h"
#include "stm32f10x.h"

// setup the screen navigation button
void setupButton()
{
	//GPIO structure to init GPIO
	GPIO_InitTypeDef GPIO_Structure;
	// Enable APB2 GPIOB clock
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	// Enable APB2 Alternate Function IO
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	// set GPIO Pin 5 to be configured
	GPIO_Structure.GPIO_Pin = GPIO_Pin_5;
	// set pin mode as Input Pull-down
	GPIO_Structure.GPIO_Mode = GPIO_Mode_IPD;
	// set pin frequency at 2MHz
	GPIO_Structure.GPIO_Speed = GPIO_Speed_2MHz;
	// initialize GPIO with previously set parameters
	GPIO_Init(GPIOB, &GPIO_Structure);
	// Configure GPIO Pin 5 as External interrupt
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource5);
	// clear external interrupt pending bit for Pin 5
	EXTI_ClearITPendingBit(EXTI_Line5);

	//EXTI structure to init External interrupt
	EXTI_InitTypeDef EXTI_InitStructure;
	//Connect EXTI Line to Button Pin
	//Configure Button EXTI line
	EXTI_InitStructure.EXTI_Line = EXTI_Line5;
	//select interrupt mode
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	//generate interrupt on rising edge
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
	//enable EXTI line
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	//send values to registers
	EXTI_Init(&EXTI_InitStructure);

	//configure NVIC
	//NVIC structure to set up NVIC controller
	NVIC_InitTypeDef NVIC_InitStructure;
	//select NVIC channel to configure
	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	//set priority to lowest
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0F;
	//set subpriority to lowest
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0F;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);

}

/**
 ******************************************************************************
 * @file    stm32f1xx_it.c
 * @author  MCD Application Team
 * @version V1.0.0
 * @date    11-February-2014
 * @brief   Main Interrupt Service Routines.
 *          This file provides template for all exceptions handler and
 *          peripherals interrupt service routine.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
 *
 * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *        http://www.st.com/software_license_agreement_liberty_v2
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/
#include "globals.h"
#include "stm32f1xx_it.h"
#include "usart.h"
#include "screen.h"
#include "timer.h"
#include <stdlib.h>

/** @addtogroup IO_Toggle
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define BUFFERED

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

// Timer
volatile uint64_t ticks = 0;

uint32_t db_time = 0; // button debounce counter
int clockrefreshcounter = 0; // counter for clock update, screen refresh and timeouts. Used in Timer4

/* Private function prototypes -----------------------------------------------*/
int milis();

/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M Processor Exceptions Handlers                          */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1)
	{
	}
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1)
	{
	}
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1)
	{
	}
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1)
	{
	}
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
void SVC_Handler(void)
{
}

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
void PendSV_Handler(void)
{
}

/**
 * @brief  This function handles SysTick Handler.
 * @param  None
 * @retval None
 */
void SysTick_Handler(void)
{
	ticks++; // increment ticks. Used in delay
}

// milis function to return ticks
int milis()
{
	return ticks;
}

// delay function, used in multiple purposes, as SPI delay, network and screen routines
void delay(int ms)
{
	int currenttime = milis();
	while(milis()<(currenttime+ms));
}

/******************************************************************************/
/*                 STM32F1xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_md.s).                                            */
/******************************************************************************/

/**
 * @brief  This function handles PPP interrupt request.
 * @param  None
 * @retval None
 */
/*void PPP_IRQHandler(void)
{
}*/

/**
 * @}
 */

// Timer4 IRQ
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update)!=RESET) // if Timer4 is SET
	{
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update); // Clear interrupt pending bit from TImer 4
		showScreen(currentscreen); // refresh screen
		clockrefreshcounter++; // increment clock counter
		switchscreen++; // increment switch screen timeout

		if(clockrefreshcounter%4==0) // check if a full second was past
		{
			clockepoch++; // increment epoch seconds
			clocktick = (clocktick == ':') ? ' ' : ':'; // switch clock tick
		}

		if(clockrefreshcounter%(4*60*5)==0) // if 5 minutes were past, enable network check flag
		{
			networkcheck = 1;
		}

		if(switchscreen%(4*15)==0) // if 15 seconds were past, switch to next message screen
		{
			if(abs(currentscreen)>=MESSAGE_SCREENS) // if current scren is the last, switch to first
			{
				currentscreen = 1;
			}
			else // otherwise increment to next
			{
				currentscreen = abs(currentscreen) + 1;
			}
		}

		if(clockrefreshcounter>=(4*3600)) // enables clock refresh flag every hour and reset clock counter (avoiding overflow)
		{
			updateclock = 1;
			clockrefreshcounter = 0;
		}
	}
}

// handler for USART1 IRQ
void USART1_IRQHandler(void)
{
	uint8_t ch;
	//if Receive interrupt
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		ch=(uint8_t)USART_ReceiveData(USART1);
#ifdef BUFFERED
		//put char to the buffer
		BufferPut(&U1Rx, ch);
#endif
	}
	if (USART_GetITStatus(USART1, USART_IT_TXE) != RESET)
	{
#ifdef BUFFERED
		if (BufferGet(&U1Tx, &ch) == SUCCESS)//if buffer read
		{
			USART_SendData(USART1, ch);
		}
		else//if buffer empty
#endif
		{
			//disable Transmit Data Register empty interrupt
			USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
		}
	}
}

// handler for button external interrupt
void EXTI9_5_IRQHandler(void)
{
	if (db_time >= milis()) // if current interrupt ocurred within the debounce period, clear interrupt
	{
		EXTI_ClearITPendingBit(EXTI_Line5);
	}
	else
	{
		db_time = milis() + 300; // refresh debounce time value
		if (EXTI_GetITStatus(EXTI_Line5)==SET) // if interrupt status for button is SET
		{
			switchscreen = 0; // reset automatic screen switch counter
			if(abs(currentscreen)==(MESSAGE_SCREENS+1)) // if current screen is the last, move to first
			{
				currentscreen = 1;
			}
			else // otherwise increment screen number
			{
				currentscreen = abs(currentscreen)+1;
			}
			EXTI_ClearITPendingBit(EXTI_Line5); // clear external interrupt for button
		}

	}
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

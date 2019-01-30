/*
 * usart.c
 */
#include "usart.h"
#ifdef BUFFERED
//initialize buffers
volatile FIFO_TypeDef U1Rx, U1Tx;
#endif

// initialize USART buffers (RX and TX) and respective GPIO Pin
void Usart1Init(void)
{
#ifdef BUFFERED
	//initialize buffers
	BufferInit(&U1Rx);
	BufferInit(&U1Tx);
#endif
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	USART_ClockInitTypeDef  USART_ClockInitStructure;
	//enable bus clocks
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	//Set USART1 Tx (PA.09) as AF push-pull
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//Set USART1 Rx (PA.10) as input floating
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	USART_ClockStructInit(&USART_ClockInitStructure);
	USART_ClockInit(USART1, &USART_ClockInitStructure);

	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	//Write USART1 parameters
	USART_Init(USART1, &USART_InitStructure);
	//Enable USART1
	USART_Cmd(USART1, ENABLE);
#ifdef BUFFERED
	//configure NVIC
	NVIC_InitTypeDef NVIC_InitStructure;
	//select NVIC channel to configure
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	//set priority to lowest
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	//set subpriority to lowest
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	//enable IRQ channel
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	//update NVIC registers
	NVIC_Init(&NVIC_InitStructure);

	//disable Transmit Data Register empty interrupt
	USART_ITConfig(USART1, USART_IT_TXE, DISABLE);
	//enable Receive Data register not empty interrupt
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
#endif
}

//send character to USART
void Usart1Put(uint8_t ch)
{
#ifdef BUFFERED
	//put char to the buffer
	BufferPut(&U1Tx, ch);
	//enable Transmit Data Register empty interrupt
	USART_ITConfig(USART1, USART_IT_TXE, ENABLE);
#else
	USART_SendData(USART1, (uint8_t) ch);
	//Loop until the end of transmission
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET)
	{
	}
#endif
}

// receive character from USART
ErrorStatus Usart1Get(uint8_t *ch){
#ifdef BUFFERED

	uint8_t c;
	//check if buffer is empty
	if(BufferIsEmpty(U1Rx) != SUCCESS)
	{
		BufferGet(&U1Rx, &c);
		*ch = c;
		return SUCCESS;
	}
	else
	{
		return ERROR;
	}

#else
	while ( USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);
	return (uint8_t)USART_ReceiveData(USART1);
#endif
}


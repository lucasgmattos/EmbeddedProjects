/*
 * buffer.h
 *
Circular FIFO
 */

#ifndef BUFFER_H_
#define BUFFER_H_
#include "STM32f10x.h"
#define USARTBUFFSIZE	2048
typedef struct{
	uint16_t in;
	uint16_t out;
	uint16_t count;
	uint8_t buff[USARTBUFFSIZE];
}FIFO_TypeDef;

void BufferInit(__IO FIFO_TypeDef *buffer);
ErrorStatus BufferPut(__IO FIFO_TypeDef *buffer, uint8_t ch);
ErrorStatus BufferGet(__IO FIFO_TypeDef *buffer, uint8_t *ch);
ErrorStatus BufferIsEmpty(__IO FIFO_TypeDef buffer);
#endif /* BUFFER_H_ */

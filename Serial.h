#ifndef __SERIAL_H__
#define __SERIAL_H__

#include "stm32f4xx.h"
#include "stdio.h"
#include "String.h"

#define SERIAL_BUFFER_SIZE 100

extern char GlobalStringBuffer[100];
extern char Serial_RxBuffer[SERIAL_BUFFER_SIZE];
extern u16 Serial_RxReadPos;
extern u16 Serial_RxCounter;

void Serial_Init(void);
void Serial_NVIC_Config(void);

void USART3_IRQHandler(void);
void Serial_Buffer_Clear(void);
int16_t Serial_Buffer_Get(void);
uint16_t Serial_Buffer_Count(void);


void Serial_WriteByte(u8 chr);
void Serial_WriteBytes(u8 *chr, u8 numBytes);
void Serial_WriteWords(u16 *word, u16 numWords);
void Serial_Print(char *chrBuf);
void Serial_Println(char *chrBuf);

#endif /* __SERIAL_H__ */

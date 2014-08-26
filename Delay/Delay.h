#ifndef __DELAY_H__
#define __DELAY_H__

#include "stm32f4xx.h"

extern __IO uint32_t TimingDelayMs;
extern __IO uint32_t TimingDelayus;
extern uint32_t MillisCounter;
extern uint32_t MicrosCounter;

void TimingDelay10us_Decrement(void);
void TimingDelayMs_Decrement(void);

void Delay_Ms(__IO uint32_t nTime);
void Delay_us(__IO uint32_t nTime);

void IncreaseMillis(void);
uint32_t Millis(void);

void Increase10Micros(void);
uint32_t Micros(void);

#endif /* __DELAY_H__ */

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "Delay.h"

__IO uint32_t TimingDelay;
uint32_t MillisCounter = 0;

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
	if (TimingDelay) TimingDelay--;
	MillisCounter++;

}

void Delay_Ms(__IO uint32_t nTime)
{ 	TimingDelay = nTime;
	while (TimingDelay != 0);
}

uint32_t Millis(void)
{
    return MillisCounter;
}

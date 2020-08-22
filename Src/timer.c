/** @file timer.c
 *
 * @brief This file contains source codes of timer.h
 */

#include "stm32f4xx_hal.h"

uint32_t value;

void timer_chronoStart()
{
	value = HAL_GetTick();
}

uint32_t timer_getTime()
{
	return HAL_GetTick() - value;
}

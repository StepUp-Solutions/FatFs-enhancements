/** @file timer.h
 *
 * @brief This file contains functions to measure time.
 * Understand "timer" as chronometer.
 * It depends on HAL drivers, to implement it without HAL drivers, just use the timers :)
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

void timer_chronoStart(void);
uint32_t timer_getTime(void);

#endif /* INC_TIMER_H_ */

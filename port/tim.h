/*
 * Minimal Timer interface methods.
 */
#ifndef __VVC_TIM
#define __VVC_TIM

// Device header file.
#include "stm32l4xx.h"

// Extern values which must be defined in the application source.
extern uint32_t SystemCoreClock;
extern volatile int cur_samples;

// Set up a timer for PWM output with a given duty cycle and freq.
// Note: The GPIO pin must be configured before calling this method.
// Note: The 'ARR' value is shared across all channels, so this
// method is capable of messing up other channels on the same timer.
void timer_pwm_out( TIM_TypeDef* TIMx, int channel,
                    float duty_cycle, int freq );

// Setup a timer to produce 'trigger output' updates
// at a given frequency.
void timer_periodic_trgo( TIM_TypeDef* TIMx, int freq_hz );
// Adjust the frequency of a timer which is currently producing
// periodic 'trigger output' updates.
void timer_adjust_trgo( TIM_TypeDef* TIMx, int freq_hz );

#endif

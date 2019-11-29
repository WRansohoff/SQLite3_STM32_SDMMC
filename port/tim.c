/*
 * Minimal Timer interface methods.
 */
#include "port/tim.h"

// Set up a timer for PWM output with a given duty cycle and freq.
// Note: The GPIO pin must be configured before calling this method.
// Note: The 'ARR' value is shared across all channels, so this
// method is capable of messing up other channels on the same timer.
void timer_pwm_out( TIM_TypeDef* TIMx, int channel,
                    float duty_cycle, int freq ) {
  if ( duty_cycle < 0 || duty_cycle > 1 ) { return; }
  if ( channel < 1 || channel > 4 ) { return; }
  // Frequency in Hz. Limit to [ 1 : (SystemClock / 2) ].
  int tfreq = freq;
  if ( tfreq > ( SystemCoreClock / 2 ) ) {
    tfreq = SystemCoreClock / 2;
  }
  else if ( tfreq < 1 ) { tfreq = 1; }
  // Calculate PSC / CCR / ARR from frequency and duty cycle.
  int psc = 0;
  int arr = SystemCoreClock / tfreq;
  if ( arr > 0xFFFF ) {
    psc = ( arr / 0xFFFF ) + 1;
    if ( psc > 0xFFFF ) { return; } /* (Timer period is too long) */
    arr = arr / psc;
  }
  int ccr = ( int )( arr * duty_cycle );
  // Configure the timer.
  TIMx->PSC      =  ( psc );
  TIMx->ARR      =  ( arr );
  if ( channel == 1 ) {
    TIMx->CCR1   =  ( ccr );
    TIMx->CCER  |=  ( TIM_CCER_CC1E );
    TIMx->CCMR1 &= ~( TIM_CCMR1_OC1M );
    TIMx->CCMR1 |=  ( 0x6 << TIM_CCMR1_OC1M_Pos );
  }
  else if ( channel == 2 ) {
    TIMx->CCR2   =  ( ccr );
    TIMx->CCER  |=  ( TIM_CCER_CC2E );
    TIMx->CCMR1 &= ~( TIM_CCMR1_OC2M );
    TIMx->CCMR1 |=  ( 0x6 << TIM_CCMR1_OC2M_Pos );
  }
  else if ( channel == 3 ) {
    TIMx->CCR3   =  ( ccr );
    TIMx->CCER  |=  ( TIM_CCER_CC3E );
    TIMx->CCMR2 &= ~( TIM_CCMR2_OC3M );
    TIMx->CCMR2 |=  ( 0x6 << TIM_CCMR2_OC3M_Pos );
  }
  else if ( channel == 4 ) {
    TIMx->CCR4   =  ( ccr );
    TIMx->CCER  |=  ( TIM_CCER_CC4E );
    TIMx->CCMR2 &= ~( TIM_CCMR2_OC4M );
    TIMx->CCMR2 |=  ( 0x6 << TIM_CCMR2_OC4M_Pos );
  }
  // Set 'update generation' bit to apply settings.
  TIMx->EGR     |=  ( TIM_EGR_UG );
  // Start the timer.
  TIMx->CR1     |=  ( TIM_CR1_CEN );
}

// Setup a timer to produce 'trigger output' updates
// at a given frequency.
void timer_periodic_trgo( TIM_TypeDef* TIMx, int freq_hz ) {
  // Set the timer frequencyh such that it updates every N Hz.
  // (N should be > X*10^2 to avoid overflowing the ARR register)
  TIMx->PSC  =  ( 0x0000 );
  TIMx->ARR  =  ( SystemCoreClock / ( freq_hz * cur_samples ) );
  // Enable trigger output on timer update events.
  TIMx->CR2 &= ~( TIM_CR2_MMS );
  TIMx->CR2 |=  ( 0x2 << TIM_CR2_MMS_Pos );
  // Start the timer.
  TIMx->CR1 |=  ( TIM_CR1_CEN );
}

// Adjust the frequency of a timer which is currently producing
// periodic 'trigger output' updates.
void timer_adjust_trgo( TIM_TypeDef* TIMx, int freq_hz ) {
  TIMx->ARR  =  ( SystemCoreClock / ( freq_hz * cur_samples ) );
}

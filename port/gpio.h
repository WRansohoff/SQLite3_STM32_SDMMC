/*
 * Minimal GPIO interface methods.
 */
#ifndef __VVC_GPIO
#define __VVC_GPIO

// Device header file.
#include "stm32l4xx.h"

// 'Quick initialization' tags
// TODO: enum?
#define GPIO_IN        ( 0 )
#define GPIO_IN_PU     ( 1 )
#define GPIO_IN_PD     ( 2 )
#define GPIO_OUT_PP    ( 3 )
#define GPIO_OUT_OD    ( 4 )
#define GPIO_OUT_OD_PU ( 5 )
#define GPIO_ANALOG    ( 6 )

/* Helper macros */
// Reset a GPIO pin's registers to 'floating input' state.
#define GPIO_CLEAR_PIN( GPIOx, p ) \
  GPIOx->OTYPER       &= ~( 0x1 << p ); \
  GPIOx->PUPDR        &= ~( 0x3 << ( p * 2 ) ); \
  GPIOx->OSPEEDR      &= ~( 0x3 << ( p * 2 ) ); \
  GPIOx->MODER        &= ~( 0x3 << ( p * 2 ) ); \
  GPIOx->AFR[ p / 8 ] &= ~( 0xF << ( ( p % 8 ) * 4 ) );
// Return the current state of a GPIO pin.
#define gpio_in( GPIOx, pin ) \
  ( GPIOx->IDR & ( 1 << ( pin ) ) );
// Pull a GPIO pin high (if it has been configured as an output.)
#define gpio_hi( GPIOx, pin ) \
  GPIOx->BSRR |=  ( 1 << ( pin ) );
// Pull a GPIO pin low (if it has been configured as an output.)
#define gpio_lo( GPIOx, pin ) \
  GPIOx->BSRR |=  ( 1 << ( pin + 16 ) );
// Toggle a GPIO pin's output state (if it is in output mode.)
#define gpio_toggle( GPIOx, pin ) \
  GPIOx->ODR  ^=  ( 1 << pin );

// Setup a GPIO pin for normal use.
void gpio_setup( GPIO_TypeDef* GPIOx, int pin, int gpio_type );

// Setup a GPIO pin for 'alternate function' mode.
void gpio_af_setup( GPIO_TypeDef* GPIOx, int pin,
                    int af_num, int ospeed );

#endif

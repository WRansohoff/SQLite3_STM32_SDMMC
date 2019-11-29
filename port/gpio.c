/*
 * Minimal GPIO interface methods.
 */
#include "port/gpio.h"

// Setup a GPIO pin for normal use.
void gpio_setup( GPIO_TypeDef* GPIOx, int pin,
                 int gpio_type ) {
  if ( pin < 0 || pin > 15 ) { return; }
  GPIO_CLEAR_PIN( GPIOx, pin );
  if      ( gpio_type == GPIO_IN ) {}
  else if ( gpio_type == GPIO_IN_PU ) {
    GPIOx->PUPDR |=  ( 0x1 << ( pin * 2 ) );
  }
  else if ( gpio_type == GPIO_IN_PD ) {
    GPIOx->PUPDR |=  ( 0x2 << ( pin * 2 ) );
  }
  else if ( gpio_type == GPIO_OUT_PP ) {
    GPIOx->MODER |=  ( 0x1 << ( pin * 2 ) );
  }
  else if ( gpio_type == GPIO_OUT_OD ) {
    GPIOx->OTYPER |=  ( 0x1 << pin );
    GPIOx->MODER  |=  ( 0x1 << ( pin * 2 ) );
  }
  else if ( gpio_type == GPIO_OUT_OD_PU ) {
    GPIOx->OTYPER |=  ( 0x1 << pin );
    GPIOx->PUPDR  |=  ( 0x1 << ( pin * 2 ) );
    GPIOx->MODER  |=  ( 0x1 << ( pin * 2 ) );
  }
  else if ( gpio_type == GPIO_ANALOG ) {
    GPIOx->MODER  |=  ( 0x3 << ( pin * 2 ) );
  }
}

// Setup a GPIO pin for 'alternate function' mode.
// TODO: This may not work with I2C without setting OTYPER to OD mode.
void gpio_af_setup( GPIO_TypeDef* GPIOx, int pin,
                    int af_num, int ospeed ) {
  if ( pin < 0 || pin > 15 ) { return; }
  GPIO_CLEAR_PIN( GPIOx, pin );
  GPIOx->MODER          |=  ( 0x2 << ( pin * 2 ) );
  GPIOx->OSPEEDR        |=  ( ( ospeed & 0x3 ) << ( pin * 2 ) );
  GPIOx->AFR[ pin / 8 ] |=  ( ( af_num & 0xF ) << ( pin % 8 ) * 4 );
}

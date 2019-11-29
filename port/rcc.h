/*
 * Minimal RCC (Reset & Clock Control) interface methods.
 */
#ifndef __VVC_RCC
#define __VVC_RCC

// Device header file.
#include "stm32l4xx.h"

// Extern values which must be defined in the application source.
extern uint32_t SystemCoreClock;

// Simple imprecise delay method.
void __attribute__( ( optimize( "O0" ) ) ) delay_cycles( uint32_t cyc );

// Placeholder method to set the core clock speed to 80MHz.
void clock_init( void );

#endif

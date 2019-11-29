/*
 * High-level BSP abstraction for the first revision
 * of the 'MotoMate' GPS handheld board. It is intended
 * to contain initialization and configuration methods
 * which depend on specific pin numberings.
 * (As a test platform for the sqlite3 library, I've
 *  commented out initialization of unrelated parts.)
 */
#ifndef __VVC_MOTOMATE_BSP
#define __VVC_MOTOMATE_BSP

// Device header file.
#include "stm32l4xx.h"

// HAL includes.
//#include "port/adc.h"
//#include "port/dma.h"
#include "port/gpio.h"
#include "port/rcc.h"
#include "port/sdmmc.h"
//#include "port/spi.h"
#include "port/tim.h"
//#include "port/uart.h"

// BSP includes.
//#include "ili9341.h"
//#include "ringbuf.h"
//#include "ufb.h"

// Variables that are defined elsewhere.
// Audio settings / wave buffer.
//extern volatile int cur_samples;
//extern volatile int cur_hz;
//extern volatile uint16_t SINE_WAVE[ MAX_SINE_SAMPLES ];
// Display settings and framebuffer.
//extern volatile uint16_t FRAMEBUFFER[ ILI9341_A ];
//extern volatile float tft_brightness;
// Ringbuffer to receive from the UART GPS module.
//extern ringbuf gps_rb;
// Current core clock speed, in Hertz.
extern uint32_t SystemCoreClock;

/*
 * Initialize the board's pins and peripherals
 * to prepare it for use.
 * TODO (long-term): USB, SD/MMC, aux I2C/UARTs, touch screen pins.
 * Sorry, right now this is an entire BSP initialization method for
 * an existing board with most parts commented-out. TODO: Cleanup.
 */
static inline void board_init( void ) {
  // Initialize the core clock speed to 80MHz.
  clock_init();

  // Enable peripherals: GPIOA, GPIOB, GPIOC, GPIOD,
  // UART4, ADC, DAC1, DMA1, SPI1, TIM3, SDMMC1, SYSCFG.
  RCC->AHB1ENR  |=  ( RCC_AHB1ENR_DMA1EN );
  RCC->AHB2ENR  |=  ( RCC_AHB2ENR_GPIOAEN |
                      RCC_AHB2ENR_GPIOBEN |
                      RCC_AHB2ENR_GPIOCEN |
                      RCC_AHB2ENR_GPIODEN |
                      RCC_AHB2ENR_ADCEN );
  RCC->APB1ENR1 |=  ( RCC_APB1ENR1_DAC1EN |
                      RCC_APB1ENR1_TIM3EN |
                      RCC_APB1ENR1_UART4EN );
  RCC->APB2ENR  |=  ( RCC_APB2ENR_SDMMC1EN |
                      RCC_APB2ENR_SPI1EN |
                      RCC_APB2ENR_SYSCFGEN );

  // Enable FPU. (Set Coprocessors 1 & 2 to 'full access')
  // Trying to use floats before this is called will cause a crash.
  SCB->CPACR    |=  ( 0xF << 20 );

  // Setup GPIO pin(s).
  /*
  // PA0, PA1: Alt. Func. #8, low-speed (UART4 TX, RX)
  gpio_af_setup( GPIOA, 0, 8, 0 );
  gpio_af_setup( GPIOA, 1, 8, 0 );
  // PC2: Analog mode (battery check ADC input)
  gpio_setup( GPIOC, 2, GPIO_ANALOG );
  // PA4: Analog (DAC1, Channel 1)
  gpio_setup( GPIOA, 4, GPIO_ANALOG );
  // PB1: Alt. Func. #2 (TIM3_CH4), low-speed (TFT backlight control)
  gpio_af_setup( GPIOB, 1, 2, 0 );
  // PB0: Push-pull output (TFT CS pin)
  gpio_setup( GPIOB, 0, GPIO_OUT_PP );
  gpio_hi( GPIOB, 0 );
  // PC5: Push-pull output (TFT Reset pin)
  gpio_setup( GPIOC, 5, GPIO_OUT_PP );
  gpio_hi( GPIOC, 5 );
  // PC4: Push-pull output (TFT Data/Command pin)
  gpio_setup( GPIOC, 4, GPIO_OUT_PP );
  // PA5, PA6, PA7: Alt. Func. #5, mid-speed (SPI1 SCK/SDO/SDI)
  gpio_af_setup( GPIOA, 5, 5, 1 );
  gpio_af_setup( GPIOA, 6, 5, 1 );
  gpio_af_setup( GPIOA, 7, 5, 1 );
  // PA15: 'Heartbeat' LED. (Note: overrides JTDI debugging pin)
  gpio_setup( GPIOA, 15, GPIO_OUT_PP );
  gpio_lo( GPIOA, 15 );
  */
  // PC8, PC9, PC10, PC11, PC12: Alt. Func. #12 / high-speed:
  // (SD/MMC dat0, dat1, dat2, dat3, and clock pins)
  gpio_af_setup( GPIOC, 8,  12, 2 );
  gpio_af_setup( GPIOC, 9,  12, 2 );
  gpio_af_setup( GPIOC, 10, 12, 2 );
  gpio_af_setup( GPIOC, 11, 12, 2 );
  gpio_af_setup( GPIOC, 12, 12, 2 );
  // PD2: Alt. Func. #12 / high-speed: SD/MMC command pin.
  gpio_af_setup( GPIOD, 2,  12, 2 );

  /*
  // Setup button pins as inputs with pull-up resistors enabled.
  // PB3, PB4, PB5, C6, C7: Navigation switch button inputs.
  gpio_setup( GPIOB, 3, GPIO_IN_PU );
  gpio_setup( GPIOB, 4, GPIO_IN_PU );
  gpio_setup( GPIOB, 5, GPIO_IN_PU );
  gpio_setup( GPIOC, 6, GPIO_IN_PU );
  gpio_setup( GPIOC, 7, GPIO_IN_PU );
  // PA8: 'Mode' button input.
  gpio_setup( GPIOA, 8, GPIO_IN_PU );
  // PB14: 'Back' button input.
  gpio_setup( GPIOB, 14, GPIO_IN_PU );

  // Enable EXTI interrupts for button pins.
  // Setup GPIO port mappings for each EXTI line.
  SYSCFG->EXTICR[ 0 ] &= ~( SYSCFG_EXTICR1_EXTI3_Msk );
  SYSCFG->EXTICR[ 0 ] |=  ( SYSCFG_EXTICR1_EXTI3_PB );
  SYSCFG->EXTICR[ 1 ] &= ~( SYSCFG_EXTICR2_EXTI4_Msk |
                            SYSCFG_EXTICR2_EXTI5_Msk |
                            SYSCFG_EXTICR2_EXTI6_Msk |
                            SYSCFG_EXTICR2_EXTI7_Msk );
  SYSCFG->EXTICR[ 1 ] |=  ( SYSCFG_EXTICR2_EXTI4_PB |
                            SYSCFG_EXTICR2_EXTI5_PB |
                            SYSCFG_EXTICR2_EXTI6_PC |
                            SYSCFG_EXTICR2_EXTI7_PC );
  SYSCFG->EXTICR[ 2 ] &= ~( SYSCFG_EXTICR3_EXTI8_Msk );
  SYSCFG->EXTICR[ 2 ] |=  ( SYSCFG_EXTICR3_EXTI8_PA );
  SYSCFG->EXTICR[ 3 ] &= ~( SYSCFG_EXTICR4_EXTI14_Msk );
  SYSCFG->EXTICR[ 3 ] |=  ( SYSCFG_EXTICR4_EXTI14_PB );
  // Enable interrupts for the given pins.
  EXTI->IMR1          |=  ( EXTI_IMR1_IM3 |
                            EXTI_IMR1_IM4 |
                            EXTI_IMR1_IM5 |
                            EXTI_IMR1_IM6 |
                            EXTI_IMR1_IM7 |
                            EXTI_IMR1_IM8 |
                            EXTI_IMR1_IM14 );
  // Disable 'rising edge' interrupts.
  EXTI->RTSR1         &= ~( EXTI_RTSR1_RT3 |
                            EXTI_RTSR1_RT4 |
                            EXTI_RTSR1_RT5 |
                            EXTI_RTSR1_RT6 |
                            EXTI_RTSR1_RT7 |
                            EXTI_RTSR1_RT8 |
                            EXTI_RTSR1_RT14 );
  // Enable 'falling edge' interrupts.
  EXTI->FTSR1         |=  ( EXTI_FTSR1_FT3 |
                            EXTI_FTSR1_FT4 |
                            EXTI_FTSR1_FT5 |
                            EXTI_FTSR1_FT6 |
                            EXTI_FTSR1_FT7 |
                            EXTI_FTSR1_FT8 |
                            EXTI_FTSR1_FT14 );
  */

  // Setup the NVIC hardware interrupts.
  // Use 4 bits for 'priority' and 0 bits for 'subpriority'.
  NVIC_SetPriorityGrouping( 0x00 );
  /*
  // DMA interrupts should be high-priority. (0 is highest)
  uint32_t dma_pri_encoding = NVIC_EncodePriority( 0x00, 0x01, 0x00 );
  NVIC_SetPriority( DMA1_Channel3_IRQn, dma_pri_encoding );
  NVIC_EnableIRQ( DMA1_Channel3_IRQn );
  // UART receive interrupts should be highest-priority.
  uint32_t uart_pri_encoding = NVIC_EncodePriority( 0x00, 0x00, 0x00 );
  NVIC_SetPriority( UART4_IRQn, uart_pri_encoding );
  NVIC_EnableIRQ( UART4_IRQn );
  // Button inputs should be fairly low priority (15 is lowest)
  uint32_t btn_pri_encoding = NVIC_EncodePriority( 0x00, 0x0C, 0x00 );
  NVIC_SetPriority( EXTI3_IRQn, btn_pri_encoding );
  NVIC_EnableIRQ( EXTI3_IRQn );
  NVIC_SetPriority( EXTI4_IRQn, btn_pri_encoding );
  NVIC_EnableIRQ( EXTI4_IRQn );
  NVIC_SetPriority( EXTI9_5_IRQn, btn_pri_encoding );
  NVIC_EnableIRQ( EXTI9_5_IRQn );
  NVIC_SetPriority( EXTI15_10_IRQn, btn_pri_encoding );
  NVIC_EnableIRQ( EXTI15_10_IRQn );
  */

  /*
  // Select the system clock as the ADC clock source.
  adc_clock_source( ADC_CLOCK_SYSCLK );
  // Turn on the ADC and calibrate it.
  adc_on( ADC1, 1 );
  // Setup ADC for a single conversion on channel 3.
  adc_conversion battery_adc = {
    channel: 3,
    sample_time: ADC_SAMP_640_CYC
  };
  adc_sequence_config( ADC1, &battery_adc, 1 );
  // Enable the ADC.
  ADC1->CR   |=   ( ADC_CR_ADEN );
  */

  /*
  // Ensure there is always a null terminator at the end of the
  // UART receive ringbuffer.
  gps_rb.buf[ GPS_RINGBUF_LEN ] = '\0';
  // Initialize UART4 for 9600-baud communication
  // with a receive timeout of 10 cycles.
  uart_on( UART4, 9600, 10 );

  // DMA configuration (DMA1, channel 3: SPI1 transmit).
  dma_config_tx( DMA1_BASE, 3,
                 ( uint32_t )&FRAMEBUFFER,
                 ( uint32_t )&( SPI1->DR ),
                 ( uint16_t )( ILI9341_A / 2 ),
                 1, DMA_PRIORITY_HI, DMA_SIZE_16b, 0, 1 );
  // Configure DMA1, channel4 for audio DAC transfers.
  dma_config_tx( DMA1_BASE, 4,
                 ( uint32_t )&SINE_WAVE,
                 ( uint32_t )&( DAC1->DHR12R1 ),
                 ( uint16_t )( cur_samples ),
                 5, DMA_PRIORITY_MID, DMA_SIZE_16b, 1, 0 );

  // Set Timer 3, Channel 4 to a 1MHz PWM signal
  // with the current display brightness.
  timer_pwm_out( TIM3, 4, tft_brightness, 1000000 );
  // Configure TIM7 to trigger sending audio samples to the DAC.
  timer_periodic_trgo( TIM7, cur_hz );

  // Setup DAC1 for sending audio samples.
  // Set trigger source to TIM7 TRGO (TRiGger Output).
  DAC1->CR &= ~( DAC_CR_TSEL1 );
  DAC1->CR |=  ( 0x2 << DAC_CR_TSEL1_Pos );
  // Enable DAC DMA requests for channel 1.
  DAC1->CR |=  ( DAC_CR_DMAEN1 );
  // Enable DAC channel 1.
  DAC1->CR |=  ( DAC_CR_EN1 );
  // (Initialize display before enabling DAC triggers to
  //  allow time for the sampling to stabilize.)
  */

  /*
  // Setup SPI1 for communicating with the TFT.
  spi_host_init( SPI1, 0, 1 );
  // Send initialization commands to the display.
  ili9341_init( SPI1 );
  // Enable DMA1, Channel 3 (Display TX).
  DMA1_Channel3->CCR |= ( DMA_CCR_EN );
  */

  // Setup the SD/MMC interface.
  sdmmc_setup( SDMMC1 );

  /*
  // Enable DMA1, Channel 4 (Audio TX).
  DMA1_Channel4->CCR |= ( DMA_CCR_EN );
  // Enable the DAC timer trigger.
  DAC1->CR |=  ( DAC_CR_TEN1 );

  // Ensure that the ADC is ready before starting.
  while ( !( ADC1->ISR & ADC_ISR_ADRDY ) ) {};
  */
}

#endif

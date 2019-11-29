/*
 * Minimal SD/MMC peripheral interface methods.
 */
#include "port/sdmmc.h"

/**
 * Setup an SD/MMC peripheral for simple 'polling mode'.
 * This is slow; no interrupts, hardware flow control, or DMA.
 */
void sdmmc_setup( SDMMC_TypeDef *SDMMCx ) {
  // Clock control register:
  // * Set the interface to use the rising edge of clock signals.
  // * Disable clock bypass. (SDMMC_CK = SDMMCCLK)
  // * Disable hardware flow control for now. (TODO: Use hw flow ctrl)
  // * Set bus width to 4 bits (microSD cards have DAT0-3 lines)
  // * Disable power-saving mode for now. (TODO: Use PWRSAV bit)
  // * Set CLKDIV to 0 (even though it is not used with clock bypass)
  SDMMC1->CLKCR &= ~( SDMMC_CLKCR_CLKDIV |
                      SDMMC_CLKCR_PWRSAV |
                      SDMMC_CLKCR_BYPASS |
                      SDMMC_CLKCR_WIDBUS |
                      SDMMC_CLKCR_NEGEDGE |
                      SDMMC_CLKCR_HWFC_EN );
  SDMMC1->CLKCR |=  ( 0x1 << SDMMC_CLKCR_WIDBUS_Pos );
  // Power on the SD/MMC peripheral.
  SDMMC1->POWER |=  ( SDMMC_POWER_PWRCTRL );
  // Wait for the peripheral to exit 'powering up' mode.
  while ( ( SDMMCx->POWER & SDMMC_POWER_PWRCTRL ) == 0x2 ) {};
}

/** Send a command to the SD/MMC card. */
void sdmmc_cmd_write( SDMMC_TypeDef *SDMMCx,
                      uint32_t cmd,
                      uint32_t dat ) {
  // The `ARG` register holds the command argument / data.
  // TODO
}

/** Read the ID / category of a card's response to a command. */
uint8_t sdmmc_cmd_read_type( SDMMC_TypeDef *SDMMCx ) {
  // TODO
  return 0;
}

/**
 * Read the contents of one of the SDMMC peripheral's 'command
 * response registers'. This can receive the response from a command.
 * If `type` is `SDMMC_RESPONSE_SHORT`, only the first word is read.
 * If `type` is `SDMMC_RESPONSE_LONG`, all 4 words are read.
 */
uint32_t sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx, int type, void *buf ) {
  // TODO
  return 0;
}

/**
 * Send a command to tell the connected SD card to use a specified
 * block size. I think that the block size is defined in bytes.
 * This method blocks on the SD card's response, which is returned.
 */
uint32_t sdmmc_set_block_len( SDMMC_TypeDef *SDMMCx, uint32_t bsize ) {
  // TODO
  return 0;
}

/**
 * Send a command to tell the connected SD card to use a specified
 * bus width. This method blocks on and returns the SD card response.
 */
uint32_t sdmmc_set_bus_width( SDMMC_TypeDef *SDMMCx, uint32_t width ) {
  // TODO
  return 0;
}

/**
 * Figure out how much storage capacity the SD card (claims to) have.
 */
uint32_t sdmmc_get_volume_size( SDMMC_TypeDef *SDMMCx ) {
  // TODO
  return 0;
}

/** Read N blocks of data from an address on the SD/MMC card. */
void sdmmc_block_read( SDMMC_TypeDef *SDMMCx,
                       blockno_t start_block,
                       void *buf,
                       int blen ) {
  // TODO
}

/** Write N blocks of data to an address on the SD/MMC card. */
void sdmmc_block_write( SDMMC_TypeDef *SDMMCx,
                        blockno_t start_block,
                        void *buf,
                        int blen ) {
  // TODO
}

/**
 * Erase a block range on the SD/MMC card, given start ID and length.
 */
void sdmmc_erase_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         blockno_t num_blocks ) {
  // TODO
}

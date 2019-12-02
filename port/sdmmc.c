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
  SDMMCx->CLKCR &= ~( SDMMC_CLKCR_CLKDIV |
                      SDMMC_CLKCR_PWRSAV |
                      SDMMC_CLKCR_BYPASS |
                      SDMMC_CLKCR_WIDBUS |
                      SDMMC_CLKCR_NEGEDGE |
                      SDMMC_CLKCR_HWFC_EN );
  SDMMCx->CLKCR |=  ( 0x1 << SDMMC_CLKCR_WIDBUS_Pos );
  // Power on the SD/MMC peripheral.
  SDMMCx->POWER |=  ( SDMMC_POWER_PWRCTRL );
  // Wait for the peripheral to exit 'powering up' mode.
  while ( ( SDMMCx->POWER & SDMMC_POWER_PWRCTRL ) == 0x2 ) {};
  // TODO: From the RM, it sounds like delaying a few cycles here
  // might be a good idea to ensure the power supply is stable?
}

/** Send a command to the SD/MMC card. */
void sdmmc_cmd_write( SDMMC_TypeDef *SDMMCx,
                      uint32_t cmd,
                      uint32_t dat,
                      int resp_type ) {
  // The `ARG` register holds the command argument / data.
  SDMMCx->ARG  =  ( dat );
  // Set the `CMD` ('command') register. Use the internal `CPSM`
  // state machine to auto-receive the response.
  SDMMCx->CMD &= ~( SDMMC_CMD_CMDINDEX |
                    SDMMC_CMD_CPSMEN |
                    SDMMC_CMD_WAITRESP |
                    SDMMC_CMD_WAITINT |
                    SDMMC_CMD_WAITPEND |
                    SDMMC_CMD_SDIOSUSPEND );
  SDMMCx->CMD |=  ( ( ( resp_type & 0x3 ) << SDMMC_CMD_WAITRESP_Pos ) |
                    SDMMC_CMD_CPSMEN |
                    ( cmd << SDMMC_CMD_CMDINDEX_Pos ) );
}

/** Read the ID / category of a card's response to a command. */
uint8_t sdmmc_cmd_read_type( SDMMC_TypeDef *SDMMCx ) {
  // Wait for a response to be received, or some sort of failure.
  while ( !( SDMMCx->STA & ( SDMMC_STA_CMDSENT |
                             SDMMC_STA_CMDREND |
                             SDMMC_STA_CTIMEOUT |
                             SDMMC_STA_CCRCFAIL ) ) ) {};
  // If a command had a response, return its type.
  if ( SDMMCx->STA & SDMMC_STA_CMDREND ) {
    return ( SDMMCx->RESPCMD & SDMMC_RESPCMD_RESPCMD );
  }
  // If there is no response and no error, return 0. I think that
  // CMDSENT indicates that no response was asked for, so this is OK.
  else if ( SDMMCx->STA & SDMMC_STA_CMDSENT ) { return 0x00; }
  // If there is an error, return 0xFF. This is the usual 'nothing
  // happened' response when SPI is used, so...sure.
  return 0xFF;
}

/**
 * Read the contents of one of the SDMMC peripheral's 'command
 * response registers'. This can receive the response from a command.
 * If `type` is `SDMMC_RESPONSE_SHORT`, only the first word is read.
 * If `type` is `SDMMC_RESPONSE_LONG`, all 4 words are read.
 */
int sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx, int type, void *buf ) {
  uint32_t *buf_ptr = ( uint32_t* )buf;
  // Wait for a response to be received, or some sort of failure.
  while ( !( SDMMCx->STA & ( SDMMC_STA_CMDSENT |
                             SDMMC_STA_CMDREND |
                             SDMMC_STA_CTIMEOUT |
                             SDMMC_STA_CCRCFAIL ) ) ) {};
  if ( SDMMCx->STA & SDMMC_STA_CMDREND ) {
    // Only check for received data if a valid response was received.
    if ( type == SDMMC_RESPONSE_SHORT ) {
      // Only read the first response register for short responses.
      buf_ptr[ 0 ] = SDMMCx->RESP1;
      // Success; return 0.
      return 0;
    }
    else if ( type == SDMMC_RESPONSE_LONG ) {
      // Read all four response registers for long responses.
      // Note that `RESP1` holds the most significant bytes.
      buf_ptr[ 0 ] = SDMMCx->RESP4;
      buf_ptr[ 1 ] = SDMMCx->RESP3;
      buf_ptr[ 2 ] = SDMMCx->RESP2;
      buf_ptr[ 3 ] = SDMMCx->RESP1;
      // Success; return 0.
      return 0;
    }
  }
  else if ( SDMMCx->STA & SDMMC_STA_CMDSENT ) {
    // No response expected, and no errors: return 0.
    return 0;
  }
  // No valid response received, or invalid response type: return -1.
  return -1;
}

/**
 * Acknowledge that a command response was received. This does not
 * send any data to the SD card; it just resets internal state.
 * TODO: I put this in a separate method so that the `read_type`
 * and `read` methods could be called independently, but it would
 * probably be better to make a struct to hold holistic response
 * data and put all of this logic into one method.
 */
void sdmmc_cmd_done( SDMMC_TypeDef *SDMMCx ) {
  // Wait for one of the relevant status flags to be set.
  // TODO: Add a timeout, or better yet make this part of
  // the 'read response' methods.
  while ( !( SDMMCx->STA & ( SDMMC_STA_CMDSENT |
                             SDMMC_STA_CMDREND |
                             SDMMC_STA_CTIMEOUT |
                             SDMMC_STA_CCRCFAIL ) ) ) {};
  // Clear all 'command receive' status and error flags.
  // Like many STM32 peripherals, setting a bit in the 'clear' register
  // actually clears the corresponding bit in the 'status' register.
  SDMMCx->ICR |= ( SDMMC_ICR_CMDSENTC |
                   SDMMC_ICR_CMDRENDC |
                   SDMMC_ICR_CTIMEOUTC |
                   SDMMC_ICR_CCRCFAILC );
}

/**
 * Send a command to tell the connected SD card to use a specified
 * block size. I think that the block size is defined in bytes.
 */
void sdmmc_set_block_len( SDMMC_TypeDef *SDMMCx,
                          uint32_t bsize ) {
  // CMD16 to set block size. High-capacity cards should not
  // use this command, since their block size is fixed internally.
  // TODO: Check response for errors.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SET_BLOCKLEN,
                   bsize,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );
}

/**
 * Send a command to tell the connected SD card to use a specified
 * bus width. This method blocks on and returns the SD card response.
 */
void sdmmc_set_bus_width( SDMMC_TypeDef *SDMMCx,
                          uint16_t card_addr,
                          uint32_t width ) {
  // CMD7 to select the card.
  // TODO: This returns an 'R1b' response, which means that
  // the `DAT0` wire is held low until the card is not busy.
  // So...should I wait for the line to go high here by checking
  // the GPIO IDR register, or is the STM32 SDMMC peripheral smart
  // enough to not set its status flags until the busy signal clears?
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card_addr ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );

  // App CMD6 to set the data bus width to 4. Apparently
  // this can only be done when the card is in 'transmission mode',
  // which it enters when selected by CMD7. But I'm not exactly
  // sure if this needs to be set for each transaction, or if it will
  // stick after being set in this method. I'm hoping for the latter.
  // (CMD55 needs to precede application commands)
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_APP,
                   0x00000000,
                   SDMMC_RESPONSE_NONE );
  sdmmc_cmd_done( SDMMCx );
  // TODO: Check response for errors.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_APP_SET_BUSW,
                   width,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );

  // CMD7 to de-select the card.
  // Sending any address except the one that the card previously
  // published will de-select it. I'm not sure if there are any
  // guaranteed invalid addresses, so for now, I'll just
  // XOR the card's address to change all of its bits.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card_addr ^ 0x0000FFFF ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );
}

/**
 * Figure out how much storage capacity the SD card (claims to) have.
 * This method returns the number of 512-byte blocks in the card,
 * so multiply it by 512 if you want the number of bytes.
 * TODO: Send CMD9 inside of this method instead of requiring the
 * raw CSD registers to be passed in.
 *
 * Okay, there are a lot of fields in the 'Chip-Specific Data'
 * response. Bit 127 indicates whether the card is standard- or
 * high-capacity. (TODO: Check instead of requiring an extra argument.)
 * SC cards have different responses from HC cards. For SC cards, the
 * card's storage capacity in bytes =
 * (SIZE_C+1)*2^(C_SIZE_MULT+2)*2^(READ_BL_LEN), where SIZE_C is bits
 * 62-73, C_SIZE_MULT is bits 47-49, and READ_BL_LEN is bits 80-83.
 * SDHC / SDXC cards are easier:
 * capacity (Kbytes) = (SIZE_C+1)*512, and SIZE_C is bits 48-69.
 */
uint32_t sdmmc_get_volume_size( SDMMC_TypeDef *SDMMCx,
                                uint32_t card_type,
                                uint32_t *csd_regs ) {
  if ( card_type == SDMMC_HC ) {
    // High-capacity card. Retrieve `SIZE_C` from bits 48-69.
    uint32_t size_c = ( ( csd_regs[ 1 ] >> 16 ) & 0x0000FFFF ) |
                      ( ( csd_regs[ 2 ] & 0x0000003F ) << 16 );
    // Total capacity is the value * 512KB. Divide by 512 to get the
    // number of blocks, so `card.blocks` = ( `size_c` + 1 ) * 1024.
    return ( size_c + 1 ) * 1024;
  }
  else {
    // Get `size_c` from bits 62-73.
    uint32_t size_c = csd_regs[ 3 ] & 0x00003FFF;
    // Get `c_size_mult` from bits 47-49.
    uint32_t c_size_mult = ( csd_regs[ 2 ] & 0x00038000 ) >> 15;
    // Get `read_bl_len` from bits 80-83.
    uint32_t read_bl_len = ( csd_regs[ 3 ] & 0x000F0000 ) >> 16;
    // Calculate number of 512-byte blocks. (See above comment block)
    // Note: ( X * 2^Y ) = X << Y, which simplifies this a bit.
    return ( ( ( size_c + 1 ) <<
           ( c_size_mult + 2 ) ) <<
           read_bl_len ) / 512;
  }
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

/*
 * Minimal SD/MMC peripheral interface methods.
 */
#include "port/sdmmc.h"

/**
 * Setup an SD/MMC peripheral for simple 'polling mode'.
 * This is slow; no interrupts, hardware flow control, or DMA.
 */
void sdmmc_setup( SDMMC_TypeDef *SDMMCx ) {
  // Power on the SD/MMC peripheral.
  SDMMCx->POWER |=  ( SDMMC_POWER_PWRCTRL );
  // Wait for the peripheral to exit 'powering up' mode.
  while ( ( SDMMCx->POWER & SDMMC_POWER_PWRCTRL ) == 0x2 ) {};
  // Clock control register:
  // * Set the interface to use the rising edge of clock signals.
  // * Disable clock bypass.
  // * Disable hardware flow control for now. (TODO: Use hw flow ctrl)
  // * Set bus width to 4 bits (microSD cards have DAT0-3 lines)
  //   (TODO: Currently using 1 bit for debugging)
  // * Disable power-saving mode for now. (TODO: Use PWRSAV bit)
  // * Set CLKDIV to 0x76 for slow speeds.
  //   Technically, the speed should be <=400KHz until init is done.
  // * Set CLKEN to enable the clock.
  SDMMCx->CLKCR &= ~( SDMMC_CLKCR_CLKDIV |
                      SDMMC_CLKCR_WIDBUS |
                      SDMMC_CLKCR_NEGEDGE |
                      SDMMC_CLKCR_BYPASS |
                      SDMMC_CLKCR_PWRSAV |
                      SDMMC_CLKCR_HWFC_EN );
  SDMMCx->CLKCR |=  ( 0x76 << SDMMC_CLKCR_CLKDIV_Pos |
                      //0x1 << SDMMC_CLKCR_WIDBUS_Pos |
                      SDMMC_CLKCR_CLKEN );
  // Set the card block size to 512 bytes.
  // TODO: It might not be in all cases, but for now this HAL assumes
  // that you will set standard-capacity cards to use 512B blocks.
  SDMMCx->DCTRL &= ~( SDMMC_DCTRL_DBLOCKSIZE );
  SDMMCx->DCTRL |=  ( 9 << SDMMC_DCTRL_DBLOCKSIZE_Pos );
  // Set the data timeout. For now, just use a fairly long value.
  SDMMCx->DTIMER =  ( 0x04000000 );
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
 * If `ignore_crc` is non-zero, then `SDMMC_STA_CCRCFAIL` will be a
 * treated as a successful response. This is necessary for some
 * application commands which do not send a CRC with the response.
 */
int sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx,
                    int type,
                    int ignore_crc,
                    void *buf ) {
  uint32_t *buf_ptr = ( uint32_t* )buf;
  // Wait for a response to be received, or some sort of failure.
  while ( !( SDMMCx->STA & ( SDMMC_STA_CMDSENT |
                             SDMMC_STA_CMDREND |
                             SDMMC_STA_CTIMEOUT |
                             SDMMC_STA_CCRCFAIL ) ) ) {};
  if ( ( SDMMCx->STA & SDMMC_STA_CMDREND ) ||
       ( ( SDMMCx->STA & SDMMC_STA_CCRCFAIL ) &&
         ( ignore_crc && ( ( type == SDMMC_RESPONSE_SHORT ) ||
                           ( type == SDMMC_RESPONSE_LONG ) ) ) ) ) {
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
  else if ( SDMMCx->STA & SDMMC_STA_CMDSENT ||
            ( ( SDMMCx->STA & SDMMC_STA_CCRCFAIL ) &&
            ( ignore_crc && ( ( type == SDMMC_RESPONSE_NONE2 ) ||
            ( type == SDMMC_RESPONSE_NONE ) ) ) ) ) {
    // No response expected, and no unexpected errors: return 0.
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

  // App CMD6 to set the data bus width. Apparently
  // this can only be done when the card is in 'transmission mode',
  // which it enters when selected by CMD7. But I'm not exactly
  // sure if this needs to be set for each transaction, or if it will
  // stick after being set in this method. I'm hoping for the latter.
  // (CMD55 needs to precede application commands)
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_APP,
                   0x00000000,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );
  // TODO: Check response for errors.
  uint32_t resp;
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_APP_SET_BUSW,
                   width,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                  SDMMC_CHECK_CRC, &resp );
  sdmmc_cmd_done( SDMMCx );

  // CMD7 to de-select the card.
  // It sounds like 0 is a reserved address, and sending it
  // should de-select all cards.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   0x00000000,
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

/**
 * Check if an SD/MMC card with the given address is currently busy
 * or not. Return non-zero if the card is busy, 0 if it isn't.
 * TODO: Should this also check the DAT0 line for 'R1b' responses?
 */
int sdmmc_is_card_busy( SDMMC_TypeDef *SDMMCx, uint16_t card_addr ) {
  // Send CMD13 to get the card's status register.
  uint32_t sd_stat_reg = 0;
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_GET_STAT,
                   ( ( uint32_t )card_addr ) << 16,
                   SDMMC_RESPONSE_SHORT );
  // Read the response.
  sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                  SDMMC_CHECK_CRC, &sd_stat_reg );
  sdmmc_cmd_done( SDMMCx );
  // The current card state is stored in bits 9-12 of the response.
  // 0 = idle, 1 = ready, 2 = ident, 3 = standby, 4 = selected for
  // data transmission, 5 = transmitting, 6 = receiving,
  // 7 = programming, 8 = disabled, 9-15 are reserved.
  // I'm going to return 'busy' for all modes except for 'ready',
  // 'standby', 'idle', 'ident', and 'selected for data transmission'.
  // In reality, after initialization the card should either be
  // in state #3 if it is not selected, or state #4 if it is.
  // Otherwise, it should be considered busy. But this method
  // might be called during the identification process, so better
  // safe than sorry.
  uint32_t cur_state = ( sd_stat_reg & 0x00008E00 ) >> 9;
  if ( ( cur_state == SDMMC_STATE_IDLE ) |
       ( cur_state == SDMMC_STATE_READY ) |
       ( cur_state == SDMMC_STATE_IDENT ) |
       ( cur_state == SDMMC_STATE_STBY ) |
       ( cur_state == SDMMC_STATE_TRAN ) ) {
    return 0;
  }
  // If the card is busy, return its state (which will be non-zero.)
  return cur_state;
}

/**
 * Read one block of data from an address on the SD/MMC card.
 * Standard-capacity cards take the byte offset of the starting
 * address as an argument, while high-capacity cards take the
 * block offset. TODO: This assumes that the block length is
 * always 512 bytes, but standard-capacity cards can have
 * a different block size. I plan to always set SC cards to
 * use 512-byte blocks, so I'm okay with that assumption.
 */
void sdmmc_read_block( SDMMC_TypeDef *SDMMCx,
                       uint32_t card_type,
                       uint16_t card_addr,
                       blockno_t start_block,
                       uint32_t *buf ) {
  // Clear the data control register.
  SDMMCx->DCTRL &= ~( SDMMC_DCTRL_DTDIR |
                      SDMMC_DCTRL_DTEN );

  uint32_t resp;
  // Calculate the command argument.
  uint32_t start_addr = ( uint32_t )start_block;
  if ( card_type == SDMMC_SC ) { start_addr *= 512; }

  // CMD7 to select the card. (TODO: Check for success)
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card_addr ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );

  // Prepare for read: set data length.
  SDMMCx->DLEN   =  ( 512 );
  // Set the data control register for 'card-to-controller' data
  // flow, and enable the data flow state machine.
  // Note: DTEN does not need to be cleared until the next transfer.
  SDMMCx->DCTRL |=  ( SDMMC_DCTRL_DTDIR |
                      SDMMC_DCTRL_DTEN );

  // CMD17 to read a single block at the given address.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_READ_BLOCK,
                   start_addr,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                  SDMMC_CHECK_CRC, &resp );
  sdmmc_cmd_done( SDMMCx );

  // Read the data from the FIFO buffer as it becomes available.
  // TODO: Synchronous polling is slow, but okay for testing.
  int buf_ind = 0;
  while ( SDMMCx->DCOUNT != 0 ) {
    while ( !( SDMMCx->STA & SDMMC_STA_RXDAVL ) ) {};
    buf[ buf_ind ] = SDMMCx->FIFO;
    ++buf_ind;
  }

  // Done reading; CMD7 to de-select the card.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   0x00000000,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );
}

/** Read N blocks of data from an address on the SD/MMC card. */
void sdmmc_read_blocks( SDMMC_TypeDef *SDMMCx,
                        blockno_t start_block,
                        uint32_t *buf,
                        int blen ) {
  // TODO: CMD23 / CMD18 to read multiple blocks.
}

/** Write one block of data to an address on the SD/MMC card. */
void sdmmc_write_block( SDMMC_TypeDef *SDMMCx,
                        uint32_t card_type,
                        uint16_t card_addr,
                        blockno_t start_block,
                        uint32_t *buf ) {
  // Clear the data control register.
  SDMMCx->DCTRL &= ~( SDMMC_DCTRL_DTDIR |
                      SDMMC_DCTRL_DTEN );

  // Calculate the command argument.
  uint32_t start_addr = ( uint32_t )start_block;
  if ( card_type == SDMMC_SC ) { start_addr *= 512; }

  // CMD7 to select the card. (TODO: Check for success)
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card_addr ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );

  // Poll CMD13 until 'ready for data' is set.
  uint32_t resp = 0x00000000;
  while ( !( resp & SDMMC_READY_FOR_DATA ) ) {
    sdmmc_cmd_write( SDMMCx,
                     SDMMC_CMD_GET_STAT,
                     ( ( uint32_t )card_addr ) << 16,
                     SDMMC_RESPONSE_SHORT );
    sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                    SDMMC_CHECK_CRC, &resp );
    sdmmc_cmd_done( SDMMCx );
  }

  // CMD24 to write a block of data.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_WRITE_BLOCK,
                   start_addr,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                  SDMMC_CHECK_CRC, &resp );
  sdmmc_cmd_done( SDMMCx );

  // Clear the 'DATAEND' and 'DBCKEND' flags.
  SDMMCx->ICR   |=  ( SDMMC_ICR_DATAENDC | SDMMC_ICR_DBCKENDC );
  // Prepare for write: set data length.
  SDMMCx->DLEN   =  ( 512 );
  // Note: DTEN does not need to be cleared until the next transfer.
  SDMMCx->DCTRL |=  ( SDMMC_DCTRL_DTEN );

  // Write data to the FIFO buffer as space frees up.
  // TODO: Synchronous polling is slow, but okay for testing.
  int buf_ind = 1;
  SDMMCx->FIFO = buf[ 0 ];
  while ( !( SDMMCx->STA & SDMMC_STA_DATAEND ) ) {
    // Use the 'half-empty' flag to send new data as long as
    // at least 8 words in the queue are empty.
    if ( SDMMCx->STA & SDMMC_STA_TXFIFOHE ) {
      SDMMCx->FIFO = buf[ buf_ind ];
      ++buf_ind;
    }
  }

  // Poll CMD13 until the state is back to 'transfer'.
  // TODO: Technically, it is okay to issue CMD7 and do other things
  // while the SD card performs its data writes. But for testing,
  // it seems safest to perform blocking writes and wait for each
  // one to finish before proceeding. I've already bricked one card...
  resp = 0x00000000;
  while ( !( resp & SDMMC_READY_FOR_DATA ) ) {
    sdmmc_cmd_write( SDMMCx,
                     SDMMC_CMD_GET_STAT,
                     ( ( uint32_t )card_addr ) << 16,
                     SDMMC_RESPONSE_SHORT );
    sdmmc_cmd_read( SDMMCx, SDMMC_RESPONSE_SHORT,
                    SDMMC_CHECK_CRC, &resp );
    sdmmc_cmd_done( SDMMCx );
  }

  // Done writing; CMD7 to de-select the card.
  sdmmc_cmd_write( SDMMCx,
                   SDMMC_CMD_SEL_DESEL,
                   0x00000000,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( SDMMCx );
}

/** Write N blocks of data to an address on the SD/MMC card. */
void sdmmc_write_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         uint32_t *buf,
                         int blen ) {
  // TODO: CMD23 / CMD 24 to write multiple blocks.
}

/**
 * Erase a block range on the SD/MMC card, given start ID and length.
 */
void sdmmc_erase_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         blockno_t num_blocks ) {
  // TODO
}

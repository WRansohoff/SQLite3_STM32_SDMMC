#include "block_sd_foss.h"

SDCard card = { 0x00000000, 0x0000, 0x0000, 0x00, 0x00 };
SDMMC_TypeDef *sdmmc = SDMMC1;

/**
 * Perform one-time peripheral initialization for the block buffer.
 */
int block_init() {
  // Currently, the `port` files initialize the interface and I/O
  // pins, so this method just sets up the SD card.
  uint8_t cmd_resp_ind = 0;
  uint32_t cmd_resp[ 4 ] = { 0, 0, 0, 0 };

  // Send CMD0 - no response data is expected, but if the card
  // doesn't respond at all, that's a problem.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_GO_IDLE,
                   0x00000000,
                   SDMMC_RESPONSE_NONE );
  cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
  sdmmc_cmd_done( sdmmc );
  if ( cmd_resp_ind == 0xFF ) {
    // Either a timeout or CRC failure occurred. Mark the card
    // as being in an error state, and return -1.
    card.type = SD_CARD_ERROR;
    card.error = SD_ERR_NOT_PRESENT;
    return -1;
  }

  // Send CMD8 - this is the next step in the init flowchart, to
  // check whether the card supports SD spec >=V2.00 or not.
  // Again, this is simple: any non-error response means that V2+
  // is supported. Bits 0-7 of the argument are a 'check pattern'
  // that the card will echo back. It sounds like it can be anything,
  // but the SD card spec recommends a value of '0b10101010'. Bits
  // 8-11 represent the voltage range we expect to use, but the only
  // valid values appear to be '0b0001' for 2.7-3.6V and '0b0010'
  // for 'low-voltage range'. So, 0x01AA is a typical pattern for
  // using an SD card with 3.3V systems.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_IF_COND,
                   0x000001AA,
                   SDMMC_RESPONSE_SHORT );
  // Receive the response, if any.
  cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT, cmd_resp );
  sdmmc_cmd_done( sdmmc );
  // Check for valid response types.
  // TODO: macro definitions for constants.
  if ( cmd_resp_ind == 5 ) {
    // The card rejected the command; this means it is SDC V1
    // (Or, according to elmchan, MMC V3)
    card.type = SD_CARD_SC;
  }
  else if ( cmd_resp_ind == 1 ) {
    // The card responded, so it is SDC V2+
    // Check the response to make sure that it supports
    // the desired voltage range. Bits 0-7 should be
    // the 'check pattern' that we sent (0xAA) and bits 8-11
    // should be 0b0001 if the card accepts a 2.7-3.6V range.
    if ( ( cmd_resp[ 0 ] & 0x000001FF ) != 0x000001AA ) {
      card.type = SD_CARD_ERROR;
      return -1;
    }
    card.type = SD_CARD_HC;
  }
  else {
    // No response; return -1.
    card.type = SD_CARD_ERROR;
    return -1;
  }

  // App CMD 41 to initialize the card.
  // Send ACMD41 with the 'HCS' bit (#30) set if the card is SD V2+.
  // You can also set the 'S18R' bit (#24) to check if the card
  // supports 1.8V signalling levels, but that is not done here.
  uint32_t acmd_arg =
    ( card.type == SD_CARD_HC ) ? 0x40000000 : 0x00000000;
  // Keep calling ACMD41 until the 'done powering up' bit is set.
  // If I read the OCR register right, that bit prevents the
  // 'standard / high capacity' flag from being set when low.
  cmd_resp[ 0 ] = 0x00000000;
  while ( !( cmd_resp[ 0 ] & 0x80000000 ) ) {
    // Send CMD55 to indicate that an app command will follow.
    sdmmc_cmd_write( sdmmc,
                     SDMMC_CMD_APP,
                     0x00000000,
                     SDMMC_RESPONSE_NONE );
    sdmmc_cmd_done( sdmmc );
    // Send APPCMD41 with HC argument.
    sdmmc_cmd_write( sdmmc,
                     SDMMC_APP_HCS_OPCOND,
                     acmd_arg,
                     SDMMC_RESPONSE_SHORT );
    cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
    sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT, cmd_resp );
    sdmmc_cmd_done( sdmmc );
  }
  // Once the above loop exits, the first response element holds
  // the card's OCR register and it is done powering up. (I hope.)
  // Check the 'high-capacity' flag and set card type accordingly.
  if ( card.type == SD_CARD_HC ) {
    // The card type was previously set to 'high-capacity' only
    // if it supported SD spec V2+. But we still need to check
    // that the card identifies itself as high-capacity.
    if ( !( cmd_resp[ 0 ] & 0x40000000 ) ) {
      // The card's response did not set the 'card capacity status'
      // bit, so mark it as a standard-capacity card.
      card.type = SD_CARD_SC;
    }
  }

  // CMD3 to get an address that the card will respond to.
  // TODO: Error-checking?
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_PUB_RCA,
                   0x00000000,
                   SDMMC_RESPONSE_SHORT );
  cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT, cmd_resp );
  sdmmc_cmd_done( sdmmc );
  // Bits 0-15 are status bits, and bits 16-31 are the new address.
  card.addr = cmd_resp[ 0 ] >> 16;

  // CMD9 to get the card's storage capacity from its
  // 'card-specific data' field.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_GET_CSD,
                   ( ( uint32_t )card.addr ) << 16,
                   SDMMC_RESPONSE_LONG );
  cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_LONG, cmd_resp );
  sdmmc_cmd_done( sdmmc );
  // Okay, there are a lot of fields in this response. Bit 127
  // indicates whether the card is standard- or high-capacity.
  // We already know that at this point, but SC cards have different
  // responses from HC cards. For SC cards, the card's storage
  // capacity in bytes = (SIZE_C+1)*2^(C_SIZE_MULT+2)*2^(READ_BL_LEN)
  // where SIZE_C is bits 62-73, C_SIZE_MULT is bits 47-49, and
  // READ_BL_LEN is bits 80-83. SDHC / SDXC cards are easier:
  // capacity (Kbytes) = (SIZE_C+1)*512, and SIZE_C is bits 48-69.
  if ( card.type == SD_CARD_HC ) {
    // High-capacity card. Retrieve `SIZE_C` from bits 48-69.
    uint32_t size_c = ( ( cmd_resp[ 1 ] >> 16 ) & 0x0000FFFF ) |
                      ( ( cmd_resp[ 2 ] & 0x0000003F ) << 16 );
    // Total capacity is the value * 512KB. Divide by 512 to get the
    // number of blocks, so `card.blocks` = `size_c` * 1024.
    card.blocks = size_c * 1024;
  }
  else {
    // Standard-capacity card.
    // Get `size_c` from bits 62-73.
    uint32_t size_c = cmd_resp[ 3 ] & 0x00003FFF;
    // Get `c_size_mult` from bits 47-49.
    uint32_t c_size_mult = ( cmd_resp[ 2 ] & 0x00038000 ) >> 15;
    // Get `read_bl_len` from bits 80-83.
    uint32_t read_bl_len = ( cmd_resp[ 3 ] & 0x000F0000 ) >> 16;
    // Calculate number of 512-byte blocks. (See above comment block)
    // Note: ( X * 2^Y ) = X << Y, which simplifies this a bit.
    card.blocks = ( ( ( size_c + 1 ) <<
                  ( c_size_mult + 2 ) ) <<
                  read_bl_len ) / 512;

    // CMD16 to set block size to 512 bytes. High-capacity cards
    // do not need this command, since their block size is fixed.
    // TODO: Check response for errors.
    sdmmc_cmd_write( sdmmc,
                     SDMMC_CMD_SET_BLOCKLEN,
                     512,
                     SDMMC_RESPONSE_SHORT );
    cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
    sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT, cmd_resp );
    sdmmc_cmd_done( sdmmc );
  }

  // CMD7 to select the card.
  // TODO: This returns an 'R1b' response, which means that
  // the `DAT0` wire is held low until the card is not busy.
  // So...should I wait for the line to go high here by checking
  // the GPIO IDR register, or is the STM32 SDMMC peripheral smart
  // enough to not set its status flags until the busy signal clears?
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card.addr ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( sdmmc );

  // App CMD6 to set the data bus width to 4. Apparently
  // this can only be done when the card is in 'transmission mode',
  // which it enters when selected by CMD7. But I'm not exactly
  // sure if this needs to be set for each transaction, or if it will
  // stick after being set in this method. I'm hoping for the latter.
  // (CMD55 needs to precede application commands)
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_APP,
                   0x00000000,
                   SDMMC_RESPONSE_NONE );
  sdmmc_cmd_done( sdmmc );
  // 0x00 = 1-bit bus width, 0x02 = 4-bit bus width.
  // TODO: Check response for errors.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_APP_SET_BUSW,
                   0x00000002,
                   SDMMC_RESPONSE_SHORT );
  cmd_resp_ind = sdmmc_cmd_read_type( sdmmc );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT, cmd_resp );
  sdmmc_cmd_done( sdmmc );

  // CMD7 to de-select the card.
  // Sending any address except the one that the card previously
  // published will de-select it. I'm not sure if there are any
  // guaranteed invalid addresses, so for now, I'll just
  // XOR the card's address to change all of its bits.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_SEL_DESEL,
                   ( ( uint32_t )card.addr ^ 0x0000FFFF ) << 16,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_done( sdmmc );

  // Done; return 0 to indicate success.
  return 0;
}

/**
 * Shut down the SD card interface.
 * TODO: Currently unused, it looks unnecessary for the applications
 * that I have in mind.
 */
int block_halt() { return 0; }

/** Read a block from the current SD card into a given buffer. */
int block_read( blockno_t block, void *buf ) {
  sdmmc_block_read( sdmmc, block, buf, 1 );
  return 0;
}

/** Write a block of data to the current SD card from a buffer. */
int block_write( blockno_t block, void *buf ) {
  sdmmc_block_write( sdmmc, block, buf, 1 );
  return 0;
}

/**
 * Get the storage capacity of the currently-connected SD card. This
 * returns the number of 512-byte blocks, not the number of bytes.
 */
blockno_t block_get_volume_size() { return card.blocks; }

/** Get the size of a block in the filesystem. */
int block_get_block_size() { return BLOCK_SIZE; }

/**
 * Check whether the currently-connected SD card is read-only.
 * TODO: Currently, the `card` struct will always say that
 * the card can be written to. It looks like high-capacity cards
 * don't support many of the write-protection features anyways.
 */
int block_get_device_read_only() { return card.read_only; }

/** Get the current error status of the connected SD card. */
int block_get_error() { return card.error; }

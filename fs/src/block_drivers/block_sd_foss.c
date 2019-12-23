#include "block_sd_foss.h"

SDCard card = { 0x00000000, 0x0000, 0x0000, 0x00, 0x00 };
SDMMC_TypeDef *sdmmc = SDMMC1;

/**
 * Perform one-time peripheral initialization for the block buffer.
 */
int block_init() {
  // Currently, the `port` files initialize the interface and I/O
  // pins, so this method just sets up the SD card.
  uint32_t cmd_resp[ 4 ] = { 0, 0, 0, 0 };

  // Send CMD0 - no response data is expected, but if the card
  // doesn't respond at all, that's a problem.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_GO_IDLE,
                   0x00000000,
                   SDMMC_RESPONSE_NONE );
  sdmmc_cmd_done( sdmmc );

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
  if ( sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT,
                       SDMMC_CHECK_CRC, cmd_resp ) == -1 ) {
    // The card rejected the command; this means it is SDC V1
    // (Or, according to elmchan, MMC V3)
    card.type = SD_CARD_SC;
  }
  else {
    // The card responded, so it is SDC V2+
    // Check the response to make sure that it supports
    // the desired voltage range. Bits 0-7 should be
    // the 'check pattern' that we sent (0xAA) and bits 8-11
    // should be 0b0001 if the card accepts a 2.7-3.6V range.
    if ( ( cmd_resp[ 0 ] & 0x000001FF ) != 0x000001AA ) {
      card.type = SD_CARD_ERROR;
      sdmmc_cmd_done( sdmmc );
      return -1;
    }
    card.type = SD_CARD_HC;
  }
  sdmmc_cmd_done( sdmmc );

  // App CMD 41 to initialize the card.
  // Send ACMD41 with the 'HCS' bit (#30) set if the card is SD V2+.
  // The 'busy' bit (#31) also seems to be required, otherwise the
  // card will always respond that it is busy. Weird.
  // You can also set the 'S18R' bit (#24) to check if the card
  // supports 1.8V signalling levels, but that is not done here.
  uint32_t acmd_arg =
    ( card.type == SD_CARD_HC ) ? 0xC0100000 : 0x80000000;
    //( card.type == SD_CARD_HC ) ? 0xC0FF8000 : 0x80FF8000;
  // Keep calling ACMD41 until the 'done powering up' bit is set.
  // If I read the OCR register right, that bit prevents the
  // 'standard / high capacity' flag from being set when low.
  cmd_resp[ 0 ] = 0x00000000;
  while ( !( cmd_resp[ 0 ] & 0x80000000 ) ) {
    // Send CMD55 to indicate that an app command will follow.
    sdmmc_cmd_write( sdmmc,
                     SDMMC_CMD_APP,
                     0x00000000,
                     SDMMC_RESPONSE_SHORT );
    sdmmc_cmd_done( sdmmc );
    // Send APPCMD41 with HC argument.
    sdmmc_cmd_write( sdmmc,
                     SDMMC_APP_HCS_OPCOND,
                     acmd_arg,
                     SDMMC_RESPONSE_SHORT );
    sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT,
                    SDMMC_NO_CRC, cmd_resp );
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

  // CMD2 to put the card into 'identification mode'.
  // The card should respond with the contents of its CID register.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_PUB_CID,
                   0x00000000,
                   SDMMC_RESPONSE_LONG );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_LONG,
                  SDMMC_CHECK_CRC, cmd_resp );
  sdmmc_cmd_done( sdmmc );

  // CMD3 to get an address that the card will respond to.
  // TODO: Error-checking?
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_PUB_RCA,
                   0x00000000,
                   SDMMC_RESPONSE_SHORT );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_SHORT,
                  SDMMC_CHECK_CRC, cmd_resp );
  sdmmc_cmd_done( sdmmc );
  // Bits 0-15 are status bits, and bits 16-31 are the new address.
  card.addr = cmd_resp[ 0 ] >> 16;

  // CMD9 to get the 'card-specific data' registers.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_GET_CSD,
                   ( ( uint32_t )card.addr ) << 16,
                   SDMMC_RESPONSE_LONG );
  sdmmc_cmd_read( sdmmc, SDMMC_RESPONSE_LONG,
                  SDMMC_CHECK_CRC, cmd_resp );
  sdmmc_cmd_done( sdmmc );

  // Find the card's storage capacity from its CSD registers.
  if ( card.type == SD_CARD_HC ) {
    // High-capacity card: get the card's capacity in 512B blocks.
    card.blocks = sdmmc_get_volume_size( sdmmc, SDMMC_HC, cmd_resp );
  }
  else {
    // Standard-capacity card.
    card.blocks = sdmmc_get_volume_size( sdmmc, SDMMC_SC, cmd_resp );
    // Set block size to 512 bytes. High-capacity cards
    // do not need this command, since their block size is fixed.
    sdmmc_set_block_len( sdmmc, 512 );
  }

  // Set the bus width to 4 bits.
  //sdmmc_set_bus_width( sdmmc, card.addr, SDMMC_BUS_WIDTH_4b );

  // Done; return 0 to indicate success.
  return 0;
}

/**
 * Shut down the SD card interface.
 * TODO: Error checking.
 */
int block_halt() {
  // Send CMD15 to put the card into an inactive state.
  sdmmc_cmd_write( sdmmc,
                   SDMMC_CMD_GO_IDLE,
                   ( ( uint32_t )card.addr ) << 16,
                   SDMMC_RESPONSE_NONE );
  sdmmc_cmd_done( sdmmc );
  // Done; return 0 to indicate success.
  return 0;
}

/** Read a block from the current SD card into a given buffer. */
int block_read( blockno_t block, void *buf ) {
  sdmmc_read_block( sdmmc,
                    ( card.type == SD_CARD_HC ) ? SDMMC_HC : SDMMC_SC,
                    card.addr,
                    block,
                    ( uint32_t* )buf );
  return 0;
}

/** Write a block of data to the current SD card from a buffer. */
int block_write( blockno_t block, void *buf ) {
  sdmmc_write_block( sdmmc,
                     ( card.type == SD_CARD_HC ) ? SDMMC_HC : SDMMC_SC,
                     card.addr,
                     block,
                     ( uint32_t* )buf );
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

#include "block_sd_foss.h"

SDCard card = {0, 0, 0, 0};
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
    card.card_type = SD_CARD_ERROR;
    card.error = SD_ERR_NOT_PRESENT;
    return -1;
  }

  // Send CMD8 - this is the next step in the init flowchart, to
  // check whether the card supports SD spec >=V2.00 or not.
  // Again, this is simple: any response means that V2+ is supported.
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
    card.card_type = SD_CARD_SC;
  }
  else if ( cmd_resp_ind == 1 ) {
    // The card responded, so it is SDC V2+
    // Check the response to make sure that it supports
    // the desired voltage range.
    if ( ( cmd_resp[ 0 ] & 0x000001FF ) == 0x000001AA ) {
      card.card_type = SD_CARD_ERROR;
      return -1;
    }
    card.card_type = SD_CARD_HC;
  }
  else {
    // No response; return -1.
    card.card_type = SD_CARD_ERROR;
    return -1;
  }

  // TODO: App command 41 to initialize the card.

  // TODO: CMD9 to get 'card-specific data' field:
  // the card's storage capacity.

  // TODO: CMD16 to ensure the block length is set to 512B.
  // SDHC / SDXC cards will always use 512B internally, and it
  // is a good value for a FAT filesystem. But some cards have
  // a default of 1024B, according to elmchan.

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

/** Get the storage capacity of the currently-connected SD card. */
blockno_t block_get_volume_size() {
  // Return ( # of bytes ) / ( block size )
  return sdmmc_get_volume_size( sdmmc ) / block_get_block_size();
}

/** Get the size of a block in the filesystem. */
int block_get_block_size() { return BLOCK_SIZE; }

/**
 * Check whether the currently-connected SD card is read-only.
 * TODO: Currently, the `card` struct will always say that
 * the card can be written to.
 */
int block_get_device_read_only() { return card.read_only; }

/** Get the current error status of the connected SD card. */
int block_get_error() { return card.error; }

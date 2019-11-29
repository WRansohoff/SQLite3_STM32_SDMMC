#include "block_sd_foss.h"

SDCard card = {0, 0, 0, 0};
SDMMC_TypeDef *sdmmc = SDMMC1;

/**
 * Send a command to the current SD card.
 * The 'chksm' argument is accepted for library compatibility,
 * but it is ignored because the STM32 SD/MMC peripheral can
 * automatically perform CRC checks for each transmission.
 * TODO: I think 'application commands' need to be handled
 * differently. Also, CMD12 might require special handling?
 */
uint16_t sd_command( uint8_t code, uint32_t data, uint8_t chksm ) {
  // Write the command and data. Use index #0 for now as a
  // stopgap; I'm not really sure how these response index
  // registers work yet.
  uint8_t cmd_index = 0;
  sdmmc_cmd_write( sdmmc, ( uint32_t )code, data, cmd_index );
  // Block on receiving the command response, then return it.
  return sdmmc_cmd_read( sdmmc, cmd_index );
}

/** Perform a software reset on the current SD card. */
int sd_card_reset() {
  // TODO
  return -1;
}

/**
 * Perform one-time peripheral initialization for the block buffer.
 */
int block_init() {
  // TODO: Currently, the `port` files initialize the interface
  // and I/O pins, so this method has nothing to do but reset
  // the SD card.
  return sd_card_reset();
}

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

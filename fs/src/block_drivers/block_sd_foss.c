#include "block_sd_foss.h"

SDCard card = {0, 0, 0, 0};
SDMMC_TypeDef *sdmmc = SDMMC1;

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

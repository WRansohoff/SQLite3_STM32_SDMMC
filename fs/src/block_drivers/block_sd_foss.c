#include "block_sd_foss.h"

SDCard card = {0, 0, 0, 0};

/** Send a command to the current SD card. */
uint16_t sd_command( uint8_t code, uint32_t data, uint8_t chksm ) {
  // TODO
  return 0xFF;
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
  // TODO
  return -1;
}

/** Read a block from the current SD card into a given buffer. */
int block_read( blockno_t block, void *buf ) {
  // TODO
  return -1;
}

/** Write a block of data to the current SD card from a buffer. */
int block_write( blockno_t block, void *buf ) {
  // TODO
  return -1;
}

/** Get the storage capacity of the currently-connected SD card. */
blockno_t block_get_volume_size() {
  // TODO
  return 0;
}

/** Get the size of a block in the filesystem. */
int block_get_block_size() { return BLOCK_SIZE; }

/** Check whether the currently-connected SD card is read-only. */
int block_get_device_read_only() {
  // TODO
  return 0;
}

/** Get the current error status of the connected SD card. */
int block_get_error() { return card.error; }

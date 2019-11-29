/*
 * Minimal SD/MMC peripheral interface methods.
 * TODO: Error checking is not implemented.
 */
#ifndef __VVC_SDMMC
#define __VVC_SDMMC

// Standard library includes.
#include <stdint.h>

// 'Gristle' Fat filesystem include, for `blockno_t` typedef.
#include "block.h"

// Device header file.
#include "stm32l4xx.h"

// Macro definitions. TODO: Enum?
#define SDMMC_RESPONSE_NONE  ( 0 )
#define SDMMC_RESPONSE_SHORT ( 1 )
#define SDMMC_RESPONSE_NONE2 ( 2 )
#define SDMMC_RESPONSE_LONG  ( 3 )

// Setup an SD/MMC peripheral for simple 'polling mode'.
// This is slow; no interrupts, hardware flow control, or DMA.
void sdmmc_setup( SDMMC_TypeDef *SDMMCx );

// Send a command to the SD/MMC card.
void sdmmc_cmd_write( SDMMC_TypeDef *SDMMCx,
                      uint32_t cmd,
                      uint32_t dat,
                      int resp_type );
// Read the `index` of a card's response to a command. This is similar
// to the card command ID / categories, but for card responses.
uint8_t sdmmc_cmd_read_type( SDMMC_TypeDef *SDMMCx );
// Read the contents of the SDMMC peripheral's 'command response'
// registers into a buffer. This receives the response from a command.
// If `type` is `SDMMC_RESPONSE_SHORT`, only the first word is read.
// If `type` is `SDMMC_RESPONSE_LONG`, all 4 words are read.
int sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx, int type, void *buf );
// Acknowledge that a command response was received. This does not
// send any data to the SD card; it just resets internal state.
void sdmmc_cmd_done( SDMMC_TypeDef *SDMMCx );
// Tell the connected SD card to use a specified block size.
// I think that the block size is defined in bytes.
uint32_t sdmmc_set_block_len( SDMMC_TypeDef *SDMMCx, uint32_t bsize );
// Tell the connected SD card to use a specified bus width.
uint32_t sdmmc_set_bus_width( SDMMC_TypeDef *SDMMCx, uint32_t width );
// Figure out how much storage capacity the SD card (claims to) have.
// TODO: I think that a 32-bit int might be too small for very
// high-capacity cards, but the higher addresses wouldn't be
// supported by this interface anyways.
uint32_t sdmmc_get_volume_size( SDMMC_TypeDef *SDMMCx );

// Read N blocks of data from an address on the SD/MMC card.
void sdmmc_block_read( SDMMC_TypeDef *SDMMCx,
                       blockno_t start_block,
                       void *buf,
                       int blen );
// Write N blocks of data to an address on the SD/MMC card.
void sdmmc_block_write( SDMMC_TypeDef *SDMMCx,
                        blockno_t start_block,
                        void *buf,
                        int blen );
// Erase a block range on the SD/MMC card, given start ID and length.
void sdmmc_erase_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         blockno_t num_blocks );

#endif

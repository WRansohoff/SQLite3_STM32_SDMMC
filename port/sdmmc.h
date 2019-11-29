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

// Setup an SD/MMC peripheral for simple 'polling mode'.
// This is slow; no interrupts, hardware flow control, or DMA.
void sdmmc_setup( SDMMC_TypeDef *SDMMCx );

// Send a command to the SD/MMC card.
void sdmmc_cmd_write( SDMMC_TypeDef *SDMMCx,
                      uint32_t cmd,
                      uint32_t index );
// Read the index of a card's response to a command.
uint8_t sdmmc_cmd_read_index( SDMMC_TypeDef *SDMMCx );
// Read the contents of one of the SDMMC peripheral's 'command
// response registers'. This can receive the response from a command.
uint32_t sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx );
// Tell the connected SD card to use a specified block size.
// I think that the block size is defined in bytes.
uint32_t sdmmc_set_block_len( SDMMC_TypeDef *SDMMC, uint32_t bsize );
// Tell the connected SD card to use a specified bus width.
uint32_t sdmmc_set_bus_width( SDMMC_TypeDef *SDMMC, uint32_t width );

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

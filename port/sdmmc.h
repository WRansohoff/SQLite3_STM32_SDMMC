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

// HAL includes.
#include "port/tim.h"

// Macro definitions. TODO: Enums?
// SD card response types. These correspond to the STM32's expected
// values in the `WAITRESP` field of the `CMD` register.
#define SDMMC_RESPONSE_NONE  ( 0 )
#define SDMMC_RESPONSE_SHORT ( 1 )
#define SDMMC_RESPONSE_NONE2 ( 2 )
#define SDMMC_RESPONSE_LONG  ( 3 )
// Bus width definitions.
#define SDMMC_BUS_WIDTH_1b   ( 0x00000000 )
#define SDMMC_BUS_WIDTH_4b   ( 0x00000002 )
// Card type definition.
#define SDMMC_SC             ( 0 )
#define SDMMC_HC             ( 1 )
// Whether to expect a valid CRC in the card's response or not.
#define SDMMC_CHECK_CRC      ( 0 )
#define SDMMC_NO_CRC         ( 1 )
// SD card state machine values, returned in the status register.
#define SDMMC_STATE_IDLE     ( 0 )
#define SDMMC_STATE_READY    ( 1 )
#define SDMMC_STATE_IDENT    ( 2 )
#define SDMMC_STATE_STBY     ( 3 )
#define SDMMC_STATE_TRAN     ( 4 )
#define SDMMC_STATE_DATA     ( 5 )
#define SDMMC_STATE_RCV      ( 6 )
#define SDMMC_STATE_PRG      ( 7 )
#define SDMMC_STATE_DIS      ( 8 )
#define SDMMC_STATE_IO       ( 15 )
// SD card status register bit flags.
#define SDMMC_READY_FOR_DATA ( 0x00000100 )

// SD card command index values. Because referring to them as
// `CMD0`, `CMD1`, etc is confusing and not very helpful.
// This is not an exhaustive list of valid commands.
// 'Go Idle' command - this resets the SD card.
#define SDMMC_CMD_GO_IDLE        ( 0 )
// 'Send Operating Conditions' command - this sends information
// about the host's capacity support, and starts the card's
// initialization process. But I don't see this command in most
// of the SD card initialization flowcharts that I've seen.
#define SDMMC_CMD_SEND_OP_COND   ( 1 )
// 'Respond With CID Register' command - this asks any cards on
// the bus to respond with the contents of their CID register.
// ('CID' = 'Card IDentifier')
#define SDMMC_CMD_PUB_CID        ( 2 )
// Ask the card to publish a new 'Relative Card Address' (RCA)
// which it will respond to. I think this is used with CMD7 to select
// an active card when there are multiple ones on the same bus.
#define SDMMC_CMD_PUB_RCA        ( 3 )
// Set the 'Driver State Register' (DSR) on the SD card. TODO: I'm not
// sure what this does; maybe it lets you store arbitrary state?
#define SDMMC_CMD_SET_DSR        ( 4 )
// Send host capacity information (like CMD1?), and ask the card
// to respond with its current operating conditions.
#define SDMMC_CMD_SET_HCS        ( 5 )
// Check the the current 'switchable function', or switch the card's
// current function. This looks like a catch-all command that is used
// to change the card's behavior, and it looks like it responds with
// a full block of data instead of the usual response registers.
#define SDMMC_CMD_SWITCH         ( 6 )
// Select or de-select an SD card using an RCA. From a card's
// perspective, it is selected if the RCA matches its address
// and deselected otherwise.
#define SDMMC_CMD_SEL_DESEL      ( 7 )
// Send the operating conditions which the host expects to the card.
// The card should respond saying whether it supports the given
// voltage range, etc.
#define SDMMC_CMD_IF_COND        ( 8 )
// Ask a specific card to send its CSD register if the RCA matches.
// (CSD = 'Card-Specific Data')
#define SDMMC_CMD_GET_CSD        ( 9 )
// Ask a specific card to send its CID register if the RCA matches.
// (CID = 'Card IDentifier')
#define SDMMC_CMD_GET_CID        ( 10 )
// Switch cards to 1.8V bus voltage for I/O signals.
#define SDMMC_CMD_LOWVOLT        ( 11 )
// Ask the card to stop an ongoing transmission.
#define SDMMC_CMD_STOP_TRANS     ( 12 )
// Ask the addressed card to send its current status register.
#define SDMMC_CMD_GET_STAT       ( 13 )
// (It looks like CMD14 is reserved)
// Put the addressed card into an inactive state.
#define SDMMC_CMD_GO_INACTIVE    ( 15 )
// Set the block size on the SD card. Default is 512 Bytes, and
// it sounds like SDHC / SDXC cards will refuse to switch from
// 512-Byte blocks internally.
#define SDMMC_CMD_SET_BLOCKLEN   ( 16 )
// Read a block of data. If the card is an SDHC / SDXC card,
// 512 Bytes will be read regardless of the block length setting.
#define SDMMC_CMD_READ_BLOCK     ( 17 )
// Read blocks of data starting at a given address, until a
// 'stop transmission' command is received.
// CMD23 might also limit the number of blocks to read?
#define SDMMC_CMD_READ_BLOCKS    ( 18 )
// (CMD19/20 not used. CMD21/22 reserved.)
// Set 'block count' value which is used by CMD18 and CMD25.
#define SDMMC_CMD_SET_NUM_BLOCKS ( 23 )
// Write a block of data. If the card is an SDHC / SDXC card.
// 512 Bytes must be written regardless of the block length setting.
#define SDMMC_CMD_WRITE_BLOCK    ( 24 )
// Write blocks of data starting at a given address, until a
// 'stop transmission' command is received.
// CMD23 might also limit the number of blocks to write?
#define SDMMC_CMD_WRITE_BLOCKS   ( 25 )
// Set writable bits in the 'Card-Specific Data' register.
#define SDMMC_CMD_SET_CSD        ( 27 )
// Enable software write protection. SDHC / SDXC cards do not
// implement this functionality or this command.
#define SDMMC_CMD_SET_WP         ( 28 )
// Disable software write protection. SDHC / SDXC cards do not
// implement this functionality or this command.
#define SDMMC_CMD_CLR_WP         ( 29 )
// Ask the card if write protection is active for a given block.
// SDHC / SDXC cards do not implement this functionality or command.
#define SDMMC_CMD_GET_WP         ( 30 )
// Set the starting address for an 'erase' operation.
// Despite the name, this does not start an erase operation.
#define SDMMC_CMD_ERASE_START    ( 32 )
// Set the ending address for an 'erase' operation.
// Despite the name, this does not end an ongoing erase operation.
#define SDMMC_CMD_ERASE_END      ( 33 )
// Erase data between the addresses set by CMD32 and CMD33.
#define SDMMC_CMD_ERASE          ( 38 )
// Command indicating that an application-specific command will follow.
#define SDMMC_CMD_APP            ( 55 )
// Command to read/write blocks of data for use with application-
// specific commands. The LSbit should be 1 for reading, 0 for writing.
#define SDMMC_CMD_APP_DAT        ( 56 )
// App command to set the data bus width.
#define SDMMC_APP_SET_BUSW       ( 6 )
// App command to get the 'status field' from an SD card.
#define SDMMC_APP_GET_STAT       ( 13 )
// App command to send host capacity information and request a card's
// operating conditions. This looks like it combines functionality
// from CMD1/5/etc, and most flowcharts that I see use this
// application command instead of the generic commands.
#define SDMMC_APP_HCS_OPCOND     ( 41 )
// App command to ask the card to mark itself as not present
// (or present) by connecting (or disconnecting) a pulling
// resistor on its `CD` line.
#define SDMMC_APP_SET_CD         ( 42 )
// App command to read SD card configuration register.
#define SDMMC_APP_GET_SCR        ( 51 )

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
int sdmmc_cmd_read( SDMMC_TypeDef *SDMMCx,
                    int type,
                    int ignore_crc,
                    void *buf );
// Acknowledge that a command response was received. This does not
// send any data to the SD card; it just resets internal state.
void sdmmc_cmd_done( SDMMC_TypeDef *SDMMCx );
// Tell the connected SD card to use a specified block size.
// I think that the block size is defined in bytes.
void sdmmc_set_block_len( SDMMC_TypeDef *SDMMCx,
                          uint32_t bsize );
// Tell the connected SD card to use a specified bus width.
void sdmmc_set_bus_width( SDMMC_TypeDef *SDMMCx,
                          uint16_t card_addr,
                          uint32_t width );
// Figure out how much storage capacity the SD card claims
// to have, in 512-byte blocks. (NOT in bytes)
uint32_t sdmmc_get_volume_size( SDMMC_TypeDef *SDMMCx,
                                uint32_t card_type,
                                uint32_t *csd_regs );
// Check if the card is busy or not.
int sdmmc_is_card_busy( SDMMC_TypeDef *SDMMCx, uint16_t card_addr );

// Read one block of data from an address on the SD/MMC card.
void sdmmc_read_block( SDMMC_TypeDef *SDMMCx,
                       uint32_t card_type,
                       uint16_t card_addr,
                       blockno_t start_block,
                       uint32_t *buf );
// Read N blocks of data from an address on the SD/MMC card.
void sdmmc_read_blocks( SDMMC_TypeDef *SDMMCx,
                        blockno_t start_block,
                        uint32_t *buf,
                        int blen );
// Write one block of data to an address on the SD/MMC card.
void sdmmc_write_block( SDMMC_TypeDef *SDMMCx,
                        uint32_t card_type,
                        uint16_t card_addr,
                        blockno_t start_block,
                        uint32_t *buf );
// Write N blocks of data to an address on the SD/MMC card.
void sdmmc_write_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         uint32_t *buf,
                         int blen );
// Erase a block range on the SD/MMC card, given start ID and length.
void sdmmc_erase_blocks( SDMMC_TypeDef *SDMMCx,
                         blockno_t start_block,
                         blockno_t num_blocks );

#endif

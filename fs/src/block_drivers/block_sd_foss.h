/**
 * 'Block driver' port for the 'Gristle' FAT filesystem.
 * Designed for an STM32L4 SD/MMC interface; the default one
 * uses SPI, and also `libopencm3` which has a viral license.
 * Parts of these files are still derived from the BSD-licensed
 * block driver code in Gristle, though.
 *
 * MIT License
 * Copyright (c) 2019 WRansohoff
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef VVC_STM32_SDMMC_BLOCKDRIVER
#define VVC_STM32_SDMMC_BLOCKDRIVER

#include <stdint.h>
#include "block.h"
#include "port/sdmmc.h"
#include "port/tim.h"

/* Constants used to define card types */
#define SD_CARD_NONE  0    /* No SD card found */
#define SD_CARD_MMC   1    /* MMC card */
#define SD_CARD_SC    2    /* Standard capacity SD card (up to 2GB)*/
#define SD_CARD_HC    3    /* High capacity SD card (4GB to 32GB)*/
#define SD_CARD_XC    4    /* eXtended Capacity SD card (up to 2TB  - Untested may work if FAT32) */
#define SD_CARD_ERROR 99   /* An error occured during setup */

/* Error status codes returned in the SD info struct */
#define SD_ERR_NO_PART      1
#define SD_ERR_NOT_PRESENT  2

/*
 * SD card info struct
 * Note: the `blocks` value represents the card's capacity in
 * 512-byte blocks. To get the capacity in bytes, multiply by 512.
 */
typedef struct {
  uint32_t  blocks;
  uint16_t  type;
  uint16_t  addr;
  uint8_t   error;
  uint8_t   read_only;
} SDCard;

#endif

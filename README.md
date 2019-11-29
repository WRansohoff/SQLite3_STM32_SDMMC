# ARM Cortex-M `sqlite3` Amalgamation Build

This repository reflects an attempt to build the `sqlite3` database library for ARM Cortex-M microcontrollers.

It is not currently functional, because I have not implemented the required methods under `fs/src/block_drivers/block_sd_foss.c` and `sys/syscalls.c`. But it does compile with the stubbed methods, and it looks like it is about 320KB with the `-Os` flag. So it might be closer to 350-400KB once the necessary helper methods are implemented, and that's pretty big for a microcontroller.

Still, the `STM32L4` line of MCUs that I plan to target has a number of chips with 1-2MB of on-chip Flash memory, so I'm hoping that I'll at least be able to test it using the chip's SD/MMC peripheral with a microSD card for file storage.

# Building

This build depends on the `Gristle` FAT filesystem library, but a snapshot is included in this repository, so you should just be able to run `make`.

Anyways, here are links to the `Gristle` filesystem library and the very cool `OggBox` project which is was written for:

https://github.com/hairymnstr/gristle/

https://github.com/hairymnstr/oggbox

And just to re-iterate, _this sqlite3 build is a non-functional work-in-progress._ Sorry.

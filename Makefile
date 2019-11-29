TARGET = sqlite
# Toolchain definitions (ARM bare metal)
TOOLCHAIN = /usr
CC = $(TOOLCHAIN)/bin/arm-none-eabi-gcc
AR = $(TOOLCHAIN)/bin/arm-none-eabi-ar
LD = $(TOOLCHAIN)/bin/arm-none-eabi-ld
OC = $(TOOLCHAIN)/bin/arm-none-eabi-objcopy
OD = $(TOOLCHAIN)/bin/arm-none-eabi-objdump
OS = $(TOOLCHAIN)/bin/arm-none-eabi-size

# C compilation flags.
CFLAGS = -mthumb -mcpu=cortex-m4 -mhard-float -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g -Wall -Os -fno-strict-aliasing -fmessage-length=0 --specs=nosys.specs
# (Device-specific)
CFLAGS += -DSTM32L496xx
# C source files.
C_SRC  = sys/syscalls.c
C_SRC += port/rcc.c
C_SRC += port/gpio.c
C_SRC += port/sdmmc.c
C_SRC += port/tim.c
C_SRC += fs/src/block_drivers/block_sd_foss.c
C_SRC += fs/src/partition.c
C_SRC += fs/src/gristle.c
C_SRC += sqlite3.c
# Object files to generate and build into a static library.
OBJS   = $(C_SRC:.c=.o)
# SQLite3 compilation flags for a minimal bare-metal build.
SQFLAGS = -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_OMIT_WAL -DHAVE_FCHOWN=0 -DHAVE_READLINK=0 -DSQLITE_OS_OTHER=1
# Directories to include header files.
INCLUDE = -I. -I./device_headers -I./sys -I./fs/src -I./fs/src/block_drivers
# Linker flags.
LDFLAGS = -mthumb -mcpu=cortex-m4 -mhard-float -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g -Wall -Os -fno-strict-aliasing -fmessage-length=0 --specs=nosys.specs -nostdlib -lgcc

.PHONY: all
all: $(TARGET).a

$(TARGET).a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -c $(CFLAGS) $(SQFLAGS) $(INCLUDE) $< -o $@

.PHONY: clean
clean:
	rm -f $(OBJS)
	rm -f $(TARGET).a

#
# Makefile for esp_light_controller project
#

# Note: Set up esp cross compiler and esptool properly

SDK_BASE    = $(HOME)/tools/esp8266/sdk/ESP8266_NONOS_SDK
ESP_TOOL    = $(HOME)/tools/esp8266/esptool/esptool.py
UART_DEVICE = /dev/ttyUSB0
BAUD_RATE   = 921600

CFLAGS = -mlongcalls
LDFLAGS = -nostdlib
LIBS = -lc -lgcc -lhal -lphy -lpp -lnet80211 -lwpa -llwip -lcrypto -ljson
LIBS_DIR = $(SDK_BASE)/lib
INCLUDE_DIR = $(SDK_BASE)/include
LD_SCRIPT=$(SDK_BASE)/ld/eagle.app.v6.ld
CC=xtensa-lx106-elf-gcc
AR=xtensa-lx106-elf-ar
LD=xtensa-lx106-elf-gcc

all: main.bin

# This target will create main binaries conform to FLASH mapping address
main.bin: main.out
	@echo "Generating binaries from esptool..."
	@$(ESP_TOOL) elf2image main.out -o main > /dev/null

main.out: main.a
	@echo "Linking objects..."
	@$(LD) -o main.out $(LDFLAGS) -T$(LD_SCRIPT) -L$(LIBS_DIR) -Wl,--start-group $(LIBS) -lmain main.a -Wl,--end-group

main.a: main.o rf_init.o
	@echo "Generating archive..."
	@$(AR) rc main.a main.o rf_init.o

main.o: main.c
	@echo "Compiling $<..."
	@$(CC) -o $@ -c $(CFLAGS) -I. -I$(INCLUDE_DIR) main.c

rf_init.o: rf_init.c
	@echo "Compiling $<..."
	@$(CC) -o $@ -c $(CFLAGS) -I. -I$(INCLUDE_DIR) rf_init.c

flash: main.bin main0x00000.bin main0x10000.bin
	@echo
	@echo "### FLASHING..."
	@echo
	$(ESP_TOOL) --port $(UART_DEVICE) --baud $(BAUD_RATE) write_flash 0x0 main0x00000.bin 0x10000 main0x10000.bin

clean:
	@echo "Clean up..."
	@rm -f *.o *.bin *.out *.a
	@echo "Done"

.PHONY = clean flash

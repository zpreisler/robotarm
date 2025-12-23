MCU = atmega328p
F_CPU = 16000000UL

CFLAGS = -Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)

all: main.hex

main.elf: main.c
	avr-gcc $(CFLAGS) -o main.elf main.c

main.hex: main.elf
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

clean:
	rm -f main.elf main.hex

.PHONY: all clean

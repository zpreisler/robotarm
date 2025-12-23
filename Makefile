MCU = atmega328p
F_CPU = 16000000UL

CFLAGS = -Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)

all: main.hex

main.elf: main.o lcd.o
	avr-gcc $(CFLAGS) -o main.elf main.o lcd.o

main.o: main.c
	avr-gcc $(CFLAGS) -c -o main.o main.c

lcd.o: lcd.c lcd.h
	avr-gcc $(CFLAGS) -c -o lcd.o lcd.c

main.hex: main.elf
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

clean:
	rm -f main.elf main.hex main.o lcd.o

.PHONY: all clean

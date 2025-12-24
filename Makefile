MCU = atmega328p
F_CPU = 16000000UL

CFLAGS = -Wall -Os -DF_CPU=$(F_CPU) -mmcu=$(MCU)

all: main.hex

main.elf: main.o lcd.o buttons.o i2c.o pca9685.o uart.o serial_commands.o commands.o lcd_menu.o
	avr-gcc $(CFLAGS) -o main.elf main.o lcd.o buttons.o i2c.o pca9685.o uart.o serial_commands.o commands.o lcd_menu.o

main.o: main.c
	avr-gcc $(CFLAGS) -c -o main.o main.c

lcd.o: lcd.c lcd.h
	avr-gcc $(CFLAGS) -c -o lcd.o lcd.c

buttons.o: buttons.c buttons.h
	avr-gcc $(CFLAGS) -c -o buttons.o buttons.c

i2c.o: i2c.c i2c.h
	avr-gcc $(CFLAGS) -c -o i2c.o i2c.c

pca9685.o: pca9685.c pca9685.h i2c.h
	avr-gcc $(CFLAGS) -c -o pca9685.o pca9685.c

uart.o: uart.c uart.h
	avr-gcc $(CFLAGS) -c -o uart.o uart.c

serial_commands.o: serial_commands.c serial_commands.h uart.h commands.h lcd.h
	avr-gcc $(CFLAGS) -c -o serial_commands.o serial_commands.c

commands.o: commands.c commands.h pca9685.h serial_commands.h
	avr-gcc $(CFLAGS) -c -o commands.o commands.c

lcd_menu.o: lcd_menu.c lcd_menu.h lcd.h buttons.h commands.h
	avr-gcc $(CFLAGS) -c -o lcd_menu.o lcd_menu.c

main.hex: main.elf
	avr-objcopy -O ihex -R .eeprom main.elf main.hex

clean:
	rm -f main.elf main.hex main.o lcd.o buttons.o i2c.o pca9685.o uart.o serial_commands.o commands.o lcd_menu.o

.PHONY: all clean

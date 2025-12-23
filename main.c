#define F_CPU 16000000UL  // 16 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"

int main(void) {
    // Initialize LCD
    lcd_ddr_out();
    lcd_init();

    // Print "hello" on the LCD
    lcd_print("hello");

    while (1) {
        // Keep running
    }

    return 0;
}

#define F_CPU 16000000UL  // 16 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include "buttons.h"

int main(void) {
    // Initialize LCD and buttons
    lcd_ddr_out();
    lcd_init();
    lcd_backlight_on();
    buttons_init();

    // Display initial message
    lcd_print("Button Test");
    _delay_ms(1000);

    while (1) {
        // Read button state
        button_t button = buttons_read();

        // Clear display and show button state
        lcd_clear();
        lcd_print("Button: ");

        switch(button) {
            case BUTTON_RIGHT:
                lcd_print("RIGHT");
                break;
            case BUTTON_UP:
                lcd_print("UP");
                break;
            case BUTTON_DOWN:
                lcd_print("DOWN");
                break;
            case BUTTON_LEFT:
                lcd_print("LEFT");
                break;
            case BUTTON_SELECT:
                lcd_print("SELECT");
                break;
            default:
                lcd_print("NONE");
                break;
        }

        // Debounce delay
        _delay_ms(200);
    }

    return 0;
}

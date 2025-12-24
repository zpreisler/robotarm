#ifndef F_CPU
#define F_CPU 16000000UL  // 16 MHz clock speed
#endif

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include "buttons.h"
#include "pca9685.h"
#include "uart.h"
#include "serial.h"
#include "lcd_menu.h"
#include "commands.h"

int main(void) {
    // Initialize all systems
    lcd_ddr_out();
    lcd_init();
    lcd_backlight_on();
    buttons_init();
    uart_init(UART_BAUD_9600);
    serial_init();

    // Initialize PCA9685 for servo control
    pca9685_init(PCA9685_DEFAULT_ADDRESS);
    pca9685_set_pwm_freq(PCA9685_DEFAULT_ADDRESS, 50);  // 50Hz for servos

    // Display startup message
    lcd_print("Robot Arm Ready");
    uart_puts("\n=== Robot Arm Controller ===\n");
    uart_puts("Type START for serial mode\n");
    uart_puts("Or use buttons for menu control\n\n");
    _delay_ms(1000);

    // Initialize command system (sets all servos to center position)
    cmd_init();

    // Initialize LCD menu system
    lcd_menu_init();

    while (1) {
        // Check for serial START command
        // With interrupt-driven UART, no need for fast polling!
        if (serial_check_start()) {
            // Enter serial mode (blocking until STOP command)
            serial_mode();

            // After exiting serial mode, redisplay menu
            lcd_menu_init();
            continue;
        }

        // Update LCD menu (handles button input and display)
        lcd_menu_update();

        _delay_ms(50);
    }

    return 0;
}

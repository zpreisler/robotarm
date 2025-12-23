#define F_CPU 16000000UL  // 16 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"
#include "buttons.h"
#include "pca9685.h"

int main(void) {
    // Initialize LCD, buttons, and PCA9685
    lcd_ddr_out();
    lcd_init();
    lcd_backlight_on();
    buttons_init();

    // Initialize PCA9685 for servo control
    pca9685_init(PCA9685_DEFAULT_ADDRESS);
    pca9685_set_pwm_freq(PCA9685_DEFAULT_ADDRESS, 50);  // 50Hz for servos

    // Display startup message
    lcd_print("Robot Arm Ready");
    _delay_ms(1000);

    // Set all servos to center position (90 degrees)
    for (uint8_t i = 0; i < 6; i++) {
        pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, i, 90);
    }

    uint8_t selected_servo = 0;
    uint8_t servo_angle = 90;

    while (1) {
        // Read button state
        button_t button = buttons_read();

        // Handle button presses
        if (button == BUTTON_RIGHT && selected_servo < 5) {
            selected_servo++;
            _delay_ms(200);
        }
        else if (button == BUTTON_LEFT && selected_servo > 0) {
            selected_servo--;
            _delay_ms(200);
        }
        else if (button == BUTTON_UP && servo_angle < 180) {
            servo_angle += 5;
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, selected_servo, servo_angle);
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && servo_angle > 0) {
            servo_angle -= 5;
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, selected_servo, servo_angle);
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Reset selected servo to center
            servo_angle = 90;
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, selected_servo, servo_angle);
            _delay_ms(200);
        }

        // Update LCD display
        lcd_clear();
        lcd_print("Servo:");
        lcd_print_char((char*)(selected_servo + '0'));
        lcd_print(" Ang:");

        // Display angle (simple integer display)
        if (servo_angle >= 100) {
            lcd_print_char((char*)((servo_angle / 100) + '0'));
        }
        if (servo_angle >= 10) {
            lcd_print_char((char*)(((servo_angle / 10) % 10) + '0'));
        }
        lcd_print_char((char*)((servo_angle % 10) + '0'));

        _delay_ms(50);
    }

    return 0;
}

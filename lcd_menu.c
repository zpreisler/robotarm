#include "lcd_menu.h"
#include "lcd.h"
#include "buttons.h"
#include "pca9685.h"
#include <util/delay.h>

/* Menu states */
typedef enum {
    STATE_MENU,
    STATE_MOTORS,
    STATE_CALIBRATION,
    STATE_POSE,
    STATE_MOVE,
    STATE_MOVE_SET_DURATION
} menu_state_t;

/* Menu options */
#define MENU_MOTORS      0
#define MENU_CALIBRATION 1
#define MENU_POSE        2
#define MENU_MOVE        3
#define NUM_MENU_OPTIONS 4

/* Global state variables */
static menu_state_t current_state = STATE_MENU;
static uint8_t menu_selection = 0;
static uint8_t selected_servo = 0;
static uint8_t servo_angles[NUM_SERVOS] = {90, 90, 90, 90, 90, 90};
static uint16_t servo_pwm_values[NUM_SERVOS] = {307, 307, 307, 307, 307, 307};  // ~1500us
static uint16_t move_duration = 1000;  // Duration in ms

/* Helper function to print a number on LCD */
static void lcd_print_number(uint16_t num) {
    if (num >= 1000) {
        lcd_putc((num / 1000) + '0');
    }
    if (num >= 100) {
        lcd_putc(((num / 100) % 10) + '0');
    }
    if (num >= 10) {
        lcd_putc(((num / 10) % 10) + '0');
    }
    lcd_putc((num % 10) + '0');
}

/* Display main menu */
static void display_menu(void) {
    lcd_clear();

    // First line
    if (menu_selection == MENU_MOTORS) {
        lcd_print(">1.Motors");
    } else if (menu_selection == MENU_CALIBRATION) {
        lcd_print(" 1.Motors");
    } else {
        lcd_print(" 1.Motors");
    }

    // Second line
    lcd_set_cursor(0x40);
    if (menu_selection == MENU_CALIBRATION) {
        lcd_print(">2.Calib");
    } else if (menu_selection == MENU_POSE) {
        lcd_print(" 2.Calib >3.POSE");
    } else if (menu_selection == MENU_MOVE) {
        lcd_print(" 3.POSE >4.MOVE");
    } else {
        lcd_print(" 2.Calib");
    }
}

/* Display Mode 1: Motors (angle control) */
static void display_motors(void) {
    lcd_clear();
    lcd_print("M");
    lcd_putc(selected_servo + '0');
    lcd_print(" Ang:");
    lcd_print_number(servo_angles[selected_servo]);

    lcd_set_cursor(0x40);
    lcd_print("L/R=Srv U/D=Ang");
}

/* Display Mode 2: Calibration (PWM control) */
static void display_calibration(void) {
    lcd_clear();
    lcd_print("M");
    lcd_putc(selected_servo + '0');
    lcd_print(" PWM:");
    lcd_print_number(servo_pwm_values[selected_servo]);

    lcd_set_cursor(0x40);
    lcd_print("L/R=Srv U/D=PWM");
}

/* Display Mode 3: POSE */
static void display_pose(void) {
    lcd_clear();
    lcd_print("POSE M");
    lcd_putc(selected_servo + '0');
    lcd_print(":");
    lcd_print_number(servo_angles[selected_servo]);

    lcd_set_cursor(0x40);
    lcd_print("L/R=Srv SEL=Exec");
}

/* Display Mode 4: MOVE - duration setting */
static void display_move_duration(void) {
    lcd_clear();
    lcd_print("MOVE Duration");

    lcd_set_cursor(0x40);
    lcd_print_number(move_duration);
    lcd_print("ms  SEL=Next");
}

/* Display Mode 4: MOVE - angle setting */
static void display_move_angles(void) {
    lcd_clear();
    lcd_print("MOVE M");
    lcd_putc(selected_servo + '0');
    lcd_print(":");
    lcd_print_number(servo_angles[selected_servo]);

    lcd_set_cursor(0x40);
    lcd_print("L/R=Srv SEL=Exec");
}

/* Execute POSE command */
static void execute_pose(void) {
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, i, servo_angles[i]);
    }

    lcd_clear();
    lcd_print("POSE Executed!");
    _delay_ms(1000);
}

/* Execute MOVE command */
static void execute_move(void) {
    // Calculate number of interpolation steps (20ms per step)
    #define MOVE_STEP_DELAY_MS 20
    uint16_t num_steps = move_duration / MOVE_STEP_DELAY_MS;
    if (num_steps == 0) {
        num_steps = 1;
    }

    // Get current positions
    uint8_t start_angles[NUM_SERVOS];
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        start_angles[i] = servo_angles[i];  // Use stored angles as current
    }

    // Calculate deltas
    int16_t deltas[NUM_SERVOS];
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        deltas[i] = servo_angles[i] - start_angles[i];
    }

    // Show progress
    lcd_clear();
    lcd_print("Moving...");

    // Perform smooth interpolated movement
    for (uint16_t step = 0; step <= num_steps; step++) {
        uint16_t factor = (step * 1000UL) / num_steps;

        for (uint8_t i = 0; i < NUM_SERVOS; i++) {
            int16_t new_angle = start_angles[i] + ((deltas[i] * (int32_t)factor) / 1000);

            if (new_angle < 0) new_angle = 0;
            if (new_angle > 180) new_angle = 180;

            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, i, (uint8_t)new_angle);
        }

        if (step < num_steps) {
            _delay_ms(MOVE_STEP_DELAY_MS);
        }
    }

    lcd_clear();
    lcd_print("MOVE Complete!");
    _delay_ms(1000);
}

/* Initialize menu system */
void lcd_menu_init(void) {
    current_state = STATE_MENU;
    menu_selection = 0;
    selected_servo = 0;

    // Initialize all servos to center
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        servo_angles[i] = 90;
        servo_pwm_values[i] = 307;  // Approximately 1500us
    }

    display_menu();
}

/* Update menu system - call in main loop */
uint8_t lcd_menu_update(void) {
    button_t button = buttons_read();

    if (button == BUTTON_NONE) {
        return 0;  // No action
    }

    /* ===== STATE: MENU ===== */
    if (current_state == STATE_MENU) {
        if (button == BUTTON_UP && menu_selection > 0) {
            menu_selection--;
            display_menu();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN && menu_selection < NUM_MENU_OPTIONS - 1) {
            menu_selection++;
            display_menu();
            _delay_ms(200);
        }
        else if (button == BUTTON_SELECT) {
            // Enter selected mode
            selected_servo = 0;

            switch (menu_selection) {
                case MENU_MOTORS:
                    current_state = STATE_MOTORS;
                    display_motors();
                    break;
                case MENU_CALIBRATION:
                    current_state = STATE_CALIBRATION;
                    display_calibration();
                    break;
                case MENU_POSE:
                    current_state = STATE_POSE;
                    display_pose();
                    break;
                case MENU_MOVE:
                    current_state = STATE_MOVE_SET_DURATION;
                    display_move_duration();
                    break;
            }
            _delay_ms(200);
        }
    }

    /* ===== STATE: MOTORS ===== */
    else if (current_state == STATE_MOTORS) {
        if (button == BUTTON_LEFT && selected_servo > 0) {
            selected_servo--;
            display_motors();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT && selected_servo < NUM_SERVOS - 1) {
            selected_servo++;
            display_motors();
            _delay_ms(200);
        }
        else if (button == BUTTON_UP && servo_angles[selected_servo] < 180) {
            servo_angles[selected_servo] += 5;
            if (servo_angles[selected_servo] > 180) {
                servo_angles[selected_servo] = 180;
            }
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, selected_servo,
                                   servo_angles[selected_servo]);
            display_motors();
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && servo_angles[selected_servo] > 0) {
            if (servo_angles[selected_servo] >= 5) {
                servo_angles[selected_servo] -= 5;
            } else {
                servo_angles[selected_servo] = 0;
            }
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, selected_servo,
                                   servo_angles[selected_servo]);
            display_motors();
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Return to menu
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    /* ===== STATE: CALIBRATION ===== */
    else if (current_state == STATE_CALIBRATION) {
        if (button == BUTTON_LEFT && selected_servo > 0) {
            selected_servo--;
            display_calibration();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT && selected_servo < NUM_SERVOS - 1) {
            selected_servo++;
            display_calibration();
            _delay_ms(200);
        }
        else if (button == BUTTON_UP && servo_pwm_values[selected_servo] < 4095) {
            servo_pwm_values[selected_servo] += 10;
            if (servo_pwm_values[selected_servo] > 4095) {
                servo_pwm_values[selected_servo] = 4095;
            }
            pca9685_set_servo_pwm(PCA9685_DEFAULT_ADDRESS, selected_servo,
                                 servo_pwm_values[selected_servo]);
            display_calibration();
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && servo_pwm_values[selected_servo] > 0) {
            if (servo_pwm_values[selected_servo] >= 10) {
                servo_pwm_values[selected_servo] -= 10;
            } else {
                servo_pwm_values[selected_servo] = 0;
            }
            pca9685_set_servo_pwm(PCA9685_DEFAULT_ADDRESS, selected_servo,
                                 servo_pwm_values[selected_servo]);
            display_calibration();
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Return to menu
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    /* ===== STATE: POSE ===== */
    else if (current_state == STATE_POSE) {
        if (button == BUTTON_LEFT && selected_servo > 0) {
            selected_servo--;
            display_pose();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT && selected_servo < NUM_SERVOS - 1) {
            selected_servo++;
            display_pose();
            _delay_ms(200);
        }
        else if (button == BUTTON_UP && servo_angles[selected_servo] < 180) {
            servo_angles[selected_servo] += 5;
            if (servo_angles[selected_servo] > 180) {
                servo_angles[selected_servo] = 180;
            }
            display_pose();
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && servo_angles[selected_servo] > 0) {
            if (servo_angles[selected_servo] >= 5) {
                servo_angles[selected_servo] -= 5;
            } else {
                servo_angles[selected_servo] = 0;
            }
            display_pose();
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Execute POSE
            execute_pose();
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    /* ===== STATE: MOVE - Set Duration ===== */
    else if (current_state == STATE_MOVE_SET_DURATION) {
        if (button == BUTTON_UP && move_duration < 9900) {
            move_duration += 100;
            display_move_duration();
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && move_duration > 100) {
            move_duration -= 100;
            display_move_duration();
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Move to angle setting
            selected_servo = 0;
            current_state = STATE_MOVE;
            display_move_angles();
            _delay_ms(200);
        }
        else if (button == BUTTON_LEFT) {
            // Cancel - return to menu
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    /* ===== STATE: MOVE - Set Angles ===== */
    else if (current_state == STATE_MOVE) {
        if (button == BUTTON_LEFT && selected_servo > 0) {
            selected_servo--;
            display_move_angles();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT && selected_servo < NUM_SERVOS - 1) {
            selected_servo++;
            display_move_angles();
            _delay_ms(200);
        }
        else if (button == BUTTON_UP && servo_angles[selected_servo] < 180) {
            servo_angles[selected_servo] += 5;
            if (servo_angles[selected_servo] > 180) {
                servo_angles[selected_servo] = 180;
            }
            display_move_angles();
            _delay_ms(100);
        }
        else if (button == BUTTON_DOWN && servo_angles[selected_servo] > 0) {
            if (servo_angles[selected_servo] >= 5) {
                servo_angles[selected_servo] -= 5;
            } else {
                servo_angles[selected_servo] = 0;
            }
            display_move_angles();
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            // Execute MOVE
            execute_move();
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    return 0;
}

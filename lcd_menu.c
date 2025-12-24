#include "lcd_menu.h"
#include "lcd.h"
#include "buttons.h"
#include "commands.h"
#include <util/delay.h>

/* Menu states */
typedef enum {
    STATE_MENU,
    STATE_MOTORS,
    STATE_CALIBRATION,
    STATE_POSE,
    STATE_MOVE
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
static uint16_t move_duration = 1000;  // Duration in ms for MOVE command

/* Temporary arrays for POSE/MOVE editing (supports up to 16 servos) */
static uint8_t temp_angles[16];

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

/* Helper function to print a menu item */
static void print_menu_item(uint8_t item, uint8_t is_selected) {
    // Print selection indicator
    lcd_putc(is_selected ? '>' : ' ');

    // Print item number (1-based)
    lcd_putc(item + '1');
    lcd_putc('.');

    // Print item name
    switch (item) {
        case MENU_MOTORS:
            lcd_print("Motors");
            break;
        case MENU_CALIBRATION:
            lcd_print("Calibration");
            break;
        case MENU_POSE:
            lcd_print("POSE");
            break;
        case MENU_MOVE:
            lcd_print("MOVE");
            break;
    }
}

/* Display main menu */
static void display_menu(void) {
    lcd_clear();

    // Calculate which 2 items to show (scrolling window)
    uint8_t first_visible = menu_selection;
    if (first_visible > 0) first_visible--;
    if (first_visible > NUM_MENU_OPTIONS - 2) {
        first_visible = NUM_MENU_OPTIONS - 2;
    }

    // First line
    print_menu_item(first_visible, menu_selection == first_visible);

    // Second line
    lcd_set_cursor(0x40);
    print_menu_item(first_visible + 1, menu_selection == first_visible + 1);
}

/* Display Mode 1: Motors (angle control) */
static void display_motors(void) {
    lcd_clear();
    lcd_print("Motor ");
    lcd_putc(selected_servo + '0');

    lcd_set_cursor(0x40);
    lcd_print("Angle: ");
    lcd_print_number(cmd_get_servo_angle(selected_servo));
}

/* Display Mode 2: Calibration (PWM pulse width control) */
static void display_calibration(void) {
    lcd_clear();
    lcd_print("Motor ");
    lcd_putc(selected_servo + '0');

    lcd_set_cursor(0x40);
    lcd_print("PWM: ");
    lcd_print_number(cmd_get_servo_pwm_us(selected_servo));
    lcd_print("us");
}

/* Display Mode 3: POSE */
static void display_pose(void) {
    lcd_clear();

    if (selected_servo < NUM_SERVOS) {
        // Showing a servo
        lcd_print("POSE Motor ");
        lcd_putc(selected_servo + '0');

        lcd_set_cursor(0x40);
        lcd_print("Angle: ");
        lcd_print_number(temp_angles[selected_servo]);
    } else {
        // Execute option
        lcd_print("POSE");

        lcd_set_cursor(0x40);
        lcd_print("Execute");
    }
}

/* Display Mode 4: MOVE */
static void display_move(void) {
    lcd_clear();

    if (selected_servo == 0) {
        // Duration setting
        lcd_print("MOVE Duration");

        lcd_set_cursor(0x40);
        lcd_print_number(move_duration);
        lcd_print("ms");
    } else if (selected_servo <= NUM_SERVOS) {
        // Servo angle (selected_servo 1-6 maps to servo 0-5)
        lcd_print("MOVE Motor ");
        lcd_putc((selected_servo - 1) + '0');

        lcd_set_cursor(0x40);
        lcd_print("Angle: ");
        lcd_print_number(temp_angles[selected_servo - 1]);
    } else {
        // Execute option
        lcd_print("MOVE");

        lcd_set_cursor(0x40);
        lcd_print("Execute");
    }
}

/* Execute POSE command */
static void execute_pose(void) {
    cmd_execute_pose(temp_angles, NUM_SERVOS);

    lcd_clear();
    lcd_print("POSE Executed!");
    _delay_ms(1000);
}

/* Execute MOVE command */
static void execute_move(void) {
    lcd_clear();
    lcd_print("Moving...");

    cmd_execute_move(move_duration, temp_angles, NUM_SERVOS);

    lcd_clear();
    lcd_print("MOVE Complete!");
    _delay_ms(1000);
}

/* Initialize menu system */
void lcd_menu_init(void) {
    current_state = STATE_MENU;
    menu_selection = 0;
    selected_servo = 0;

    // Initialize temp angles from current servo positions
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        temp_angles[i] = cmd_get_servo_angle(i);
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
        if (button == BUTTON_UP) {
            if (menu_selection > 0) {
                menu_selection--;
            } else {
                menu_selection = NUM_MENU_OPTIONS - 1;  // Wrap to end
            }
            display_menu();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN) {
            if (menu_selection < NUM_MENU_OPTIONS - 1) {
                menu_selection++;
            } else {
                menu_selection = 0;  // Wrap to beginning
            }
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
                    current_state = STATE_MOVE;
                    display_move();
                    break;
            }
            _delay_ms(200);
        }
    }

    /* ===== STATE: MOTORS ===== */
    else if (current_state == STATE_MOTORS) {
        if (button == BUTTON_UP) {
            if (selected_servo > 0) {
                selected_servo--;
            } else {
                selected_servo = NUM_SERVOS - 1;  // Wrap to last servo
            }
            display_motors();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN) {
            if (selected_servo < NUM_SERVOS - 1) {
                selected_servo++;
            } else {
                selected_servo = 0;  // Wrap to first servo
            }
            display_motors();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT) {
            uint8_t angle = cmd_get_servo_angle(selected_servo);
            if (angle < 180) {
                angle += 5;
                if (angle > 180) {
                    angle = 180;
                }
                cmd_set_servo_angle(selected_servo, angle);
            }
            display_motors();
            _delay_ms(100);
        }
        else if (button == BUTTON_LEFT) {
            uint8_t angle = cmd_get_servo_angle(selected_servo);
            if (angle > 0) {
                if (angle >= 5) {
                    angle -= 5;
                } else {
                    angle = 0;
                }
                cmd_set_servo_angle(selected_servo, angle);
            }
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
        if (button == BUTTON_UP) {
            if (selected_servo > 0) {
                selected_servo--;
            } else {
                selected_servo = NUM_SERVOS - 1;  // Wrap to last servo
            }
            display_calibration();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN) {
            if (selected_servo < NUM_SERVOS - 1) {
                selected_servo++;
            } else {
                selected_servo = 0;  // Wrap to first servo
            }
            display_calibration();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT) {
            uint16_t pulse_us = cmd_get_servo_pwm_us(selected_servo);
            if (pulse_us < 20000) {
                pulse_us += 10;
                if (pulse_us > 20000) {
                    pulse_us = 20000;
                }
                cmd_set_servo_pwm_us(selected_servo, pulse_us);
            }
            display_calibration();
            _delay_ms(100);
        }
        else if (button == BUTTON_LEFT) {
            uint16_t pulse_us = cmd_get_servo_pwm_us(selected_servo);
            if (pulse_us > 0) {
                if (pulse_us >= 10) {
                    pulse_us -= 10;
                } else {
                    pulse_us = 0;
                }
                cmd_set_servo_pwm_us(selected_servo, pulse_us);
            }
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
        uint8_t num_items = NUM_SERVOS + 1;  // Servos + Execute

        if (button == BUTTON_UP) {
            if (selected_servo > 0) {
                selected_servo--;
            } else {
                selected_servo = num_items - 1;  // Wrap to Execute
            }
            display_pose();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN) {
            if (selected_servo < num_items - 1) {
                selected_servo++;
            } else {
                selected_servo = 0;  // Wrap to first servo
            }
            display_pose();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT && selected_servo < NUM_SERVOS) {
            // Only adjust angle if on a servo item
            if (temp_angles[selected_servo] < 180) {
                temp_angles[selected_servo] += 5;
                if (temp_angles[selected_servo] > 180) {
                    temp_angles[selected_servo] = 180;
                }
                display_pose();
            }
            _delay_ms(100);
        }
        else if (button == BUTTON_LEFT && selected_servo < NUM_SERVOS) {
            // Only adjust angle if on a servo item
            if (temp_angles[selected_servo] > 0) {
                if (temp_angles[selected_servo] >= 5) {
                    temp_angles[selected_servo] -= 5;
                } else {
                    temp_angles[selected_servo] = 0;
                }
                display_pose();
            }
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            if (selected_servo == NUM_SERVOS) {
                // Execute
                execute_pose();
            }
            // Return to menu
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    /* ===== STATE: MOVE ===== */
    else if (current_state == STATE_MOVE) {
        uint8_t num_items = NUM_SERVOS + 2;  // Duration + Servos + Execute

        if (button == BUTTON_UP) {
            if (selected_servo > 0) {
                selected_servo--;
            } else {
                selected_servo = num_items - 1;  // Wrap to Execute
            }
            display_move();
            _delay_ms(200);
        }
        else if (button == BUTTON_DOWN) {
            if (selected_servo < num_items - 1) {
                selected_servo++;
            } else {
                selected_servo = 0;  // Wrap to Duration
            }
            display_move();
            _delay_ms(200);
        }
        else if (button == BUTTON_RIGHT) {
            if (selected_servo == 0) {
                // Adjust duration
                if (move_duration < 9900) {
                    move_duration += 100;
                    display_move();
                }
            } else if (selected_servo <= NUM_SERVOS) {
                // Adjust servo angle
                uint8_t servo_idx = selected_servo - 1;
                if (temp_angles[servo_idx] < 180) {
                    temp_angles[servo_idx] += 5;
                    if (temp_angles[servo_idx] > 180) {
                        temp_angles[servo_idx] = 180;
                    }
                    display_move();
                }
            }
            _delay_ms(100);
        }
        else if (button == BUTTON_LEFT) {
            if (selected_servo == 0) {
                // Adjust duration
                if (move_duration > 100) {
                    move_duration -= 100;
                    display_move();
                }
            } else if (selected_servo <= NUM_SERVOS) {
                // Adjust servo angle
                uint8_t servo_idx = selected_servo - 1;
                if (temp_angles[servo_idx] > 0) {
                    if (temp_angles[servo_idx] >= 5) {
                        temp_angles[servo_idx] -= 5;
                    } else {
                        temp_angles[servo_idx] = 0;
                    }
                    display_move();
                }
            }
            _delay_ms(100);
        }
        else if (button == BUTTON_SELECT) {
            if (selected_servo == NUM_SERVOS + 1) {
                // Execute
                execute_move();
            }
            // Return to menu
            current_state = STATE_MENU;
            display_menu();
            _delay_ms(200);
        }
    }

    return 0;
}

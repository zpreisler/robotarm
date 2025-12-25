#ifndef LCD_MENU_H
#define LCD_MENU_H

#include <stdint.h>
#include "serial.h"  // For NUM_SERVOS

/**
 * LCD Menu System for Robot Arm Controller
 *
 * Provides 4 control modes:
 * 1. Motors - Select servo and adjust angle (0-180°)
 * 2. Calibration - Select servo and adjust PWM directly (0-4095)
 * 3. POSE - Set all servo angles and execute
 * 4. MOVE - Set duration and all servo angles for smooth movement
 */

/**
 * Initialize the LCD menu system
 * Sets initial state and servo positions
 */
void lcd_menu_init(void);

/**
 * Update the LCD menu system
 * Call this in main loop to process button input and update display
 * Returns 1 if serial mode should be entered, 0 otherwise
 */
uint8_t lcd_menu_update(void);

#endif /* LCD_MENU_H */

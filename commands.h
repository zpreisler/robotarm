#ifndef COMMANDS_H
#define COMMANDS_H

#include <stdint.h>
#include "serial.h"  // For NUM_SERVOS

/**
 * Centralized Servo Command Module
 *
 * This module provides a unified interface for all servo control operations,
 * used by both serial commands and LCD menu system.
 * Maintains state and provides high-level commands like POSE and MOVE.
 */

/**
 * Initialize the command system
 * Sets all servos to center position (90 degrees)
 */
void cmd_init(void);

/**
 * Set a single servo by angle (0-180 degrees)
 * Updates internal state and sends command to PCA9685
 * @param channel Servo channel (0-5)
 * @param angle Angle in degrees (0-180)
 */
void cmd_set_servo_angle(uint8_t channel, uint8_t angle);

/**
 * Set a single servo by PWM pulse width in microseconds (0-20000us)
 * Updates internal state and sends command to PCA9685
 * Used for calibration mode
 * @param channel Servo channel (0-5)
 * @param pulse_us Pulse width in microseconds (0-20000)
 */
void cmd_set_servo_pwm_us(uint8_t channel, uint16_t pulse_us);

/**
 * Get current angle of a servo
 * @param channel Servo channel (0-5)
 * @return Current angle (0-180)
 */
uint8_t cmd_get_servo_angle(uint8_t channel);

/**
 * Get current PWM pulse width of a servo (for calibration)
 * @param channel Servo channel (0-5)
 * @return Current pulse width in microseconds (0-20000)
 */
uint16_t cmd_get_servo_pwm_us(uint8_t channel);

/**
 * Execute POSE command - set multiple servos instantly
 * @param angles Array of target angles
 * @param num_servos Number of servos to set (1-NUM_SERVOS)
 */
void cmd_execute_pose(const uint8_t *angles, uint8_t num_servos);

/**
 * Execute MOVE command - smooth interpolated movement
 * All servos move simultaneously and finish at the same time
 * @param duration_ms Movement duration in milliseconds
 * @param target_angles Array of target angles
 * @param num_servos Number of servos to move (1-NUM_SERVOS)
 */
void cmd_execute_move(uint16_t duration_ms, const uint8_t *target_angles, uint8_t num_servos);

#endif /* COMMANDS_H */

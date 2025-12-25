#include "commands.h"
#include "pca9685.h"
#include <util/delay.h>

/* State arrays - track current positions (support up to 16 servos) */
static uint8_t servo_angles[16];        // Angles in degrees (0-180)
static uint16_t servo_pulse_widths[16]; // Pulse widths in microseconds (0-20000)

/**
 * Initialize command system
 * Sets all servos to center position
 */
void cmd_init(void) {
    // Initialize to center position (90° = 1500us pulse width)
    for (uint8_t i = 0; i < NUM_SERVOS; i++) {
        servo_angles[i] = 90;
        servo_pulse_widths[i] = 1500;  // Center pulse width
        pca9685_set_servo_pwm_us(PCA9685_DEFAULT_ADDRESS, i, 1500);
    }
}

/**
 * Set single servo by angle
 */
void cmd_set_servo_angle(uint8_t channel, uint8_t angle) {
    if (channel >= NUM_SERVOS) {
        return;  // Invalid channel
    }

    // Clamp angle to valid range
    if (angle > 180) {
        angle = 180;
    }

    // Calculate pulse width from angle (using standard servo range)
    uint16_t pulse_us = SERVO_MIN_PULSE + ((uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180);

    // Update state
    servo_angles[channel] = angle;
    servo_pulse_widths[channel] = pulse_us;

    // Send to hardware
    pca9685_set_servo_pwm_us(PCA9685_DEFAULT_ADDRESS, channel, pulse_us);
}

/**
 * Set single servo by PWM pulse width (calibration mode)
 */
void cmd_set_servo_pwm_us(uint8_t channel, uint16_t pulse_us) {
    if (channel >= NUM_SERVOS) {
        return;  // Invalid channel
    }

    // Clamp pulse width to valid range (max 20000us at 50Hz)
    if (pulse_us > 20000) {
        pulse_us = 20000;
    }

    // Update state
    servo_pulse_widths[channel] = pulse_us;

    // Send to hardware
    pca9685_set_servo_pwm_us(PCA9685_DEFAULT_ADDRESS, channel, pulse_us);
}

/**
 * Get current servo angle
 */
uint8_t cmd_get_servo_angle(uint8_t channel) {
    if (channel >= NUM_SERVOS) {
        return 90;  // Default
    }
    return servo_angles[channel];
}

/**
 * Get current servo PWM pulse width
 */
uint16_t cmd_get_servo_pwm_us(uint8_t channel) {
    if (channel >= NUM_SERVOS) {
        return 1500;  // Default center
    }
    return servo_pulse_widths[channel];
}

/**
 * Execute POSE - set multiple servos instantly
 */
void cmd_execute_pose(const uint8_t *angles, uint8_t num_servos) {
    if (num_servos > NUM_SERVOS) {
        num_servos = NUM_SERVOS;
    }

    // Set all servos
    for (uint8_t i = 0; i < num_servos; i++) {
        cmd_set_servo_angle(i, angles[i]);
    }
}

/**
 * Execute MOVE - smooth interpolated movement
 * All servos finish simultaneously using linear interpolation
 */
void cmd_execute_move(uint16_t duration_ms, const uint8_t *target_angles, uint8_t num_servos) {
    if (num_servos > NUM_SERVOS) {
        num_servos = NUM_SERVOS;
    }

    // Calculate number of interpolation steps (20ms per step)
    // Match PCA9685 PWM update frequency (50Hz = 20ms period)
    #define MOVE_STEP_DELAY_MS 20
    uint16_t num_steps = duration_ms / MOVE_STEP_DELAY_MS;
    if (num_steps == 0) {
        num_steps = 1;  // Minimum one step
    }

    // Pre-calculate starting positions and deltas (sized for max 16 servos)
    int16_t start_angles[16];
    int16_t deltas[16];

    for (uint8_t i = 0; i < num_servos; i++) {
        start_angles[i] = servo_angles[i];  // Current position
        deltas[i] = target_angles[i] - servo_angles[i];  // Change needed
    }

    // Perform smooth interpolated movement
    for (uint16_t step = 0; step <= num_steps; step++) {
        // Calculate interpolation factor (0-1000 for fixed-point arithmetic)
        uint16_t factor = (step * 1000UL) / num_steps;

        // Update each servo position
        for (uint8_t i = 0; i < num_servos; i++) {
            // Linear interpolation: start + delta * factor
            // factor is 0-1000, so divide by 1000 after multiplication
            int16_t new_angle = start_angles[i] + ((deltas[i] * (int32_t)factor) / 1000);

            // Clamp to valid range
            if (new_angle < 0) new_angle = 0;
            if (new_angle > 180) new_angle = 180;

            // Send to hardware (don't update state yet, wait for final position)
            pca9685_set_servo_angle(PCA9685_DEFAULT_ADDRESS, i, (uint8_t)new_angle);
        }

        // Delay between steps (except after the last step)
        if (step < num_steps) {
            _delay_ms(MOVE_STEP_DELAY_MS);
        }
    }

    // Update stored angles to final positions
    for (uint8_t i = 0; i < num_servos; i++) {
        servo_angles[i] = target_angles[i];
    }
}

#ifndef PCA9685_H
#define PCA9685_H

#include <avr/io.h>
#include <stdint.h>

/*
 * PCA9685 16-Channel 12-bit PWM Driver
 *
 * I2C Address: 0x40 (default)
 * PWM Frequency: 50Hz (standard for servos)
 * PWM Resolution: 12-bit (4096 steps)
 */

/* PCA9685 Default I2C Address */
#define PCA9685_DEFAULT_ADDRESS 0x40

/* PCA9685 Register Addresses */
#define PCA9685_MODE1        0x00
#define PCA9685_MODE2        0x01
#define PCA9685_SUBADR1      0x02
#define PCA9685_SUBADR2      0x03
#define PCA9685_SUBADR3      0x04
#define PCA9685_PRESCALE     0xFE
#define PCA9685_LED0_ON_L    0x06
#define PCA9685_LED0_ON_H    0x07
#define PCA9685_LED0_OFF_L   0x08
#define PCA9685_LED0_OFF_H   0x09
#define PCA9685_ALL_LED_ON_L 0xFA
#define PCA9685_ALL_LED_ON_H 0xFB
#define PCA9685_ALL_LED_OFF_L 0xFC
#define PCA9685_ALL_LED_OFF_H 0xFD

/* MODE1 Register Bits */
#define PCA9685_MODE1_RESTART  0x80
#define PCA9685_MODE1_SLEEP    0x10
#define PCA9685_MODE1_ALLCALL  0x01
#define PCA9685_MODE1_AI       0x20

/* MODE2 Register Bits */
#define PCA9685_MODE2_OUTDRV   0x04

/* Configuration */
#define PCA9685_CHANNELS       16
#define PCA9685_PWM_FULL       4096

/* Servo timing defaults (microseconds) */
#define SERVO_MIN_PULSE        500   // Extended range - try wider pulses
#define SERVO_MAX_PULSE        2500  // Extended range - try wider pulses
#define SERVO_CENTER_PULSE     1500

/* Function declarations */

/**
 * Initialize PCA9685
 * @param address I2C address of PCA9685 (typically 0x40)
 */
void pca9685_init(uint8_t address);

/**
 * Set PWM frequency
 * @param address I2C address of PCA9685
 * @param freq Frequency in Hz (typical: 50Hz for servos)
 */
void pca9685_set_pwm_freq(uint8_t address, uint16_t freq);

/**
 * Set PWM output for a specific channel
 * @param address I2C address of PCA9685
 * @param channel Channel number (0-15)
 * @param on 12-bit ON time (0-4095)
 * @param off 12-bit OFF time (0-4095)
 */
void pca9685_set_pwm(uint8_t address, uint8_t channel, uint16_t on, uint16_t off);

/**
 * Set servo angle (0-180 degrees)
 * @param address I2C address of PCA9685
 * @param channel Channel number (0-15)
 * @param angle Angle in degrees (0-180)
 */
void pca9685_set_servo_angle(uint8_t address, uint8_t channel, uint8_t angle);

/**
 * Set servo pulse width in microseconds
 * @param address I2C address of PCA9685
 * @param channel Channel number (0-15)
 * @param pulse_us Pulse width in microseconds (typically 1000-2000)
 */
void pca9685_set_servo_pulse(uint8_t address, uint8_t channel, uint16_t pulse_us);

/**
 * Set servo PWM value directly (for calibration)
 * @param address I2C address of PCA9685
 * @param channel Channel number (0-15)
 * @param pwm_value Direct PWM value (0-4095)
 */
void pca9685_set_servo_pwm(uint8_t address, uint8_t channel, uint16_t pwm_value);

/**
 * Turn off all PWM outputs
 * @param address I2C address of PCA9685
 */
void pca9685_all_off(uint8_t address);

#endif /* PCA9685_H */

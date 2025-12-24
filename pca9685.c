#include "pca9685.h"
#include "i2c.h"
#include <util/delay.h>

/*
 * Initialize PCA9685
 * Sets up I2C and configures PCA9685 for servo control
 */
void pca9685_init(uint8_t address) {
    // Initialize I2C
    i2c_init();

    // Reset PCA9685
    i2c_write_byte(address, PCA9685_MODE1, PCA9685_MODE1_RESTART);
    _delay_ms(10);

    // Set to sleep mode to change prescaler
    i2c_write_byte(address, PCA9685_MODE1, PCA9685_MODE1_SLEEP);
    _delay_ms(1);

    // Set PWM frequency to 50Hz (default for servos)
    pca9685_set_pwm_freq(address, 50);

    // Wake up and enable auto-increment
    uint8_t mode1 = PCA9685_MODE1_AI | PCA9685_MODE1_ALLCALL;
    i2c_write_byte(address, PCA9685_MODE1, mode1);
    _delay_ms(1);

    // Set output mode to totem pole (not open drain)
    i2c_write_byte(address, PCA9685_MODE2, PCA9685_MODE2_OUTDRV);
}

/*
 * Set PWM frequency
 * Formula: prescale = round(25MHz / (4096 * freq)) - 1
 */
void pca9685_set_pwm_freq(uint8_t address, uint16_t freq) {
    // Calculate prescale value
    // 25MHz is the internal oscillator frequency
    uint32_t prescaleval = 25000000;
    prescaleval /= 4096;
    prescaleval /= freq;
    prescaleval -= 1;

    uint8_t prescale = (uint8_t)prescaleval;

    // Read current MODE1 register
    uint8_t oldmode = i2c_read_byte(address, PCA9685_MODE1);

    // Set to sleep mode (required to change prescale)
    uint8_t newmode = (oldmode & 0x7F) | PCA9685_MODE1_SLEEP;
    i2c_write_byte(address, PCA9685_MODE1, newmode);

    // Write prescale value
    i2c_write_byte(address, PCA9685_PRESCALE, prescale);

    // Restore old mode
    i2c_write_byte(address, PCA9685_MODE1, oldmode);
    _delay_ms(1);

    // Restart
    i2c_write_byte(address, PCA9685_MODE1, oldmode | PCA9685_MODE1_RESTART);
}

/*
 * Set PWM output for a specific channel
 * on: 12-bit ON time (0-4095)
 * off: 12-bit OFF time (0-4095)
 */
void pca9685_set_pwm(uint8_t address, uint8_t channel, uint16_t on, uint16_t off) {
    if (channel >= PCA9685_CHANNELS) {
        return;  // Invalid channel
    }

    // Calculate register address for this channel
    uint8_t reg = PCA9685_LED0_ON_L + (channel * 4);

    // Write ON and OFF values (4 bytes total)
    // Using I2C auto-increment feature
    i2c_start(address << 1);
    i2c_write(reg);
    i2c_write(on & 0xFF);         // ON_L
    i2c_write((on >> 8) & 0xFF);  // ON_H
    i2c_write(off & 0xFF);        // OFF_L
    i2c_write((off >> 8) & 0xFF); // OFF_H
    i2c_stop();
}

/*
 * Set servo angle (0-180 degrees)
 * Converts angle to pulse width and sets PWM
 */
void pca9685_set_servo_angle(uint8_t address, uint8_t channel, uint8_t angle) {
    // Limit angle to 0-180
    if (angle > 180) {
        angle = 180;
    }

    // Convert angle to pulse width (1000-2000 microseconds)
    // pulse_us = SERVO_MIN_PULSE + (angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180)
    uint16_t pulse_us = SERVO_MIN_PULSE + ((uint32_t)angle * (SERVO_MAX_PULSE - SERVO_MIN_PULSE) / 180);

    // Set servo pulse
    pca9685_set_servo_pwm_us(address, channel, pulse_us);
}

/*
 * Set servo pulse width in microseconds
 * Converts microseconds to 12-bit PWM value
 */
void pca9685_set_servo_pwm_us(uint8_t address, uint8_t channel, uint16_t pulse_us) {
    // At 50Hz, period = 20000 microseconds
    // PWM resolution = 4096 steps
    // PWM_value = (pulse_us * 4096) / 20000

    uint32_t pwm_value = ((uint32_t)pulse_us * 4096) / 20000;

    // Limit to 12-bit value
    if (pwm_value > 4095) {
        pwm_value = 4095;
    }

    // Set PWM with ON time at 0, OFF time at calculated value
    pca9685_set_pwm(address, channel, 0, (uint16_t)pwm_value);
}

/*
 * Set servo PWM value directly (raw 12-bit value)
 * Bypasses angle/pulse conversion for direct hardware control
 */
void pca9685_set_servo_pwm_raw(uint8_t address, uint8_t channel, uint16_t pwm_value) {
    // Limit to 12-bit value
    if (pwm_value > 4095) {
        pwm_value = 4095;
    }

    // Set PWM with ON time at 0, OFF time at specified value
    pca9685_set_pwm(address, channel, 0, pwm_value);
}

/*
 * Turn off all PWM outputs
 */
void pca9685_all_off(uint8_t address) {
    i2c_write_byte(address, PCA9685_ALL_LED_OFF_L, 0);
    i2c_write_byte(address, PCA9685_ALL_LED_OFF_H, 0x10);  // Full OFF bit
}

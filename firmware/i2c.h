#ifndef I2C_H
#define I2C_H

#include <avr/io.h>
#include <stdint.h>

/*
 * I2C/TWI Communication Library for ATmega328p
 *
 * Hardware pins:
 * - SDA: PC4 (Arduino A4)
 * - SCL: PC5 (Arduino A5)
 */

/* I2C Status Codes */
#define I2C_START           0x08
#define I2C_REP_START       0x10
#define I2C_MT_SLA_ACK      0x18
#define I2C_MT_SLA_NACK     0x20
#define I2C_MT_DATA_ACK     0x28
#define I2C_MT_DATA_NACK    0x30
#define I2C_MR_SLA_ACK      0x40
#define I2C_MR_SLA_NACK     0x48
#define I2C_MR_DATA_ACK     0x50
#define I2C_MR_DATA_NACK    0x58

/* I2C Error Codes */
#define I2C_OK              0
#define I2C_ERROR_NODEV     1
#define I2C_ERROR_DATA      2

/* Function declarations */
void i2c_init(void);
uint8_t i2c_start(uint8_t address);
void i2c_stop(void);
uint8_t i2c_write(uint8_t data);
uint8_t i2c_read_ack(void);
uint8_t i2c_read_nack(void);

/* Helper functions */
uint8_t i2c_write_byte(uint8_t address, uint8_t reg, uint8_t data);
uint8_t i2c_read_byte(uint8_t address, uint8_t reg);

#endif /* I2C_H */

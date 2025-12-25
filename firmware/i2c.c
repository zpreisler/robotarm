#include "i2c.h"

/*
 * Initialize I2C/TWI
 * Sets clock frequency to 100kHz (standard mode)
 * Formula: SCL = F_CPU / (16 + 2*TWBR*Prescaler)
 * For 100kHz with 16MHz: TWBR = 72, Prescaler = 1
 */
void i2c_init(void) {
    // Set bit rate register for 100kHz
    TWBR = 72;

    // Set prescaler to 1 (TWPS = 00)
    TWSR = 0x00;

    // Enable TWI
    TWCR = (1 << TWEN);
}

/*
 * Send I2C START condition
 * Returns: I2C_OK on success, error code on failure
 */
uint8_t i2c_start(uint8_t address) {
    // Send START condition
    TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);

    // Wait for TWINT flag
    while (!(TWCR & (1 << TWINT)));

    // Check status
    if ((TWSR & 0xF8) != I2C_START && (TWSR & 0xF8) != I2C_REP_START) {
        return I2C_ERROR_NODEV;
    }

    // Send device address
    TWDR = address;
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag
    while (!(TWCR & (1 << TWINT)));

    // Check status
    uint8_t status = TWSR & 0xF8;
    if (status != I2C_MT_SLA_ACK && status != I2C_MR_SLA_ACK) {
        return I2C_ERROR_NODEV;
    }

    return I2C_OK;
}

/*
 * Send I2C STOP condition
 */
void i2c_stop(void) {
    // Send STOP condition
    TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);

    // Wait for STOP to complete
    while (TWCR & (1 << TWSTO));
}

/*
 * Write one byte to I2C bus
 * Returns: I2C_OK on success, error code on failure
 */
uint8_t i2c_write(uint8_t data) {
    // Load data into data register
    TWDR = data;
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag
    while (!(TWCR & (1 << TWINT)));

    // Check status
    if ((TWSR & 0xF8) != I2C_MT_DATA_ACK) {
        return I2C_ERROR_DATA;
    }

    return I2C_OK;
}

/*
 * Read one byte from I2C bus with ACK
 * Used when more bytes will be read
 */
uint8_t i2c_read_ack(void) {
    // Enable TWI, clear TWINT, send ACK
    TWCR = (1 << TWINT) | (1 << TWEN) | (1 << TWEA);

    // Wait for TWINT flag
    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

/*
 * Read one byte from I2C bus with NACK
 * Used for the last byte to be read
 */
uint8_t i2c_read_nack(void) {
    // Enable TWI, clear TWINT, send NACK
    TWCR = (1 << TWINT) | (1 << TWEN);

    // Wait for TWINT flag
    while (!(TWCR & (1 << TWINT)));

    return TWDR;
}

/*
 * Write a byte to a register
 * Returns: I2C_OK on success, error code on failure
 */
uint8_t i2c_write_byte(uint8_t address, uint8_t reg, uint8_t data) {
    uint8_t status;

    // Start + device address (write mode)
    status = i2c_start(address << 1);
    if (status != I2C_OK) {
        i2c_stop();
        return status;
    }

    // Write register address
    status = i2c_write(reg);
    if (status != I2C_OK) {
        i2c_stop();
        return status;
    }

    // Write data
    status = i2c_write(data);
    if (status != I2C_OK) {
        i2c_stop();
        return status;
    }

    // Stop condition
    i2c_stop();

    return I2C_OK;
}

/*
 * Read a byte from a register
 * Returns: data byte (returns 0 on error)
 */
uint8_t i2c_read_byte(uint8_t address, uint8_t reg) {
    uint8_t data;
    uint8_t status;

    // Start + device address (write mode) to set register
    status = i2c_start(address << 1);
    if (status != I2C_OK) {
        i2c_stop();
        return 0;
    }

    // Write register address
    status = i2c_write(reg);
    if (status != I2C_OK) {
        i2c_stop();
        return 0;
    }

    // Repeated start + device address (read mode)
    status = i2c_start((address << 1) | 1);
    if (status != I2C_OK) {
        i2c_stop();
        return 0;
    }

    // Read data with NACK (last byte)
    data = i2c_read_nack();

    // Stop condition
    i2c_stop();

    return data;
}

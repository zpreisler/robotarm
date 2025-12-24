#ifndef UART_H
#define UART_H

#include <avr/io.h>
#include <stdint.h>

/*
 * UART (Serial) Communication Library for ATmega328p
 *
 * Hardware pins:
 * - TX: PD1 (Arduino Pin 1) - Transmit to computer
 * - RX: PD0 (Arduino Pin 0) - Receive from computer
 *
 * When connected via USB, the Arduino's USB-to-serial chip
 * translates between USB and UART automatically.
 */

/* Common baud rates */
#define UART_BAUD_9600    9600
#define UART_BAUD_19200   19200
#define UART_BAUD_38400   38400
#define UART_BAUD_57600   57600
#define UART_BAUD_115200  115200

/* Default baud rate */
#ifndef UART_BAUD
#define UART_BAUD UART_BAUD_9600
#endif

/* Function declarations */

/**
 * Initialize UART
 * @param baud Baud rate (e.g., 115200)
 */
void uart_init(uint32_t baud);

/**
 * Send a single character
 * @param c Character to send
 */
void uart_putc(char c);

/**
 * Send a string
 * @param str Null-terminated string
 */
void uart_puts(const char *str);

/**
 * Receive a single character (blocking)
 * Waits until a character is available
 * @return Received character
 */
char uart_getc(void);

/**
 * Check if data is available to read
 * @return 1 if data available, 0 otherwise
 */
uint8_t uart_available(void);

/**
 * Flush the receive buffer
 */
void uart_flush(void);

#endif /* UART_H */

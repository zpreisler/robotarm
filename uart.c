#include "uart.h"

/*
 * Initialize UART
 * Configures UART for 8 data bits, 1 stop bit, no parity
 */
void uart_init(uint32_t baud) {
    // Calculate baud rate register value
    // UBRR = (F_CPU / (16 * baud)) - 1
    uint16_t ubrr = (F_CPU / (16UL * baud)) - 1;

    // Set baud rate
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Enable receiver and transmitter
    UCSR0B = (1 << RXEN0) | (1 << TXEN0);

    // Set frame format: 8 data bits, 1 stop bit, no parity
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

/*
 * Send a single character
 * Waits until transmit buffer is ready
 */
void uart_putc(char c) {
    // Wait for empty transmit buffer
    while (!(UCSR0A & (1 << UDRE0)));

    // Put data into buffer, sends the data
    UDR0 = c;
}

/*
 * Send a null-terminated string
 */
void uart_puts(const char *str) {
    while (*str) {
        uart_putc(*str);
        str++;
    }
}

/*
 * Receive a single character (blocking)
 * Waits until data is available
 */
char uart_getc(void) {
    // Wait for data to be received
    while (!(UCSR0A & (1 << RXC0)));

    // Get and return received data from buffer
    return UDR0;
}

/*
 * Check if data is available in receive buffer
 */
uint8_t uart_available(void) {
    return (UCSR0A & (1 << RXC0)) ? 1 : 0;
}

/*
 * Flush the receive buffer
 * Reads and discards all waiting data
 */
void uart_flush(void) {
    uint8_t dummy;
    while (UCSR0A & (1 << RXC0)) {
        dummy = UDR0;
    }
    (void)dummy;  // Suppress unused variable warning
}

#include "uart.h"
#include <avr/interrupt.h>

/*
 * Circular buffer for interrupt-driven RX
 */
#define UART_RX_BUFFER_SIZE 64  // Must be power of 2
#define UART_RX_BUFFER_MASK (UART_RX_BUFFER_SIZE - 1)

static volatile uint8_t rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint8_t rx_head = 0;  // Write position (ISR writes here)
static volatile uint8_t rx_tail = 0;  // Read position (main code reads here)

/*
 * UART RX Complete Interrupt Handler
 * Fires automatically when a character is received
 * Reads character and stores in circular buffer
 */
ISR(USART_RX_vect) {
    uint8_t data = UDR0;  // Read data (clears interrupt flag)
    uint8_t next_head = (rx_head + 1) & UART_RX_BUFFER_MASK;

    // Store data if buffer not full
    if (next_head != rx_tail) {
        rx_buffer[rx_head] = data;
        rx_head = next_head;
    }
    // If buffer full, character is dropped (overflow)
}

/*
 * Initialize UART
 * Configures UART for 8 data bits, 1 stop bit, no parity
 * Enables RX interrupt for automatic character buffering
 */
void uart_init(uint32_t baud) {
    // Enable U2X (double-speed) mode for better baud rate accuracy
    // This is what Arduino Serial.begin() does for 115200 baud at 16 MHz
    UCSR0A = (1 << U2X0);

    // Calculate baud rate register value for U2X mode
    // UBRR = (F_CPU / (8 * baud)) - 1
    // For 115200 baud: UBRR = 16, actual = 117647 baud, error = 2.1%
    uint16_t ubrr = (F_CPU / (8UL * baud)) - 1;

    // Set baud rate
    UBRR0H = (uint8_t)(ubrr >> 8);
    UBRR0L = (uint8_t)ubrr;

    // Enable receiver, transmitter, and RX complete interrupt
    UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);

    // Set frame format: 8 data bits, 1 stop bit, no parity
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);

    // Enable global interrupts
    sei();
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
 * Automatically adds \r before \n for proper terminal display
 */
void uart_puts(const char *str) {
    while (*str) {
        if (*str == '\n') {
            uart_putc('\r');  // Send carriage return before newline
        }
        uart_putc(*str);
        str++;
    }
}

/*
 * Receive a single character (blocking)
 * Waits until data is available in buffer
 * Reads from interrupt-driven circular buffer
 */
char uart_getc(void) {
    // Wait for data in buffer
    while (rx_head == rx_tail);

    // Get data from buffer
    uint8_t data = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1) & UART_RX_BUFFER_MASK;

    return data;
}

/*
 * Check if data is available in receive buffer
 * Returns number of bytes available (0 if empty)
 */
uint8_t uart_available(void) {
    return (rx_head != rx_tail) ? 1 : 0;
}

/*
 * Flush the receive buffer
 * Discards all buffered data
 */
void uart_flush(void) {
    // Simply reset buffer pointers (ISR will continue filling from head)
    rx_tail = rx_head;
}

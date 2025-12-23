#define F_CPU 16000000UL  // 16 MHz clock speed

#include <avr/io.h>
#include <util/delay.h>

int main(void) {
    // Set PB5 (Arduino pin 13) as output
    // PB5 is the built-in LED on Arduino Uno
    DDRB |= (1 << PB5);

    while (1) {
        // Turn LED on
        PORTB |= (1 << PB5);
        _delay_ms(1000);  // Wait 1 second

        // Turn LED off
        PORTB &= ~(1 << PB5);
        _delay_ms(1000);  // Wait 1 second
    }

    return 0;
}

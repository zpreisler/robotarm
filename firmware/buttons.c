#include "buttons.h"

/*
 * Initialize ADC for button reading
 * - Uses ADC0 (analog pin A0)
 * - Sets reference voltage to AVCC (5V)
 * - Sets prescaler to 128 for 16MHz clock (125kHz ADC clock)
 */
void buttons_init(void) {
    // Set ADC reference to AVCC (bit REFS0 = 1)
    // Select ADC0 (MUX bits = 0000)
    ADMUX = (1 << REFS0);

    // Enable ADC and set prescaler to 128
    // ADC clock = 16MHz / 128 = 125kHz (within 50-200kHz recommended range)
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/*
 * Read raw ADC value from A0
 * Returns 10-bit value (0-1023)
 */
uint16_t buttons_read_adc(void) {
    // Start conversion
    ADCSRA |= (1 << ADSC);

    // Wait for conversion to complete
    while (ADCSRA & (1 << ADSC));

    // Return ADC value
    return ADC;
}

/*
 * Read button state
 * Converts ADC value to button identifier
 */
button_t buttons_read(void) {
    uint16_t adc_value = buttons_read_adc();

    // Determine which button is pressed based on ADC value
    if (adc_value >= BUTTON_RIGHT_MIN && adc_value < BUTTON_RIGHT_MAX) {
        return BUTTON_RIGHT;
    }
    else if (adc_value >= BUTTON_UP_MIN && adc_value < BUTTON_UP_MAX) {
        return BUTTON_UP;
    }
    else if (adc_value >= BUTTON_DOWN_MIN && adc_value < BUTTON_DOWN_MAX) {
        return BUTTON_DOWN;
    }
    else if (adc_value >= BUTTON_LEFT_MIN && adc_value < BUTTON_LEFT_MAX) {
        return BUTTON_LEFT;
    }
    else if (adc_value >= BUTTON_SELECT_MIN && adc_value < BUTTON_SELECT_MAX) {
        return BUTTON_SELECT;
    }

    return BUTTON_NONE;
}

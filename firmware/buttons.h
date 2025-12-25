#ifndef BUTTONS_H
#define BUTTONS_H

#include <avr/io.h>
#include <stdint.h>

/*
 * LCD Keyboard Shield Buttons
 * All buttons connected to analog pin A0 (ADC0) via resistor network
 * Each button produces a different voltage level
 */

/* Button definitions */
typedef enum {
    BUTTON_NONE = 0,
    BUTTON_RIGHT,
    BUTTON_UP,
    BUTTON_DOWN,
    BUTTON_LEFT,
    BUTTON_SELECT
} button_t;

/* ADC threshold values for button detection */
#define BUTTON_RIGHT_MIN   0
#define BUTTON_RIGHT_MAX   50
#define BUTTON_UP_MIN      50
#define BUTTON_UP_MAX      250
#define BUTTON_DOWN_MIN    250
#define BUTTON_DOWN_MAX    450
#define BUTTON_LEFT_MIN    450
#define BUTTON_LEFT_MAX    650
#define BUTTON_SELECT_MIN  650
#define BUTTON_SELECT_MAX  850

/* Function declarations */
void buttons_init(void);
uint16_t buttons_read_adc(void);
button_t buttons_read(void);

#endif /* BUTTONS_H */

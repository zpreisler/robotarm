#ifndef LCD_H
#define LCD_H

#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

/*
 * LCD Pin Configuration
 * Override these before including lcd.h if you need different pins
 *
 * Default configuration for LCD Keyboard Shield:
 * - RS: Pin 8 (PB0)
 * - Enable: Pin 9 (PB1)
 * - D4: Pin 4 (PD4)
 * - D5: Pin 5 (PD5)
 * - D6: Pin 6 (PD6)
 * - D7: Pin 7 (PD7)
 * - RW: Not used on most shields (tied to GND)
 */

#ifndef LCD_ENABLE_PORT
#define LCD_ENABLE_PORT PORTB
#define LCD_ENABLE_DDR DDRB
#define LCD_ENABLE_PIN 1
#endif

#ifndef LCD_RS_PORT
#define LCD_RS_PORT PORTB
#define LCD_RS_DDR DDRB
#define LCD_RS_PIN 0
#endif

#ifndef LCD_RW_PORT
#define LCD_RW_PORT PORTB
#define LCD_RW_DDR DDRB
#define LCD_RW_PIN 2
#endif

#ifndef LCD_DATA_PORT
#define LCD_DATA_PORT PORTD
#define LCD_DATA_DDR DDRD
#define LCD_DATA_PIN PIND
#define LCD_D4_PIN 4
#define LCD_D5_PIN 5
#define LCD_D6_PIN 6
#define LCD_D7_PIN 7
#endif

/* Pin manipulation macros */
#define LCD_ENABLE_HIGH LCD_ENABLE_PORT |= (1 << LCD_ENABLE_PIN)
#define LCD_ENABLE_LOW LCD_ENABLE_PORT &= ~(1 << LCD_ENABLE_PIN)

#define LCD_RS_HIGH LCD_RS_PORT |= (1 << LCD_RS_PIN)
#define LCD_RS_LOW LCD_RS_PORT &= ~(1 << LCD_RS_PIN)

#define LCD_RW_HIGH LCD_RW_PORT |= (1 << LCD_RW_PIN)
#define LCD_RW_LOW LCD_RW_PORT &= ~(1 << LCD_RW_PIN)

/* Data port manipulation - D4-D7 on PORTD bits 4-7 */
#define LCD_DATA_HIGH LCD_DATA_PORT |= 0b11110000
#define LCD_DATA_LOW LCD_DATA_PORT &= 0b00001111

/* Control pins - RS, Enable, RW on PORTB bits 0-2 */
#define LCD_CMD_HIGH LCD_ENABLE_PORT |= 0b00000111
#define LCD_CMD_LOW LCD_ENABLE_PORT &= 0b11111000

/* DDR configuration */
#define LCD_CTRL_DDR_OUTPUT LCD_ENABLE_DDR |= 0b00000111  // RS, Enable, RW as outputs
#define LCD_DATA_DDR_INPUT LCD_DATA_DDR &= 0b00001111     // D4-D7 as inputs
#define LCD_DATA_DDR_OUTPUT LCD_DATA_DDR |= 0b11110000    // D4-D7 as outputs

/* Function declarations */
void lcd_ddr_out(void);
void lcd_pulse(void);
void lcd_init_pulse(void);
void lcd_blk(void);
void lcd_4bit(void);
void lcd_function_set(void);
void lcd_display_on(void);
void lcd_display_off(void);
void lcd_clear(void);
void lcd_entry_mode(void);
void lcd_16x2(void);
void lcd_init(void);
void lcd_set_cursor(uint8_t address);
void lcd_print_char(const char *c);
void lcd_print(const char *c);

#endif /* LCD_H */

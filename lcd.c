#include "lcd.h"

inline void lcd_ddr_out(){
	LCD_CTRL_DDR_OUTPUT;
	LCD_DATA_DDR_OUTPUT;
	LCD_RW_LOW;  // Keep RW LOW for write-only mode
}

void lcd_pulse(){
	LCD_ENABLE_HIGH;
	_delay_us(1);
	LCD_ENABLE_LOW;
}

void lcd_init_pulse(){
	LCD_CMD_LOW;
	LCD_DATA_LOW;
	PORTD |= 0b00110000; // D4,D5 high
	lcd_pulse();
}

void lcd_blk(){
	// Use delay instead of busy flag checking
	// Most commands need ~37-50us, we use 50us to be safe
	_delay_us(50);
}

void lcd_4bit(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;
	PORTD |= 0b00100000; // D5 high
	lcd_pulse();
}

void lcd_function_set(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;

	PORTD |= 0b00100000; // D5 high
	lcd_pulse();

	LCD_DATA_LOW;
	PORTD |= 0b11000000; // D7,D6 high
	lcd_pulse();
}

void lcd_display_on(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;

	lcd_pulse();

	PORTD |= 0b11110000; // D7,D6,D5,D4 high
	lcd_pulse();
}

void lcd_display_off(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;

	lcd_pulse();

	PORTD |= 0b10000000; // D7 high
	lcd_pulse();
}

void lcd_clear(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;

	lcd_pulse();

	PORTD |= 0b00010000; // D4 high
	lcd_pulse();

	// Clear command needs longer delay (~1.52ms)
	_delay_ms(2);
}

void lcd_entry_mode(){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;

	lcd_pulse();

	PORTD |= 0b01100000; // D6,D5 high
	lcd_pulse();
}

void lcd_16x2(){ //Call after initialization sequence
	lcd_function_set();
	lcd_display_on();
	lcd_clear();
	lcd_entry_mode();
}

void lcd_init(){
	_delay_ms(15);	//wait > 15ms
	lcd_init_pulse();
	_delay_ms(5); //wait > 4.1ms
	lcd_init_pulse();
	_delay_us(100); //wait > 100us
	lcd_init_pulse();
	lcd_4bit();
	lcd_16x2();
}

void lcd_set_cursor(uint8_t address){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_DATA_LOW;
	PORTD |= 0b10000000 | (address & 0xF0);  // Set cursor command + upper nibble
	lcd_pulse();

	LCD_DATA_LOW;
	PORTD |= (address & 0x0F) << 4;  // Lower nibble
	lcd_pulse();
}

void lcd_print_char(const char *c){
	lcd_blk();
	LCD_CMD_LOW;
	LCD_RS_HIGH;

	LCD_DATA_LOW;
	PORTD |= ((uint8_t)*c & 0xF0);  // Upper nibble
	lcd_pulse();

	LCD_DATA_LOW;
	PORTD |= ((uint8_t)*c & 0x0F) << 4;  // Lower nibble
	lcd_pulse();
}

void lcd_print(const char *c){
	while(*c){
		lcd_print_char(c);
		c++;
	}
}

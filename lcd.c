#include "lcd.h"

void lcd_pulse_enable() {
	LCD_PORT |= (1 << EN);
	_delay_us(1);
	LCD_PORT &= ~(1 << EN);
	_delay_us(100);
}

void lcd_send_nibble(uint8_t nibble) {
	LCD_PORT = (LCD_PORT & 0x0F) | (nibble & 0xF0); // Send high nibble
	lcd_pulse_enable();
}

void lcd_command(uint8_t cmd) {
	LCD_PORT &= ~(1 << RS); // RS = 0 for command
	lcd_send_nibble(cmd & 0xF0);
	lcd_send_nibble(cmd << 4);
	_delay_ms(2);
}

void lcd_write_char(char data) {
	LCD_PORT |= (1 << RS); // RS = 1 for data
	lcd_send_nibble(data & 0xF0);
	lcd_send_nibble(data << 4);
	_delay_ms(2);
}

void lcd_init2() {
	LCD_DDR |= 0xFF; // Configure LCD port as output
	_delay_ms(20);   // Wait for LCD to power up

	// Initialization sequence
	lcd_command(0x02); // Initialize in 4-bit mode
	lcd_command(0x28); // Function set: 4-bit mode, 2 lines, 5x7 dots
	lcd_command(0x0C); // Display ON, cursor OFF
	lcd_command(0x06); // Entry mode: auto increment cursor
	lcd_clear();       // Clear display
}

void lcd_clear() {
	// lcd_command(0x01); // Clear display
	// _delay_ms(20);
	char clr_char = '\0';
	dis_cmd(0x80);
	// clear the first row
	for (int i = 0; i < 32; i++) {
		dis_data(clr_char);
	}
	dis_cmd(0xC0); // Row 1, Col 0
	// clear the second row
	for (int i = 0; i < 32; i++) {
		dis_data(clr_char);
	}
	dis_cmd(0x80);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
	uint8_t pos = (row == 0) ? 0x80 : 0xC0;
	lcd_command(pos + col);
}

void lcd_print(const char* str) {
	while (*str) {
		lcd_write_char(*str++);
	}
}

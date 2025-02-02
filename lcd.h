#ifndef LCD_H
#define LCD_H

#include <avr/io.h>
#include <util/delay.h>

// Define LCD pins and port
#define LCD_PORT PORTB
#define LCD_DDR  DDRB
#define RS PC0
#define EN PC1

// Function Prototypes
void lcd_init(void);
void lcd_command(uint8_t cmd);
void lcd_write_char(char data);
void lcd_clear(void);
void lcd_set_cursor(uint8_t row, uint8_t col);
void lcd_print(const char* str);

#endif

#define F_CPU 8000000UL // Set clock frequency to 8 MHz
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include "lcd.h"
#include "lcd.c"

struct Alarm {
	uint8_t alarm_hour;
	uint8_t alarm_minute;
	char alarm_message[20];
	int isFree;
};
typedef struct Alarm Alarm;

void init_timer();
void check_alarm();
char get_keypad_key();
void configure_alarm();
void buzzer_on();
void buzzer_off();

#define KEYPAD_PORT PORTC
#define KEYPAD_PIN  PINC
#define KEYPAD_DDR  DDRC
#define rs PB2
#define en PB3
#define min(a,b) (a < b ? a : b)
#define max(a,b) (a > b ? a : b)
#define DIGIT_PORT PORTA
#define SEGMENT_PORT PORTD
#define DIGIT_DDR DDRA
#define SEGMENT_DDR DDRD

const uint8_t segment_map[] = {
	0xC0, // 0
	0xF9, // 1
	0xA4, // 2
	0xB0, // 3
	0x99, // 4
	0x92, // 5
	0x82, // 6
	0xF8, // 7
	0x80, // 8
	0x90, // 9
};

void init_7seg() {
	DIGIT_DDR = 0xFF; 
	SEGMENT_DDR = 0xFF; 
	DIGIT_PORT = 0x00;
	SEGMENT_PORT = 0x00;
}

void display_digit(uint8_t digit, uint8_t value, uint8_t dp) {
	DIGIT_PORT = (1 << digit);
	SEGMENT_PORT = segment_map[value] & (dp ? 0b01111111 : 0xFF); //(dp && (value % 2 == 0) ? 0b10000000 : 0); // Send segment data
	_delay_ms(5); 
	// DIGIT_PORT = 0xFF;
}


volatile uint8_t seconds = 0, minutes = 0, hours = 0;
Alarm alarms[4]; // 4 alarms maximum, if got deleted, set isFree true
Alarm EEMEM eeprom_alarms[4]; // 4 alarms in EEPROM
volatile uint8_t alarm_active = 0;

void save_alarms_to_eeprom() {
	eeprom_update_block((const void*)alarms, (void*)eeprom_alarms, sizeof(alarms));
}

void load_alarms_from_eeprom() {
	eeprom_read_block((void*)alarms, (const void*)eeprom_alarms, sizeof(alarms));
}

void lcd_init();
void dis_cmd(char);
void dis_data(char);
void lcdcmd(char);
void lcddata(char);

void dis_msg(char *message) {
	for (int i = 0; message[i] != '\0'; i++) {
		dis_data(message[i]);
		_delay_ms(50);
	}
	//_delay_ms(50);
}

void dis_msg_inst(char *message) {
	for (int i = 0; message[i] != '\0'; i++) {
		dis_data(message[i]);
	}
}

void init_keypad() {
	KEYPAD_DDR = 0x0F;
	KEYPAD_PORT = 0xFF;
}

void lcd_init()	
{
	dis_cmd(0x02);		
	dis_cmd(0x28);
	//dis_cmd(0x01);	                  
	dis_cmd(0x0C);
	dis_cmd(0x06);
	dis_cmd(0x83);
}

void dis_cmd(char cmd_value)
{
	char cmd_value1;
	
	cmd_value1 = cmd_value & 0xF0;		//mask lower nibble because PA4-PA7 pins are used.
	lcdcmd(cmd_value1);			// send to LCD
	
	cmd_value1 = ((cmd_value<<4) & 0xF0);	//shift 4-bit and mask
	lcdcmd(cmd_value1);			// send to LCD
}


void dis_data(char data_value)
{
	char data_value1;
	
	data_value1=data_value&0xF0;
	lcddata(data_value1);
	
	data_value1=((data_value<<4)&0xF0);
	lcddata(data_value1);
}

void lcdcmd(char cmdout)
{
	PORTB=cmdout;
	PORTB&=~(1<<rs);
	//PORTB&=~(1<<rw);
	PORTB|=(1<<en);
	_delay_ms(1);
	PORTB&=~(1<<en);
}

void lcddata(char dataout)
{
	PORTB=dataout;
	PORTB|=(1<<rs);
	//PORTB&=~(1<<rw);
	PORTB|=(1<<en);
	_delay_ms(1);
	PORTB&=~(1<<en);
}

void init_timer() {
	TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
	OCR1A = 7812; 
	TIMSK |= (1 << OCIE1A); 
}

ISR(TIMER1_COMPA_vect) {
	seconds++;
	if (seconds >= 60) {
		seconds = 0;
		minutes++;
		if (minutes >= 60) {
			minutes = 0;
			hours = (hours + 1) % 24;
		}
	}
}

void display_alarms() {
	char cmd;
	while ((cmd = get_keypad_key()) != 'D' ) {
		if (cmd >= '1' && cmd <= '4') {
			alarms[cmd - '1'].isFree = 1;
			lcd_clear();
			dis_msg("Alarm ");
			dis_data(cmd);
			dis_msg(" deleted");
			save_alarms_to_eeprom();
			_delay_ms(1000);
		}
		//lcd_clear();
		dis_cmd(0X80); // move cursor to start
		dis_msg_inst(" ");
		if (alarms[0].isFree) {
			dis_msg_inst("EMPTY");
			} else {
			dis_data('0' + alarms[0].alarm_hour / 10);
			dis_data('0' + alarms[0].alarm_hour % 10);
			dis_data(':');
			dis_data('0' + alarms[0].alarm_minute / 10);
			dis_data('0' + alarms[0].alarm_minute % 10);
		}
		
		dis_msg_inst("    ");
		
		if (alarms[1].isFree) {
			dis_msg_inst("EMPTY");
			} else {
			dis_data('0' + alarms[1].alarm_hour / 10);
			dis_data('0' + alarms[1].alarm_hour % 10);
			dis_data(':');
			dis_data('0' + alarms[1].alarm_minute / 10);
			dis_data('0' + alarms[1].alarm_minute % 10);
		}
		
		dis_cmd(0XC0);
		dis_msg_inst(" ");
		
		if (alarms[2].isFree) {
			dis_msg_inst("EMPTY");
			} else {
			dis_data('0' + alarms[2].alarm_hour / 10);
			dis_data('0' + alarms[2].alarm_hour % 10);
			dis_data(':');
			dis_data('0' + alarms[2].alarm_minute / 10);
			dis_data('0' + alarms[2].alarm_minute % 10);
		}
		
		dis_msg_inst("    ");
		
		if (alarms[3].isFree) {
			dis_msg_inst("EMPTY");
			} else {
			dis_data('0' + alarms[3].alarm_hour / 10);
			dis_data('0' + alarms[3].alarm_hour % 10);
			dis_data(':');
			dis_data('0' + alarms[3].alarm_minute / 10);
			dis_data('0' + alarms[3].alarm_minute % 10);
		}
		
		_delay_ms(100);
	}
	lcd_clear();
	dis_msg("");
}

void create_custom_char(uint8_t location, uint8_t charmap[]) {
	location &= 0x07;
	dis_cmd(0x40 | (location << 3));
	for (uint8_t i = 0; i < 8; i++) {
		dis_data(charmap[i]); 
	}
}

void display_filled_block(uint8_t row, uint8_t col) {
	dis_cmd(0x80 | (row * 0x40 + col)); 
	dis_data(0xFF);
}

void display_empty_block(uint8_t row, uint8_t col) {
	dis_cmd(0x80 | (row * 0x40 + col));
	dis_data(0x00);
}


int main(void) {
    DDRC = 0xF0; 
    DDRD = 0xFF; 
    PORTD = 0xFF;
	
	int row = 0;
	int col = 0;
	int mode = 0; // 0: write, 1: clear

    DDRB = 0xFF;
    lcd_init(); 
	init_timer();
	init_keypad();
	init_7seg();
	load_alarms_from_eeprom();
    sei();
	
	char *welcome_message = "";
	char time_str[9];
	dis_cmd(0x80); // set cursor to (0, 0)
	dis_msg(welcome_message);

    while (1) {
		char key = get_keypad_key();
		
        if (key == 'A') {
			DIGIT_PORT = 0;
            configure_alarm(); 
        } else if (key == 'B') {
			DIGIT_PORT = 0;
			display_alarms();
        } else if (key == 'C') {
			DIGIT_PORT = 0;
            // set clock
			lcd_clear();
			dis_msg("Enter Time:");
			
			// Set hours
			dis_cmd(0xC0); // row 1, col 0
			_delay_ms(50); // just to make it smooth
			dis_data('H');
			dis_data(':');
			char key = 0;
			while ((key = get_keypad_key()) == 0 || key - '0' > 9); // Wait for first digit
			hours = (key - '0') * 10;
			dis_data(key);
			while ((key = get_keypad_key()) == 0 || key - '0' > 9 || (hours + (key - '0') >= 24)); // Wait for second digit
			hours += (key - '0');
			dis_data(key);

			// Set minutes
			// dis_cmd(0x80);
			dis_data(' ');
			
			_delay_ms(50); // just to make it smooth
			dis_data('M');
			dis_data(':');
			while ((key = get_keypad_key()) == 0 || key - '0' > 9); // Wait for first digit
			minutes = (key - '0') * 10;
			dis_data(key);
			while ((key = get_keypad_key()) == 0 || key - '0' > 9 || (minutes + (key - '0')) >= 60); // Wait for second digit
			minutes += (key - '0');
			dis_data(key);
			
			lcd_clear();
			dis_cmd(0XC0);
			_delay_ms(50); // just to make it smooth

            //lcd_clear();
			// _delay_ms(10);
			// dis_cmd(0x80);
            //char *message = "Reset";
			//for (int i = 0; message[i] != '\0'; i++) {
			//	dis_data(message[i]);
			//}
        } else {
			check_alarm();
			//sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);
			//dis_cmd(0XC0);
			//for (int i = 0; time_str[i] != '\0'; i++) {
				//dis_data(time_str[i]);
			//}
			uint8_t digits[6];
			digits[0] = hours / 10;
			digits[1] = hours % 10; 
			digits[2] = minutes / 10; 
			digits[3] = minutes % 10; 
			digits[4] = seconds / 10; 
			digits[5] = seconds % 10; 
			
			if (mode == 0) {
				display_filled_block(row, col);
				col += 2;
				if (col > 15) {
					col = 0;
					row++;
				}
				if (row > 1) {
					mode = 1;
					row = 0;
					col = 0;
				}
			}
			if (mode == 1) {
				display_empty_block(row, col);
				col += 2;
				if (col >= 15) {
					col = 0;
					row++;
				}
				if (row > 1) {
					mode = 0;
					row = 0;
					col = 0;
				}
			}
			
			_delay_ms(5);
			

			for (uint8_t i = 0; i < 6; i++) {
				display_digit(i, digits[i], (i == 1 || i == 3) && digits[5] % 2 == 0); // Enable DP at 1st and 3rd separators
			}
		}
    }
}

void configure_alarm() {
	int freeAlarm = -1;
	for (int i = 0; i < 4; i++) {
		if (alarms[i].isFree) {
			freeAlarm = i;
			break;
		}
	}
	if (freeAlarm == -1) {
		lcd_clear();
		dis_msg("Alarms Full");
		_delay_ms(2000);
		lcd_clear();
		return;
	}
	
	lcd_clear();
	dis_cmd(0x80);
	char *message = "Set Alarm";
	for (int i = 0; message[i] != '\0'; i++) {
		dis_data(message[i]);
		_delay_ms(50);
	}

	_delay_ms(200);

	dis_cmd(0xC0); 
	dis_data('H');
	dis_data(':');
	char key = 0;
	while ((key = get_keypad_key()) == 0 || key - '0' > 9);
	alarms[freeAlarm].alarm_hour = (key - '0') * 10;
	dis_data(key);
	while ((key = get_keypad_key()) == 0 || key - '0' > 9);
	alarms[freeAlarm].alarm_hour += (key - '0');
	dis_data(key);

	// Set minutes
	// dis_cmd(0x80);
	dis_data(' ');
	
	dis_data('M');
	dis_data(':');
	while ((key = get_keypad_key()) == 0); // first digit
	alarms[freeAlarm].alarm_minute = (key - '0') * 10;
	dis_data(key);
	while ((key = get_keypad_key()) == 0);
	alarms[freeAlarm].alarm_minute += (key - '0');
	dis_data(key);
	
	// take the alarm message from user
	
	//dis_cmd(0X80); // move cursor to (0, 0)
	lcd_clear();
	dis_msg("write a message");
	dis_cmd(0XC0);
	dis_msg("confirm: +");
	while(get_keypad_key() != 'D');
	lcd_clear();
	
	char alarm_message[20];
	//dis_cmd(0XC0); // move cursor to (0, 1)
	char prev_char = get_keypad_key();
	for (int i = 0; i < 20; i++) {
		while ((alarm_message[i] = get_keypad_key()) == prev_char);
		if (alarm_message[i] == 'D') {
			alarms[freeAlarm].alarm_message[i] = '\0';
			break;
		}
		// char only mode
		// use the num mode as an option to exit this mode?
		switch (alarm_message[i]) {
			case '7':
			alarms[freeAlarm].alarm_message[i] = 'a';
			break;
			
			case '8':
			alarms[freeAlarm].alarm_message[i] = 'b';
			break;
			
			case '9':
			alarms[freeAlarm].alarm_message[i] = 'c';
			break;
			
			case '4':
			alarms[freeAlarm].alarm_message[i] = 'd';
			break;
			
			case '5':
			alarms[freeAlarm].alarm_message[i] = 'e';
			break;
			
			case '6':
			alarms[freeAlarm].alarm_message[i] = 'f';
			break;
			
			case '1':
			alarms[freeAlarm].alarm_message[i] = 'g';
			break;
			
			case '2':
			alarms[freeAlarm].alarm_message[i] = 'h';
			break;
			
			case '3':
			alarms[freeAlarm].alarm_message[i] = 'i';
			break;
			
			case '*':
			alarms[freeAlarm].alarm_message[i] = 'j';
			break;
			
			case '0':
			alarms[freeAlarm].alarm_message[i] = 'k';
			break;
			
			case '#':
			alarms[freeAlarm].alarm_message[i] = 'l';
			break;
			
			case 'A':
			alarms[freeAlarm].alarm_message[i] = '.';
			break;
			
			case 'B':
			alarms[freeAlarm].alarm_message[i] = '?';
			break;
			
			case 'C':
			//alarms[freeAlarm].alarm_message[i] = '!'; // use as backspace
			i = max(i - 2, -1);
			lcd_clear();
			//dis_msg_inst("Message: ");
			for (int j = 0; j <= min(i, 15); j++) {
				dis_data(alarms[freeAlarm].alarm_message[j]);
			}
			dis_cmd(0XC0);
			for (int j = 16; j <= i; j++) {
				dis_data(alarms[freeAlarm].alarm_message[j]);
			}
			dis_cmd(0X80);
			// continue;
			break;
		}
		if (i == 16) {
			dis_cmd(0XC0);
		}
		dis_data(alarms[freeAlarm].alarm_message[i]);
	}

	alarm_active = 1;
	lcd_clear();
	dis_cmd(0x80);
	char *end_message = "Alarm Set";
	dis_msg(end_message);
	alarms[freeAlarm].isFree = 0;
	save_alarms_to_eeprom();
	_delay_ms(1000);
	lcd_clear();
	dis_msg("");
}

void check_alarm() {
	for (int i = 0; i < 4; i++) { // change the last condition back to &&
		if (!alarms[i].isFree && hours == alarms[i].alarm_hour && minutes == alarms[i].alarm_minute && seconds == 0) { // && seconds == 0
			//for (int i = 0; i < 150; i++) {
			//	lcd_clear();
			//	_delay_ms(80); // each cycle takes 2 seconds, 
								// 1 second showing the message,
								// 0.8 seconds clear screen, and
								// 0.2 seconds loading the message on the screen
				
			//	dis_cmd(0x80);
			//	dis_msg("ALARM");
			//	_delay_ms(100);				
			//}
			
			lcd_clear();
			// dis_msg("ALARM"); // replace with the message
			//for (int j = 0; j < 16 && alarms[i].alarm_message[j] != '\0'; j++) {
				//dis_data(alarms[i].alarm_message[j]);
			//}
			dis_msg(alarms[i].alarm_message);
			dis_cmd(0XC0);
			//for (int j = 16; j < 32 && alarms[i].alarm_message[j] != '\0'; j++) {
				//dis_data(alarms[i].alarm_message[j]);
			//}
			while (seconds != 30) {
				DIGIT_PORT = 0;
				buzzer_on();
				char cmd = get_keypad_key();
				if (cmd == 'B') {
					alarms[i].alarm_minute += 2;
					if (alarms[i].alarm_minute >= 60) {
						alarms[i].alarm_minute -= 60;
						alarms[i].alarm_hour++;
					}
					lcd_clear();
					buzzer_off();
					return;
				} else if (cmd == 'A') { // turn off the alarm
					lcd_clear();
					dis_msg("Turned Off");
					_delay_ms(2000);
					break;
				}
			}
			buzzer_off();
			// alarms[i].isFree = 1; // free the alarm space
			lcd_clear();
		}
	}
}

void buzzer_on() {
	PORTB |= (1 << 0);
}

void buzzer_off() {
	PORTB &= ~(1 << PB0);
}

char get_keypad_key() {
	const char keys[4][4] = {
		{'7', '8', '9', 'A'},
		{'4', '5', '6', 'B'},
		{'1', '2', '3', 'C'},
		{'*', '0', '#', 'D'}
	};
    for (uint8_t row = 0; row < 4; row++) {
        KEYPAD_PORT = ~(1 << row);
        _delay_ms(1);

        for (uint8_t col = 0; col < 4; col++) {
            if (!(KEYPAD_PIN & (1 << (col + 4)))) {
                _delay_ms(50);
                while (!(KEYPAD_PIN & (1 << (col + 4))));
                return keys[row][col];
            }
        }
    }
    return 0;
}

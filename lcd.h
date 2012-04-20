// Display code for controlling a PCD8544 (Nokia 3300) display using avr
// by Henning Meyer 
// based on Arduino Tutorial http://www.arduino.cc/playground/Code/PCD8544

/*
        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef LCD_H
#define LCD_H


void LcdLEDon(void);
void LcdLEDoff(void);
inline void gotoXY(int x, int y);
void LcdCharacter(uint8_t c);
void LcdString(char *s);
void LcdNumber(uint16_t num);
void LcdFillLine(void);

#define LCD_X     84
#define LCD_Y     48

#define HIGH 1
#define LOW 0

#define LCD_C     LOW
#define LCD_D     HIGH

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/interrupt.h>	// interrupt service routines
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>			// some handy functions like utoa()
#include "character.h"

void LcdWrite(uint8_t dc, uint8_t data);

inline void LcdByte(uint8_t b) {
	LcdWrite(LCD_D, b);
}

inline void LcdSpace(void) {
	LcdWrite(LCD_D, 0x00);
}

inline void LcdSv(uint8_t pref) {
	LcdString("sv");
	if (pref == 12)
		LcdCharacter('p');
	else if (pref == 9)
		LcdCharacter('n');
	else if (pref == 6)
		LcdCharacter('u');
	else if (pref == 3)
		LcdCharacter('m');
	LcdCharacter(' ');
}

inline void LcdSvH(uint8_t pref) {
	LcdString("/h");
	LcdSv( pref );
}


// LCD IS CONNECTED TO PORT B

#define PIN_SCE   _BV(PB7)
#define PIN_RESET _BV(PB6)
#define PIN_DC    _BV(PB5)
#define PIN_SDIN  _BV(PB0)
#define PIN_SCLK  _BV(PB1)
//#define PIN_LED   _BV(PB3)


#define LCD_CMD   0

#define byte uint8_t
#define HIGH 1
#define LOW 0


volatile uint8_t col = 0;


inline void digitalWrite(byte pin, byte v) {
	if (v) PORTB |= pin;
	else PORTB &= ~pin;
}

inline void shiftOut(uint8_t dataPin, uint8_t clockPin,/* uint8_t bitOrder,*/ uint8_t val) {
        uint8_t i;
        for (i = 0; i < 8; i++)  {
//               if (bitOrder == LSBFIRST)
//                       digitalWrite(dataPin, !!(val & (1 << i)));
//                else    
                        digitalWrite(dataPin, !!(val & (1 << (7 - i))));
                        
                digitalWrite(clockPin, HIGH);
                digitalWrite(clockPin, LOW);            
        }
}


inline void LcdWrite(byte dc, byte data) {
	digitalWrite(PIN_DC, dc);
	digitalWrite(PIN_SCE, LOW);
	shiftOut(PIN_SDIN, PIN_SCLK,/* MSBFIRST,*/ data);
	digitalWrite(PIN_SCE, HIGH);
	if (dc == LCD_D) col++;
}



inline void LcdClear(void)
{
	int index;
	for (index = 0; index < LCD_X * LCD_Y / 8; index++)
		LcdSpace();
}


inline LcdCharacter(uint8_t c)
{
	byte index;
	if (c>='0' && c<='9') c -= '0';
	else switch (c) {
		case 'c': c = 10; break;
		case 'p': c = 11; break;
		case 'm': c = 12; break;
		case 's': c = 13; break;
		case 'i': c = 14; break;
		case 'f': c = 15; break;
		case 'v': c = 16; break;
		case 'u': c = 17; break;
		case 'n': c = 18; break;
		case 'k': c = 19; break;
		case 'h': c = 20; break;
		case '/': c = 21; break;
		case ':': c = 22; break;
		case ' ': c = 23; break;
	}
	uint16_t d = pgm_read_word(nums + c);
	LcdSpace();
	for (index = 0; index < 3; index++) {
		LcdWrite(LCD_D, d & B8(11111));
		d >>= 5;
	}
}

void LcdString(char *s) {
	if (!(*s)) return;
	char *t = s;
	while(*t!=0) t++;
	do { t--;LcdCharacter(*t); } while (t!=s);
}




void LcdNumber(uint16_t num)
{
	if (num == 0) 
		LcdCharacter('0');
	while (num>0) {
		LcdCharacter('0' + (num%10));
		num /= 10;
	}
}

void LcdFillLine(void) {
	while(col<LCD_X)
		LcdSpace();
}


inline void gotoXY(int x, int y)
{
	LcdWrite( 0, 0x80 | x);  // Column.
	col = x;
	LcdWrite( 0, 0x40 | y);  // Row.  
}


inline void LcdInitialise(void) {
	DDRB |= PIN_SCE  // set LCD pins as OUTPUT
		| PIN_RESET
		| PIN_DC
		| PIN_SDIN
		| PIN_SCLK
		/*| PIN_LED*/;
	digitalWrite(PIN_RESET, LOW);
	digitalWrite(PIN_RESET, HIGH);
//	digitalWrite(PIN_LED, LOW);

	LcdWrite( LCD_CMD, 0x21 );  // LCD Extended Commands.
	LcdWrite( LCD_CMD, 0xBf );  // Set LCD Vop (Contrast). //B1
	LcdWrite( LCD_CMD, 0x04 );  // Set Temp coefficent. //0x04
	LcdWrite( LCD_CMD, 0x14 );  // LCD bias mode 1:48. //0x13
	LcdWrite( LCD_CMD, 0x0C );  // LCD in normal mode. 0x0d for inverse
	LcdWrite(LCD_C, 0x20);
	LcdWrite(LCD_C, 0x0C);

	LcdClear();
}

void LcdLEDon(void) {
//	digitalWrite( PIN_LED, HIGH);
}

void LcdLEDoff(void) {
//	digitalWrite( PIN_LED, LOW);
}

#endif

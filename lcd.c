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

#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/interrupt.h>	// interrupt service routines
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>			// some handy functions like utoa()
#include "lcd.h"

// LCD IS CONNECTED TO PORT B

#define PIN_SCE   _BV(PB7)
#define PIN_RESET _BV(PB6)
#define PIN_DC    _BV(PB5)
#define PIN_SDIN  _BV(PB0)
#define PIN_SCLK  _BV(PB1)
//#define PIN_LED   _BV(PB3)

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_CMD   0

#define byte uint8_t
#define HIGH 1
#define LOW 0

#define HEX__(n) 0x##n##LU

/* 8-bit conversion function */
#define B8__(x) ((x&0x0000000FLU)?1:0) \
+((x&0x000000F0LU)?2:0) \
+((x&0x00000F00LU)?4:0) \
+((x&0x0000F000LU)?8:0) \
+((x&0x000F0000LU)?16:0) \
+((x&0x00F00000LU)?32:0) \
+((x&0x0F000000LU)?64:0) \
+((x&0xF0000000LU)?128:0)

/* *** user macros *** */

/* for upto 8-bit binary constants */
#define B8(d) ((unsigned char)B8__(HEX__(d)))

#define character(a,b,c,d,e) ((uint16_t)(\
	(a&4)<<12 | (b&4)<<11 | (c&4)<<10 | (d&4)<<9 | (e&4)<<8 |\
	(a&2)<< 8 | (b&2)<< 7 | (c&2)<< 6 | (d&2)<<5 | (e&2)<<4 |\
	(a&1)<< 4 | (b&1)<< 3 | (c&1)<< 2 | (d&1)<<1 | (e&1)))

static const uint16_t c1 = character(
	B8(110),
	B8(010),
	B8(010),
	B8(010),
	B8(010));
	

static const uint16_t nums[] = {
	character( // 0
		B8(110),
		B8(101),
		B8(101),
		B8(101),
		B8(111)),
	character( // 1
		B8(110),
		B8(010),
		B8(010),
		B8(010),
		B8(010)),
	character( // 2
		B8(110),
		B8(001),
		B8(111),
		B8(100),
		B8(111)),
	character( // 3
		B8(110),
		B8(001),
		B8(111),
		B8(001),
		B8(111)),
	character( // 4
		B8(100),
		B8(101),
		B8(111),
		B8(001),
		B8(001)),
	character( // 5
		B8(111),
		B8(100),
		B8(111),
		B8(001),
		B8(110)),
	character( // 6
		B8(111),
		B8(100),
		B8(111),
		B8(101),
		B8(011)),
	character( // 7
		B8(111),
		B8(001),
		B8(010),
		B8(100),
		B8(100)),
	character( // 8
		B8(110),
		B8(101),
		B8(111),
		B8(101),
		B8(111)),
	character( // 9
		B8(110),
		B8(101),
		B8(111),
		B8(001),
		B8(111)),
	character( // c - 10
		B8(011),
		B8(100),
		B8(100),
		B8(100),
		B8(111)),
	character( // p - 11
		B8(110),
		B8(101),
		B8(111),
		B8(100),
		B8(100)),
	character( // m - 12
		B8(111),
		B8(111),
		B8(111),
		B8(101),
		B8(101)),
	character( // s -13
		B8(011),
		B8(100),
		B8(111),
		B8(001),
		B8(111)),
	character( // i - 14
		B8(010),
		B8(000),
		B8(010),
		B8(010),
		B8(010)),
	character( // f - 15
		B8(011),
		B8(100),
		B8(111),
		B8(100),
		B8(100)),
	character( // , - 16
		B8(000),
		B8(000),
		B8(000),
		B8(010),
		B8(010))
};

volatile uint8_t col = 0;


void digitalWrite(byte pin, byte v) {
	if (v) PORTB |= pin;
	else PORTB &= ~pin;
}

void shiftOut(uint8_t dataPin, uint8_t clockPin,/* uint8_t bitOrder,*/ uint8_t val) {
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


void LcdWrite(byte dc, byte data) {
	digitalWrite(PIN_DC, dc);
	digitalWrite(PIN_SCE, LOW);
	shiftOut(PIN_SDIN, PIN_SCLK,/* MSBFIRST,*/ data);
	digitalWrite(PIN_SCE, HIGH);
	if (dc == LCD_D) col++;
}

void LcdSpace(void) {
	LcdWrite(LCD_D, 0x00);
}

void LcdByte(uint8_t b) {
	LcdWrite(LCD_D, b);
}

void LcdClear(void)
{
	int index;
	for (index = 0; index < LCD_X * LCD_Y / 8; index++)
		LcdSpace();
}


void LcdCharacter(uint8_t c)
{
	byte index;
	uint16_t d = nums[c];
	LcdSpace();
	for (index = 0; index < 3; index++) {
		LcdWrite(LCD_D, d & B8(11111));
		d >>= 5;
	}
}

void LcdCPS(void) {
	LcdCharacter(13); // s
	LcdCharacter(11); // p
	LcdCharacter(10); // c
	LcdSpace();
	LcdSpace();
}

void LcdCPM(char mode) {
	if (mode == 'i')
		LcdCharacter(14);
	else if (mode == 'f')
		LcdCharacter(15);
	else // mode = s
		LcdCharacter(13);
	LcdSpace();
	LcdSpace();
		
	LcdCharacter(12); // m
	LcdCharacter(11); // p
	LcdCharacter(10); // c
	LcdSpace();
	LcdSpace();
}

void LcdNumber(uint16_t num)
{
	if (num == 0) 
		LcdCharacter(0);
	while (num>0) {
		LcdCharacter(num%10);
		num /= 10;
	}
}

void LcdFillLine(void) {
	while(col<LCD_X)
		LcdSpace();
}


void gotoXY(int x, int y)
{
	LcdWrite( 0, 0x80 | x);  // Column.
	col = x;
	LcdWrite( 0, 0x40 | y);  // Row.  
}


void LcdInitialise(void) {
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


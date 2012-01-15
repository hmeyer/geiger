#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/interrupt.h>	// interrupt service routines
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>			// some handy functions like utoa()

// LCD IS CONNECTED TO PORT B

#define PIN_SCE   _BV(PB7)
#define PIN_RESET _BV(PB6)
#define PIN_DC    _BV(PB5)
#define PIN_SDIN  _BV(PB0)
#define PIN_SCLK  _BV(PB1)
//#define PIN_LED   _BV(PB3)

#define LCD_C     LOW
#define LCD_D     HIGH

#define LCD_X     84
#define LCD_Y     48
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
	character( // c
		B8(011),
		B8(100),
		B8(100),
		B8(100),
		B8(111)),
	character( // p
		B8(110),
		B8(101),
		B8(111),
		B8(100),
		B8(100)),
	character( // m
		B8(111),
		B8(111),
		B8(111),
		B8(101),
		B8(101)),
	character( // s
		B8(011),
		B8(100),
		B8(111),
		B8(001),
		B8(111))
};

static uint8_t col = 0;


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

void LcdClear(void)
{
	int index;
	for (index = 0; index < LCD_X * LCD_Y / 8; index++)
		LcdWrite(LCD_D, 0x00);
}

void LcdDigit(byte dig)
{
	byte index;
	uint16_t d = nums[dig];
	LcdWrite(LCD_D, 0x00);
	for (index = 0; index < 5; index++) {
		LcdWrite(LCD_D, d & B8(11111));
		d >>= 5;
	}
}

void LcdCPS(void) {
	LcdDigit(13); // s
	LcdDigit(11); // p
	LcdDigit(10); // c
	LcdWrite(LCD_D, 0x00);
	LcdWrite(LCD_D, 0x00);
}

void LcdCPM(void) {
	LcdDigit(12); // m
	LcdDigit(11); // p
	LcdDigit(10); // c
	LcdWrite(LCD_D, 0x00);
	LcdWrite(LCD_D, 0x00);
}

void LcdNumber(uint32_t num)
{
	while (num>0) {
		LcdDigit(num%10);
		num /= 10;
	}
}

void LcdFillLine(void) {
	while(col<LCD_X)
		LcdWrite(LCD_D, 0x00);
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

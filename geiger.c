/*
	Title: Geiger Counter with Serial Data Reporting
	Description: This is the firmware for the mightyohm.com Geiger Counter.
		There is more information at http://mightyohm.com/geiger 
		
	Author:		Jeff Keyzer
	Company:	MightyOhm Engineering
	Website:	http://mightyohm.com/
	Contact:	jeff <at> mightyohm.com
  
	This firmware controls the ATtiny2313 AVR microcontroller on board the Geiger Counter kit.
	
	When an impulse from the GM tube is detected, the firmware flashes the LED and produces a short
	beep on the piezo speaker.  It also outputs an active-high pulse (default 100us) on the PULSE pin.
	
	A pushbutton on the PCB can be used to mute the beep.
	
	A running average of the detected counts per second (CPS), counts per minute (CPM), and equivalent dose
	(uSv/hr) is output on the serial port once per second. The dose is based on information collected from 
	the web, and may not be accurate.
	
	The serial port is configured for BAUD baud, 8-N-1 (default 9600).
	
	The data is reported in comma separated value (CSV) format:
	CPS, #####, CPM, #####, uSv/hr, ###.##, SLOW|FAST|INST
	
	There are three modes.  Normally, the sample period is LONG_PERIOD (default 60 seconds). This is SLOW averaging mode.
	If the last five measured counts exceed a preset threshold, the sample period switches to SHORT_PERIOD seconds (default 5 seconds).
	This is FAST mode, and is more responsive but less accurate. Finally, if CPS > 255, we report CPS*60 and switch to
	INST mode, since we can't store data in the (8-bit) sample buffer.  This behavior could be customized to suit a particular
	logging application.
	
	The largest CPS value that can be displayed is 65535, but the largest value that can be stored in the sample buffer is 255.
	
	***** WARNING *****
	This Geiger Counter kit is for EDUCATIONAL PURPOSES ONLY.  Don't even think about using it to monitor radiation in
	life-threatening situations, or in any environment where you may expose yourself to dangerous levels of radiation.
	Don't rely on the collected data to be an accurate measure of radiation exposure! Be safe!
	
	
	Change log:
	8/4/11 1.00: Initial release for Chaos Camp 2011!


		Copyright 2011 Jeff Keyzer, MightyOhm Engineering
 
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

// Includes
#include <avr/io.h>			// this contains the AVR IO port definitions
#include <avr/interrupt.h>	// interrupt service routines
#include <avr/pgmspace.h>	// tools used to store variables in program memory
#include <avr/sleep.h>		// sleep mode utilities
#include <util/delay.h>		// some convenient delay functions
#include <stdlib.h>			// some handy functions like utoa()

#include "lcd.h"

// Defines
#define VERSION			"1.00"
#define URL				"http://mightyohm.com/geiger"

#define	F_CPU			8000000	// AVR clock speed in Hz
#define THRESHOLD		1000	// CPM threshold for fast avg mode
#define LONG_PERIOD		30	// # of samples to keep in memory in slow avg mode
#define SHORT_PERIOD	5		// # or samples for fast avg mode
#define SCALE_FACTOR	57		//	CPM to uSv/hr conversion factor (x10,000 to avoid float)


void checkevent(void);	// flash LED and beep the piezo
void sleep1s(void);  // sleep using Timer1 Interupt

// Global variables
volatile uint8_t nobeep = 0;		// flag used to mute beeper
volatile uint16_t count = 0;		// number of GM events that has occurred
volatile uint16_t slowcsum = 0;		// GM counts per minute in slow mode
volatile uint16_t fastcsum = 0;		// GM counts per SHORT_PEROID
volatile uint16_t cps = 0;			// GM counts per second, updated once a second
volatile uint8_t overflow = 0;		// overflow flag

volatile uint8_t buffer[LONG_PERIOD];	// the sample buffer

volatile uint8_t eventflag;	// flag for ISR to tell main loop if a GM event has occurred
volatile uint8_t tick;		// flag that tells main() when 1 second has passed
volatile uint8_t idx = 0;					// sample buffer index



// Interrupt service routines

//	Pin change interrupt for pin INT0
//	This interrupt is called on the falling edge of a GM pulse.
ISR(INT0_vect)
{
	if (count < UINT16_MAX)	// check for overflow, if we do overflow just cap the counts at max possible
		count++; // increase event counter
	eventflag = 1;	// tell main program loop that a GM pulse has occurred
}

//	Pin change interrupt for pin INT1 (pushbutton)
//	If the user pushes the button, this interrupt is executed.
//	We need to be careful about switch bounce, which will make the interrupt
//	execute multiple times if we're not careful.
ISR(INT1_vect)
{
	_delay_ms(25);					// slow down interrupt calls (crude debounce)
	if ((PIND & _BV(PD3)) == 0)		// is button still pressed?
		nobeep ^= 1;				// toggle mute mode
	
	EIFR |= _BV(INTF1);				// clear interrupt flag to avoid executing ISR again due to switch bounce
}

/*	Timer1 compare interrupt 
 *	This interrupt is called every time TCNT1 reaches OCR1A and is reset back to 0 (CTC mode).
 *  Timer1 is setup so this happens once a second.
 */
ISR(TIMER1_COMPA_vect)
{
	tick = 1;	// update flag
	
//	PORTB ^= _BV(PB4);	// toggle the LED (for debugging purposes)
	cps = count;
	count = 0;  // reset counter
	slowcsum -= buffer[idx];		// subtract oldest sample in sample buffer

	uint16_t t = cps;
	if (t > UINT8_MAX) {	// watch out for overflowing the sample buffer
		t = UINT8_MAX;
		overflow = 1;
	}

	slowcsum += t;			// add current sample
	buffer[idx] = t;	// save current sample to buffer (replacing old value)
	
	// Compute CPM based on the last SHORT_PERIOD samples
	int8_t x = idx - SHORT_PERIOD;
	if (x < 0) x += LONG_PERIOD;
	fastcsum -= buffer[x];
	fastcsum += t;
	
	// Move to the next entry in the sample buffer
	idx++;
	if (idx == LONG_PERIOD)
		idx = 0;
}


// flash LED and beep the piezo
void checkevent(void)
{
	if (eventflag) {		// a GM event has occurred, do something about it!
		eventflag = 0;		// reset flag as soon as possible, in case another ISR is called while we're busy

		PORTB |= _BV(PB4);	// turn on the LED
		
		if(!nobeep) {		// check if we're in mute mode
			TCCR0A |= _BV(COM0A0);	// enable OCR0A output on pin PB2
			TCCR0B |= _BV(CS01);	// set prescaler to clk/8 (1Mhz) or 1us/count
			OCR0A = 160;	// 160 = toggle OCR0A every 160ms, period = 320us, freq= 3.125kHz
		}
		
		// 10ms delay gives a nice short flash and 'click' on the piezo
		_delay_ms(10);
			
		PORTB &= ~(_BV(PB4));	// turn off the LED
		
		TCCR0B = 0;				// disable Timer0 since we're no longer using it
		TCCR0A &= ~(_BV(COM0A0));	// disconnect OCR0A from Timer0, this avoids occasional HVPS whine after beep
	}	
}

// log data to LCD
void reportLCD(void) {
	uint16_t cpm;	// This is the CPM value we will report
	char mode;	// logging mode
	if(tick) {	// 1 second has passed, time to report data via UART
/*
		gotoXY(0,0);
		uint8_t x;
		for(x = 0; x < LONG_PERIOD; x++) {
			if (x == idx) {
				 LcdSpace();
				 LcdSpace();
				 LcdSpace();
			}
			LcdNumber(buffer[x]);
			LcdSpace();
		}
		LcdFillLine();
*/
		tick = 0;	// reset flag for the next interval
		uint16_t fastcpm = fastcsum * (60 / SHORT_PERIOD);
		uint16_t slowcpm = slowcsum * (60 / LONG_PERIOD);
			
		if (overflow) {
			cpm = cps*60UL;
			mode = 'i'; // INST mode
			overflow = 0;
		}				
		else if (fastcpm > THRESHOLD) {	// if cpm is too high, use the short term average instead
			mode = 'f'; // FAST mode
			cpm = fastcpm;	// report cpm based on last 5 samples
		} else {
			mode = 's'; // SLOW mode
			cpm = slowcpm;	// report cpm based on last 60 samples
		}
		gotoXY(14,3);
		LcdCPM(mode);
		LcdNumber(cpm);
		LcdFillLine();
		gotoXY(20,2);
		LcdCPS();
		LcdNumber(cps);
		LcdFillLine();
	}
}
/*
void sleep1s(void) { // sleep using Timer1 Interupt
	set_sleep_mode(SLEEP_MODE_IDLE);	// CPU will go to sleep but peripherals keep running
	sleep_enable();		// enable sleep
	sleep_cpu();		// put the core to sleep
		// Zzzzzzz...	CPU is sleeping!
		// Execution will resume here when the CPU wakes up.
	sleep_disable();	// disable sleep so we don't accidentally go to sleep
}
*/

// Start of main program
int main(void)
{	
	uint8_t i;
	for(i = 0; i < LONG_PERIOD; i++)
		buffer[i] = 0; // init buffer

	// Set up AVR IO ports
	DDRB = _BV(PB4) | _BV(PB2);  // set pins connected to LED and piezo as outputs
	PORTD |= _BV(PD3);	// enable internal pull up resistor on pin connected to button

	LcdInitialise();
	
	// Set up external interrupts	
	// INT0 is triggered by a GM impulse
	// INT1 is triggered by pushing the button
	MCUCR |= _BV(ISC01) | _BV(ISC11);	// Config interrupts on falling edge of INT0 and INT1
	GIMSK |= _BV(INT0) | _BV(INT1);		// Enable external interrupts on pins INT0 and INT1
	
	// Configure the Timers		
	// Set up Timer0 for tone generation
	// Toggle OC0A (pin PB2) on compare match and set timer to CTC mode
	TCCR0A = (0<<COM0A1) | (1<<COM0A0) | (0<<WGM02) |  (1<<WGM01) | (0<<WGM00);
	TCCR0B = 0;	// stop Timer0 (no sound)

	// Set up Timer1 for 1 second interrupts
	TCCR1B = _BV(WGM12) | _BV(CS12);  // CTC mode, prescaler = 256 (32us ticks)
	OCR1A = 31250;  // 32us * 31250 = 1 sec
	TIMSK = _BV(OCIE1A);  // Timer1 overflow interrupt enable
	
	sei();	// Enable interrupts

	while(1) {	// loop forever
		//sleep1s();
		// Configure AVR for sleep, this saves a couple mA when idle
		set_sleep_mode(SLEEP_MODE_IDLE);	// CPU will go to sleep but peripherals keep running
		sleep_enable();		// enable sleep
		sleep_cpu();		// put the core to sleep
		
		// Zzzzzzz...	CPU is sleeping!
		// Execution will resume here when the CPU wakes up.
		
		sleep_disable();	// disable sleep so we don't accidentally go to sleep

		checkevent();	// check if we should signal an event (led + beep)

		reportLCD();	// send results to LCD
		
		checkevent();	// check again before going to sleep
		
	}	
	return 0;	// never reached
}


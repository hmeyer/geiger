#ifndef CHARACTER_H
#define CHARACTER_H

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
	

static const prog_uint16_t nums[] PROGMEM = {
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
	character( // V - 16
		B8(101),
		B8(101),
		B8(101),
		B8(101),
		B8(010)),
	character( // µ - 17
		B8(101),
		B8(101),
		B8(111),
		B8(100),
		B8(100)),
	character( // N - 18
		B8(110),
		B8(101),
		B8(101),
		B8(101),
		B8(101)),
	character( // K - 19
		B8(101),
		B8(101),
		B8(110),
		B8(101),
		B8(101)),
	character( // H - 20
		B8(101),
		B8(101),
		B8(111),
		B8(101),
		B8(101)),
	character( // / - 21
		B8(001),
		B8(001),
		B8(010),
		B8(100),
		B8(100)),
	character( // : - 22
		B8(000),
		B8(010),
		B8(000),
		B8(010),
		B8(000)),
	character( // : SPACE 23
		B8(000),
		B8(000),
		B8(000),
		B8(000),
		B8(000))
};
#endif

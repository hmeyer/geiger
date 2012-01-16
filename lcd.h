void LcdInitialise(void);
void LcdLEDon(void);
void LcdLEDoff(void);
void LcdClear(void);
void gotoXY(int x, int y);
void LcdByte(uint8_t b);
void LcdSpace(void);
void LcdCharacter(uint8_t c);
void LcdNumber(uint16_t num);
void LcdCPM(char mode);
void LcdCPS(void);
void LcdFillLine(void);

#define LCD_X     84
#define LCD_Y     48

#pragma once

#include <inttypes.h>
#include "hardware/i2c.h"

// I2C pins
#define I2C0_SCL 21
#define I2C0_SDA 20

// Device I2C Arress
#define LCD_ADDRESS 0x3E

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

#define MAX_LINES 2
#define MAX_CHARS 16

#define TOP_ROW 0
#define BOTTOM_ROW 1

void InitializeDisplay();
void ClearDisplay();
void Home();
void NoDisplay();
void Display();
void BlinkingOff();
void BlinkingOn();
void CursorOff();
void CursorOn();
void ScrollDisplayLeft();
void ScrollDisplayRight();
void LeftToRight();
void RightToLeft();
void Autoscroll();
void NoAutoscroll();
void CreateChar(uint8_t location, uint8_t charmap[]);
void SetCursor(uint8_t col, uint8_t row);
void Write(uint8_t val);
void Command(uint8_t val);
void Print(const char* str);
void PrintN(const char* str, int len);

void SendByte(unsigned char dta);
void SendByteS(const unsigned char* dta, unsigned char len);
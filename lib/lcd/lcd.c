#include "lcd.h"
#include <string.h>

#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "pico/stdlib.h"

uint8_t displayfunction_; 	// stores current "function set" command  (datasheet page 14)
uint8_t displaycontrol_;  	// stores current "display switch" command
uint8_t displaymode_;		// stores current "input set" command

void InitializeDisplay() {
	// initiialize I2C protocol
	i2c_init(i2c0, 100 * 1000); // running i2c0 at 100kHz
	gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
	gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
	gpio_pull_up(I2C0_SDA);
	gpio_pull_up(I2C0_SCL);
	bi_decl(bi_2pins_with_func(I2C0_SDA, I2C0_SCL, GPIO_FUNC_I2C));

	/*
	Based on the block diagram at page 13 of:
	https://files.seeedstudio.com/wiki/Grove-16x2_LCD_Series/res/JDH_1804_Datasheet.pdf
	*/	
	sleep_ms(20);
	
	displayfunction_ = LCD_2LINE | LCD_5x8DOTS;
	Command(LCD_FUNCTIONSET | displayfunction_);
	sleep_us(50);

	displaycontrol_ = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
	sleep_us(50);

	ClearDisplay();
	displaymode_ = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	Command(LCD_ENTRYMODESET | displaymode_);
}



void ClearDisplay() {
	Command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
	sleep_ms(2);
}

void Home() {
	Command(LCD_RETURNHOME);  // set cursor position to zero
	sleep_ms(2);
}

void SetCursor(uint8_t col, uint8_t row) {
	unsigned char val = (row == 0 ? col | 0x80 : col | 0x80 | 0x40);
	unsigned char dta[2] = {LCD_SETDDRAMADDR, val};
	SendByteS(dta, 2);
}

// Turn the display on/off (quickly)
void NoDisplay() {
	displaycontrol_ &= ~LCD_DISPLAYON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

void Display() {
	displaycontrol_ |= LCD_DISPLAYON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

void CursorOff() {
	displaycontrol_ &= ~LCD_CURSORON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

void CursorOn() {
	displaycontrol_ |= LCD_CURSORON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

// Turn on and off the blinking cursor
void BlinkingOff() {
	displaycontrol_ &= ~LCD_BLINKON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

void BlinkingOn() {
	displaycontrol_ |= LCD_BLINKON;
	Command(LCD_DISPLAYCONTROL | displaycontrol_);
}

// These commands scroll the display without changing the RAM
void ScrollDisplayLeft(void) {
	Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void ScrollDisplayRight(void) {
	Command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LeftToRight(void) {
	displaymode_ |= LCD_ENTRYLEFT;
	Command(LCD_ENTRYMODESET | displaymode_);
}

// This is for text that flows Right to Left
void RightToLeft(void) {
	displaymode_ &= ~LCD_ENTRYLEFT;
	Command(LCD_ENTRYMODESET | displaymode_);
}

// This will 'right justify' text from the cursor
void Autoscroll(void) {
	displaymode_ |= LCD_ENTRYSHIFTINCREMENT;
	Command(LCD_ENTRYMODESET | displaymode_);
}

// This will 'left justify' text from the cursor
void NoAutoscroll(void) {
	displaymode_ &= ~LCD_ENTRYSHIFTINCREMENT;
	Command(LCD_ENTRYMODESET | displaymode_);
}

void CreateChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7;  // we only have 8 locations (3 bytes used), so we discard any data after the 3th bit

	// set CGRAM location to write into
	Command(LCD_SETCGRAMADDR | (location << 3));
	
	unsigned char dta[9];
	dta[0] = LCD_SETCGRAMADDR; // first byte informs that we are writing into CGRAM
	for (int i = 0; i < 8; i++) {
		dta[i + 1] = charmap[i]; // then in the next 8 we send our char data
	}
	
	SendByteS(dta, 9);
}

/*********** mid level commands, for sending data/cmds */

// send command
inline void Command(uint8_t value) {
	unsigned char dta[2] = {LCD_SETDDRAMADDR, value};
	SendByteS(dta, 2);
}

// Print a character from font table
void Write(uint8_t value) {
	unsigned char dta[2] = {LCD_SETCGRAMADDR, value};
	SendByteS(dta, 2);
}

// Print a string
void Print(const char* str) {
	for (int i = 0; i < strlen(str); i++) {
		unsigned char dta[2] = {LCD_SETCGRAMADDR, str[i]};
		SendByteS(dta, 2);
	}
}

void PrintN(const char* str, int len) {
	for (int i = 0; i < len; i++) {
		unsigned char dta[2] = {LCD_SETCGRAMADDR, str[i]};
		SendByteS(dta, 2);
	}
}

void SendByte(unsigned char dta) {
	i2c_write_blocking(i2c0, LCD_ADDRESS, &dta, 1, false);
}

void SendByteS(const unsigned char* dta, unsigned char len) {
	i2c_write_blocking(i2c0, LCD_ADDRESS, dta, len, false);
}
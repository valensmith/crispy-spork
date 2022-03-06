/*
 * lcd.c
 *
 * Created: 27/09/2021 8:08:43 PM
 *  Author: hsmit
 */ 
// GLOBAL DEFINES
#define F_CPU 8000000L // run CPU at 16 MHz
#define LED 5 // Boarduino LED on PB5, used for testing purposes
#define ClearBit(x,y) x &= ~_BV(y) // equivalent to cbi(x,y)
#define SetBit(x,y) x |= _BV(y) // equivalent to sbi(x,y)
// INCLUDES
#include "lcd.h"



const char ohm[8] = {
	0b00000000,
	0b00001110,
	0b00010001,
	0b00010001,
	0b00010001,
	0b00001010,
	0b00001010,
	0b00011011,
};

const char pc[8] = {
	0b00011100,
	0b00010100,
	0b00011100,
	0b00010000,
	0b00010111,
	0b00000100,
	0b00000100,
	0b00000111,
};

const char light[8] = {
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
	0b00011111,
};

const char hold[8] = {
	0b00011011,
	0b00011011,
	0b00011011,
	0b00011011,
	0b00011011,
	0b00011011,
	0b00011011,
	0b00011011,
};

void SetupPorts()
{
	DDRD |= 0x3C; // 0011.1100; set D2-D5 as outputs
	DDRC |= 0x06; // 0000.0110; set C0-C1 as outputs
}
void msDelay(int delay) // put into a routine
{ // to remove code inlining
	for (int i=0;i<delay;i++) // at cost of timing accuracy
	_delay_ms(1);
}
void FlashLED()
{
	SetBit(PORTD,LED);
	msDelay(250);
	ClearBit(PORTD,LED);
	msDelay(250);
}
void PulseEnableLine ()
{
	SetBit(PORTC,LCD_E); // take LCD enable line high
	_delay_us(40); // wait 40 microseconds
	ClearBit(PORTC,LCD_E); // take LCD enable line low
}
void SendNibble(byte data)
{
	PORTD &= 0xC3; // 1100.0011 = clear 4 data lines
	if (data & _BV(4)) SetBit(PORTD,DAT4);
	if (data & _BV(5)) SetBit(PORTD,DAT5);
	if (data & _BV(6)) SetBit(PORTD,DAT6);
	if (data & _BV(7)) SetBit(PORTD,DAT7);
	PulseEnableLine(); // clock 4 bits into controller
}
void SendByte (byte data)
{
	SendNibble(data); // send upper 4 bits
	SendNibble(data<<4); // send lower 4 bits
	ClearBit(PORTD,5); // turn off boarduino LED
}
void LCD_Cmd (byte cmd)
{
	ClearBit(PORTC,LCD_RS); // R/S line 0 = command data
	SendByte(cmd); // send it
}
void LCD_Char (byte ch)
{
	SetBit(PORTC,LCD_RS); // R/S line 1 = character data
	SendByte(ch); // send it
}
void LCD_Init()
{
	LCD_Cmd(0x33); // initialize controller
	LCD_Cmd(0x32); // set to 4-bit input mode
	LCD_Cmd(0x28); // 2 line, 5x7 matrix
	LCD_Cmd(0x0C); // turn cursor off (0x0E to enable)
	LCD_Cmd(0x06); // cursor direction = right
	LCD_Cmd(0x01); // start with clear display
	msDelay(3); // wait for LCD to initialize
}
void LCD_Clear() // clear the LCD display
{
	LCD_Cmd(CLEARDISPLAY);
	msDelay(3); // wait for LCD to process command
}
void LCD_Home() // home LCD cursor (without clearing)
{
	LCD_Cmd(SETCURSOR);
}
void LCD_Goto(byte x, byte y) // put LCD cursor on specified line
{
	byte addr = 0; // line 0 begins at addr 0x00
	switch (y)
	{
		case 1: addr = 0x40; break; // line 1 begins at addr 0x40
		case 2: addr = 0x14; break;
		case 3: addr = 0x54; break;
	}
	LCD_Cmd(SETCURSOR+addr+x); // update cursor with x,y position
}
void LCD_Line(byte row) // put cursor on specified line
{
	LCD_Goto(0,row);
}
void LCD_Message(const char *text) // display string on LCD
{
	while (*text) // do until /0 character
	LCD_Char(*text++); // send char & update char pointer
}
void LCD_Hex(int data)
// displays the hex value of DATA at current LCD cursor position
{
	char st[8] = ""; // save enough space for result
	itoa(data,st,16); // convert to ascii hex
	//LCD_Message("0x"); // add prefix "0x" if desired
	LCD_Message(st); // display it on LCD
}
void LCD_Integer(int data)
// displays the integer value of DATA at current LCD cursor position
{
	char st[8] = ""; // save enough space for result
	itoa(data,st,10); // convert to ascii
	LCD_Message(st); // display in on LCD
}

void LCD_custom_char(int loc, const char *custom) {
	LCD_Cmd (0x40 + (loc*8));
	for (int i = 0; i < 8; i++) {
		LCD_Char(custom[i]);
	}	
}
// ---------------------------------------------------------------------------

void init_brightness(void) {
	DDRD |= (1<<DDD6);
	DDRB |= (1<<DDB1);
	TCCR0A |= (1<<COM0A1) | (1<<WGM01) | (1<<WGM00);
	TCCR0B |= (1<<CS00);
	TCCR1A |= (1<<COM1A1) | (1<<WGM11) | (1<<WGM11);
	TCCR1B |= (1<<CS10);
	OCR0A = 120;
	OCR1A = 306;
}
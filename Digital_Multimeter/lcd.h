/*
 * lcd.h
 *
 * Created: 27/09/2021 8:08:55 PM
 *  Author: hsmit
 */ 


#ifndef LCD_H_
#define LCD_H_

// INCLUDES
#include <avr/io.h> // deal with port registers
#include <util/delay.h> // used for _delay_ms function
#include <string.h> // string manipulation routines
#include <stdlib.h>
#include <stdint.h>

typedef uint8_t byte; // I just like byte & sbyte better
typedef int8_t sbyte;

// The LCD module requires 6 I/O pins: 2 control lines & 4 data lines.
// PortD is used for data communications with the HD44780-controlled LCD.
// The following defines specify which port pins connect to the controller:
#define LCD_RS 1 // pin for LCD R/S (eg PB0)
#define LCD_E 2 // pin for LCD enable
#define DAT4 2 // pin for d4
#define DAT5 3 // pin for d5
#define DAT6 4 // pin for d6
#define DAT7 5 // pin for d7
// The following defines are HD44780 controller commands
#define CLEARDISPLAY 0x01
#define SETCURSOR 0x80
#define NUMCHARS 64 // number of characters per screen

const char ohm[8];

const char pc[8];

const char light[8];

const char hold[8];

void SetupPorts();

void msDelay(int delay);

void PulseEnableLine ();

void SendNibble(byte data);

void SendByte (byte data);

void LCD_Cmd (byte cmd);

void LCD_Char (byte ch);

void LCD_Init();

void LCD_Clear();

void LCD_Home();

void LCD_Goto(byte x, byte y);

void LCD_Line(byte row);

void LCD_Message(const char *text);

void LCD_Hex(int data);

void LCD_Integer(int data);

void LCD_custom_char(int loc, const char *custom);

void UpdateCursor (byte count);

char GetNextChar(char ch);

void FillScreen ();

void init_brightness(void);


#endif /* LCD_H_ */
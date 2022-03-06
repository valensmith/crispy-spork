// GLOBAL DEFINES
#define F_CPU 8000000L // run CPU at 16 MHz
#define LED 5 // Boarduino LED on PB 5
#define ClearBit(x,y) x &= ~_BV(y) // equivalent to cbi(x,y)
#define SetBit(x,y) x |= _BV(y) // equivalent to sbi(x,y)
// ---------------------------------------------------------------------------
// INCLUDES
#include <avr/io.h> // deal with port registers
#include <util/delay.h> // used for _delay_ms function
#include <string.h> // string manipulation routines
#include <stdlib.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
//
// HEADER INCLUDES
#include "buttons.h"
#include "lcd.h"
#include "ads1115.h"
#include "uart_hal.h"
#include "eeprom.h"

uint8_t pause = 0;
int max = -1000;
int min = 1000;
uint16_t offset;
uint8_t probe = 0;
static volatile uint8_t last_button_state2;
uint8_t bright = 2;
uint8_t current_mode = 0;
int continu = 20;
uint8_t held[] = "h\n";
int communicate = 4;
uint8_t run = 20;
uint8_t err = 0;
uint8_t conToggle = 0;

int16_t adc0, adc1, adc2, adc3, adcB;
float volts0, volts1, volts2, volts3, voltsB;
int num = 1;
float vrms;
float voltages[250];
int vrmsI = 0;
float voltageBattery;

// ---------------------------------------------------------------------------


uint8_t change_mode(uint8_t mode);
void bright_change(uint8_t up);
void set_battery(void);

void reset_minmax() {
	min = 1000;
	max = -1000;
}
void setup_initialisation() {
	SetupPorts(); // use PortD for LCD interface
	LCD_Init(); // initialize LCD controller
	init_button_interrupts(); // using pins b0, b6, b7, c0, d7
	init_brightness(); // starting at brightness 3
	bright_change(1); // prints the brightness bar graph on the LCD
	reset_minmax();
	uart_init(9600,0); // baud rate of 9600
	//err = EEPROM_write(96, 20); only run this to set continuity to 2.0
	err = EEPROM_read(96, &run); // reads the saved continuity value
	set_battery(); // saves the voltage of the battery
	sei();
	
	DDRC &= ~(1<<DDC3) | (1<<DDC0);
		
	LCD_Line(0); // this block sets up the initial display
	LCD_Message("DC Voltage (V)");
	LCD_Goto(0,1);
	LCD_Message("Mx:");
	LCD_Goto(10,1);
	LCD_Message("Mn:"); 
	
	LCD_custom_char(0, pc); // creates the custom characters
	LCD_custom_char(1, ohm);
	LCD_custom_char(2, light);
	LCD_custom_char(3, hold);
}


void set_offset (uint16_t oset) { // sets the offset value
	offset = oset;
}

void set_battery() { // saves the voltage of the battery
	int16_t adcBat = ads1115_readADC_SingleEnded(0x48, 2, ADS1115_DR_860SPS, ADS1115_PGA_6_144);
	voltageBattery = adcBat/32767.0 * 6.144;
}


uint8_t change_mode(uint8_t mode) { // sets of the LCD for the next mode and resets the necessary values
	if (mode == 3) {
		mode = 0;
	} else {
		mode += 1;
	}
	
	LCD_Clear();
	bright_change(-1);
	pause = 0;

	switch (mode) {
		case 0: // DC
			reset_minmax();
			LCD_Line(0);
			LCD_Message("DC Voltage (V)");
			LCD_Goto(0,1);
			LCD_Message("Mx:");
			LCD_Goto(10,1);
			LCD_Message("Mn:");
			break;
		case 1: // AC
			reset_minmax();
			vrmsI = 0;
			LCD_Line(0);
			LCD_Message("AC Voltage (VRMS)");
			LCD_Goto(0,1);
			LCD_Message("Mx:");
			LCD_Goto(10,1);
			LCD_Message("Mn:");
			break;
		case 2: // R
			set_offset(0);
			LCD_Line(0);
			LCD_Message("Resistance");			
			break;
		case 3: // C
			set_offset(0);
			LCD_Line(0);
			LCD_Message("Continuity");
			break;
	}
	return mode;
}

void bright_change(uint8_t up) { // changes brightness and prints on LCD
	if (up == 1) {
		if (bright != 4) {
			bright++;
		}
	} else if (up == 0){
		if (bright != 0) {
			bright--;
		}
	}
	OCR1A = bright * 51; 
	LCD_Goto(19, 0);
	LCD_Char(0xFE);
	LCD_Goto(19, 1);
	LCD_Char(0xFE);
	LCD_Goto(19, 2);
	LCD_Char(0xFE);
	LCD_Goto(19, 3);
	LCD_Char(0xFE);
	for (int i = 0; i < bright; i++) {
		int temp = 3 - i;
		LCD_Goto(19, temp);
		LCD_Char(0xFE);
		if (bright) {
			LCD_Goto(19, temp);
			LCD_Char(2);
		}
	}
}

void toggle_hold() { // toggles hold and tells gui
	if (pause) {
		LCD_Goto(18, 0);
		LCD_Message(" ");
	} else {
		LCD_Goto(18, 0);
		LCD_Char(3);
	}
	uart_send_string(held);
	pause = 1 - pause;
}

ISR(PCINT2_vect) { // activated by pin d7 for hold
	cli();
	uint8_t button_state = PIND & 0x80;
	if (!(last_button_state2 & (1 << 7)) && (button_state & (1 << 7))) {
		toggle_hold();
		msDelay(200);
	}
	last_button_state2 = button_state;
	sei();
}
 
void reset_values() { // handles the multi-purpose reset/probe button
	switch (current_mode) {
		case 0:
		reset_minmax();
		break;
		case 1:
		reset_minmax();
		break;
		case 2:
		probe = 1;
		break;
		case 3:
		if (conToggle) {
			 EEPROM_write(96, 145);
		} else {
			EEPROM_write(96, 20);
		}
		conToggle = 1 - conToggle;
		break;
	 }
 }
 
ISR(PCINT1_vect) { // activated by pin c0 for reset/probe
	cli();		
	reset_values();
	sei();
}

 void print_decimal(int num, int col, int row) { //prints the decimal at the required position
		int whole = (num/100);
		int decimal = abs(num % 100);
		 
		LCD_Goto(col, row);
		LCD_Message("     ");
		LCD_Goto(col, row);
		LCD_Integer(whole);
		LCD_Goto((col+1+(whole>=10)+(whole>=100)+(whole<0)), row);
		LCD_Message(".");
		LCD_Goto((col+2+(whole>=10)+(whole>=100)+(whole<0)), row);
		if (decimal < 10) {
			LCD_Integer(0);
		}
		LCD_Integer(decimal);
}

void dc_voltage() { // reads adc for DC voltage and prints it

	int16_t dc = ads1115_readADC_SingleEnded(0x48, 3, ADS1115_DR_8SPS, ADS1115_PGA_0_256);
	
	float dcf = 6.4 * dc * 0.0078125;
	print_decimal(dcf, 0, 2);
	
	char temp[80];
	dtostrf((dcf / 100), 3, 2, temp);
	uart_send_string_char(temp);
	uart_send_char('\n');
	
	if (dcf > max) {
		max = dcf;
		print_decimal(max, 3, 1);
	} 
	if (dcf < min) {
		min = dcf;
		print_decimal(min, 13, 1);
	}
}

void take_samples(float* v, int num) { // creates a list of AC voltage samples
	int i = 0;
	while (i < num) {
		int16_t adc3 = ads1115_readADC_SingleEnded(0x48, 3, ADS1115_DR_860SPS, ADS1115_PGA_0_256);
		volts3 = 6.4 * adc3 * 0.0078125;
		v[vrmsI] = volts3;
		if (vrmsI != num - 1) {
			vrmsI += 1;
			} else {
			vrmsI = 0;
		}
		i++;
	}
}

void ac_voltage() { // reads the adc for ac voltage and prints it
	take_samples(voltages, 250);
	int j;
	long double sum = 0;
	for (j=0; j<250; j++) {
		sum += ((voltages[j]) * (voltages[j]));
	}
	vrms = sqrt(sum/250);
	
	if (((abs(voltages[230]) - abs(voltages[20])) < 0.02) &&
		((abs(voltages[140]) - abs(voltages[73])) < 0.02) &&
		((abs(voltages[80]) - abs(voltages[182])) < 0.02) &&
		((abs(voltages[13]) - abs(voltages[103])) < 0.02) &&
		((abs(voltages[120]) - abs(voltages[94])) < 0.02) &&
		((abs(voltages[243]) - abs(voltages[156])) < 0.02) &&
		((abs(voltages[223]) - abs(voltages[136])) < 0.02) &&
		((abs(voltages[213]) - abs(voltages[146])) < 0.02) &&
		((abs(voltages[253]) - abs(voltages[237])) < 0.02)) {
			vrms = (abs(voltages[200]) + abs(voltages[14]) + abs(voltages[139]))/3;
	}
	
	print_decimal(vrms, 0, 2);
 	char temp[80];
 	dtostrf((vrms/100), 3, 2, temp);
 	uart_send_string_char(temp);
 	uart_send_char('\n');
	
	if (vrms > max) {
		max = vrms;
		print_decimal(max, 3, 1);
	}
	if (vrms < min) {
		min = vrms;
		print_decimal(min, 13, 1);
	}
}

void three_sig_print(double rloaded, int col, int row) { // prints R values in 3 sig figures
	int rload = rloaded;
	int whole = abs(rload / 100);
	int decimal = abs(rload % 100);
	
	LCD_Goto(col, row);
	LCD_Message("       ");
	if ((whole % 100)) {
		LCD_Goto(col, row);
		LCD_Integer(whole);
	} else if ((whole % 10)) {
		LCD_Goto(col, row);
		LCD_Integer(whole);
		LCD_Message(".");
		LCD_Integer(decimal/10);
	} else {
		LCD_Goto(col, row);
		LCD_Integer(whole);
		LCD_Message(".");
		if (decimal < 10) {
			LCD_Integer(0);
			} else {
			LCD_Integer(decimal);
		}
	}
}

void three_sig(double rload) { // prints the si unit of R value
	char temp[80];
	dtostrf(rload, 3, 2, temp);
	uart_send_string_char(temp);
	uart_send_char('\n');
	
	LCD_Line(1);
	if (rload < 1000) {
		three_sig_print(rload*100, 0, 1);
	} else if (rload < 1000000) {
		three_sig_print(rload/10, 0, 1);
		LCD_Char('k');
	} else if (rload < 1200000) {
		three_sig_print(rload/10000, 0, 1);
		LCD_Char('M');
	} else {
		LCD_Message(">1.2M");
	}
	LCD_Char(1);
}


void print_threshold() { // reads addr 96 for the continuity value
	uint8_t con = 0;
	EEPROM_read(96, &con);
	print_decimal((int)(con*10), 7, 1);	
}

int check_mode() { // checks what resistance values are being used for R mode
	int currentMode = 0;
	
	int high, med; // switch states
	
	msDelay(100);
	adc0 = ads1115_readADC_SingleEnded(0x48, 0, ADS1115_DR_8SPS, ADS1115_PGA_6_144);
	volts2 = adc0 / 32767.0 * 6.144;
	msDelay(100);
	adc1 = ads1115_readADC_SingleEnded(0x48, 1, ADS1115_DR_8SPS, ADS1115_PGA_6_144);
	volts0 = adc1 / 32767.0 * 6.144;
	msDelay(100);
	adc2 = ads1115_readADC_SingleEnded(0x48, 2, ADS1115_DR_8SPS, ADS1115_PGA_6_144);
	volts1 = adc2 / 32767.0 * 6.144;
	
	if ((volts1 - volts2) < 0.01) {
		// HIGH shorted
		high = 0;
	} else {
		high = 1;
	}
	if ((volts0 - volts1) < 0.01) {
		// MED shorted
		med = 0;
	} else {
		med = 1;
	}
	
	if (!high && !med) {
		// LOW
		currentMode = 0;
	} else if (!high && med) {
		// MED
		currentMode = 1;
	} else if (high && med) {
		currentMode = 2;
	} else if (high && !med) {
		currentMode = -2;
	} else {
		currentMode = -1;
	}
	
	return currentMode;
}

void resistance_mode(uint8_t continuity) { // reads adc for resistance values
	
	msDelay(100);
	adcB = ads1115_readADC_SingleEnded(0x48, 2, ADS1115_DR_8SPS, ADS1115_PGA_6_144);

	voltsB = adcB/32767.0 * 6.144;
	
	// LOW = 0, MED = 1, HIGH = 2
	// assume start in low mode
	static int state = 0;
	double r[3] = {50.0, 5150.0, 79450.0};
	char* mode[3] = {"LOW ", "MED ", "HIGH"};
	
	double resistance;
	
	if ((voltageBattery - voltsB) < 0.05) {
		if (state == 0) {
			state = 2;
		} else if (state == 1 && check_mode() == 1) {
			state = 2;
		} else {
			// do nothing
		}
	} else if ((voltageBattery - voltsB) > (voltageBattery - 0.1)) {
		if (state == 2) {
			state = 1;
		} else if (state == 1 && check_mode() == 1) {
			state = 0;
		} else {
			// do nothing
		}
	} else {
		// do nothing
	}

	resistance = r[state];
	if (1) {
		LCD_Goto(5, 2);
		LCD_Message("go to ");
		LCD_Message(mode[state]);
	}
	
	double current = (voltageBattery-voltsB) / resistance;
	
	if (continuity) {
		current = (voltageBattery - voltsB) / 50.0;
	}
	
	if (probe && !continuity) {
		set_offset(voltsB / current);
		probe = 0;
	}
	
	double rload = voltsB / current - offset;
	
	three_sig(rload);

	if (continuity) {
		print_threshold();
		LCD_Goto(13,1);
		if ((rload) > (run/10) || voltsB > (voltageBattery - 0.1)) {
			LCD_Message("OPEN ");
		} else {
			LCD_Message("SHORT");
		}
	}
	if (!continuity) {
		if (state == check_mode()) {
			LCD_Goto(0, 2);
			LCD_Message("good");
		} else {
			LCD_Goto(0, 2);
			LCD_Message("bad ");
		}
	}
}
// ---------------------------------------------------------------------------
// MAIN PROGRAM
int main(void) {
	setup_initialisation();
	int8_t btn;
	uint8_t data[80];
	uint8_t model[] = "m\n";
 	double rtest = 5864.78934;
	char ctest[80];
	dtostrf(rtest, 4, 2, ctest);
	uint8_t brighUp[] = "B\n";
	uint8_t brighDown[] = "b\n";
	uint8_t testComs[] = "z\n";
	int message_i;

	
	
	
	while(1) {
		msDelay(100);
		btn = button_pushed();
		if ((btn == -1)) {
			uart_send_string(testComs);
		}
		switch (btn) {
			case BUTTON0_PUSHED:
			current_mode = change_mode(current_mode);
			uart_send_string(model);
			break;
			case BUTTON6_PUSHED:
			bright_change(0);
			uart_send_string(brighDown);
			break;
			case BUTTON7_PUSHED:
			bright_change(1);
			uart_send_string(brighUp);
			break;
		}
		if(!pause) {
			switch (current_mode) {
				case 0:
				dc_voltage();
				break;
				case 1:
				ac_voltage();
				break;
				case 2:
				resistance_mode(0);
				break;
				case 3:
				resistance_mode(1);
				break;
			}
		} else {
			uart_send_string(testComs);
		}
		if(uart_read_count() > 0){
			for(int i = 0; uart_read_count() > 0; i++){
				data[i] = uart_read();
			}
			if ((data[0] == 'm')) {
				current_mode = change_mode(current_mode);
				communicate = 1;
			}
			if ((data[0] == 'B')) {
				bright_change(1);
				communicate = 1;
			}
			if ((data[0] == 'b')) {
				bright_change(0);
				communicate = 1;
			}
			if ((data[0] == 'h')) {
				toggle_hold();
				communicate = 1;
			}
			if ((data[0] == 'r')) {
				reset_values();
				communicate = 1;
			}
			if ((data[0] == 'l')) {
				communicate = 1;
			}
			if (isdigit(data[0])) { // this is for setting continuity from the pc. Doesn't currently work
				communicate = 1;
			 	run = 0;
			 	for (int i = 0; i < (sizeof(data)/sizeof(data[0])); i++) {
			 		run = run*10;
			 		run += data[i];
			 	}
			  	err = EEPROM_write(96,run);
			}
			if ((data[0] == '%')) {
				communicate = 1;
				LCD_Line(3);
				LCD_Message("                ");
				LCD_Line(3);
				for (message_i = 1; message_i < 17; message_i++) {
					if (data[message_i] == '@') {
						LCD_Message(" ");
						break;
					}
					LCD_Char(data[message_i]);
					_delay_ms(20);
				}
			}
		}	
		LCD_Goto(17, 0);
		if (communicate) {
			LCD_Char(0);
			} else {
			LCD_Message(" ");
		}
		if (communicate > 0) {
			communicate--;
		}
	}
	return 0;
}
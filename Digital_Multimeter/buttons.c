/*
 * buttons.c
 *
 * Author: Peter Sutton
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "buttons.h"
#include "lcd.h"


// Global variable to keep track of the last button state so that we 
// can detect changes when an interrupt fires. The lower 3 bits (0 to 2)
// will correspond to the last state of port B pins 0 to 2.
static volatile uint8_t last_button_state;

// Our button queue. button_queue[0] is always the head of the queue. If we
// take something off the queue we just move everything else along. We don't
// use a circular buffer since it is usually expected that the queue is very
// short. In most uses it will never have more than 1 element at a time.
// This button queue can be changed by the interrupt handler below so we should
// turn off interrupts if we're changing the queue outside the handler.
#define BUTTON_QUEUE_SIZE 4
static volatile uint8_t button_queue[BUTTON_QUEUE_SIZE];
static volatile int8_t queue_length;

// Setup interrupt if any of pins B0 to B2 change. We do this
// using a pin change interrupt. These pins correspond to pin
// change interrupts PCINT8 to PCINT10 which are covered by
// Pin change interrupt 1.
void init_button_interrupts(void) {
	// Enable the interrupt (see datasheet page 77)
	PCICR = (1<<PCIE0)|(1<<PCIE1)|(1<<PCIE2);
	sei();
	
	// Make sure the interrupt flag is cleared (by writing a 
	// 1 to it) (see datasheet page 78)
	PCIFR = (1<<PCIF0)|(1<<PCIF1)|(1<<PCIF2);
	
	// Choose which pins we're interested in by setting
	// the relevant bits in the mask register (see datasheet page 78)
	PCMSK0 = (1<<PCINT0)|(1<<PCINT1)|(1<<PCINT6)|(1<<PCINT7);	
	PCMSK1 = (1<<PCINT8);
	PCMSK2 = (1<<PCINT23);
	
	// Empty the button push queue
	queue_length = 0;
}

int8_t button_pushed() {
	int8_t return_value = NO_BUTTON_PUSHED;	// Assume no button pushed
	if(queue_length > 0) {
		// Remove the first element off the queue and move all the other
		// entries closer to the front of the queue. We turn off interrupts (if on)
		// before we make any changes to the queue. If interrupts were on
		// we turn them back on when done.
		return_value = button_queue[0];
		
		// Save whether interrupts were enabled and turn them off
		int8_t interrupts_were_enabled = bit_is_set(SREG, SREG_I);
		cli();
		
		for(uint8_t i = 1; i < queue_length; i++) {
			button_queue[i-1] = button_queue[i];
		}
		queue_length--;
		
		if(interrupts_were_enabled) {
			// Turn them back on again
			sei();
		}
	}
	return return_value;
}

// Interrupt handler for a change on buttons
ISR(PCINT0_vect) {
	// Get the current state of the buttons. We'll compare this with
	// the last state to see what has changed.
	uint8_t button_state = PINB & 0xC3;
	
	// Iterate over all the buttons and see which ones have changed.
	// Any button pushes are added to the queue of button pushes (if
	// there is space). We ignore button releases so we're just looking
	// for a transition from 0 in the last_button_state bit to a 1 in the 
	// button_state.
	static int pins[4] = {0,6,7};
	for(uint8_t pin=0; pin<=2; pin++) {
		if(queue_length < BUTTON_QUEUE_SIZE && 
				(button_state & (1<<pins[pin])) && 
				!(last_button_state & (1<<pins[pin]))) {
			// Add the button push to the queue (and update the
			// length of the queue
			button_queue[queue_length++] = pins[pin];
			msDelay(50);
		}
	}
	
	// Remember this button state
	last_button_state = button_state;
}

	

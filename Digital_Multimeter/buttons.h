#ifndef BUTTONS_H_
#define BUTTONS_H_

#include <stdint.h>

#define NO_BUTTON_PUSHED (-1)
#define BUTTON0_PUSHED 0
#define BUTTON6_PUSHED 6
#define BUTTON7_PUSHED 7

/* Set up pin change interrupts on pins B0 to B2.
 * It is assumed that global interrupts are off when this function is called
 * and are enabled sometime after this function is called.
 */
void init_button_interrupts(void);

/* Return the last button pushed (0 to 3) or -1 (NO_BUTTON_PUSHED) if 
 * there are no button pushes to return. (A small queue of button pushes
 * is kept. This function should be called frequently enough to
 * ensure the queue does not overflow. Excess button pushes are
 * discarded.)
 */

int8_t button_pushed();



#endif /* BUTTONS_H_ */
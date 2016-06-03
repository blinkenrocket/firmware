/*
 * Copyright (C) 2016 by Daniel Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#include <avr/io.h>
#include <stdlib.h>

#include "system.h"

int main (void)
{
	rocket.initialize();

	while (1) {
		// nothing to do here, go to idle to save power
		SMCR = _BV(SE);
		asm("sleep");
		/*
		 * The display timer causes a wakeup after 256µs. Run the system
		 * loop after the timer's ISR is done.
		 * The Modem also causes wakeups, which is pretty convenient since
		 * it means we can immediately process the received data.
		 */
		rocket.loop();
	}

	return 0;
}

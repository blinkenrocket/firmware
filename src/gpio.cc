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

#include "gpio.h"

GPIO gpio;

void GPIO::pinMode(const uint8_t pin, uint8_t mode)
{
	/*
	 * pin is const, so this switch statement is optimized out by the
	 * compiler. If mode is also a literal, the function call will also
	 * be optimized out, resulting in just 2 Bytes per pinMode call.
	 */
	switch (pin) {
		case 1:
			if (mode == 1)
				DDRC |= _BV(PC0);
			else
				DDRC &= ~_BV(PC0);
		break;
		case 2:
			if (mode == 1)
				DDRC |= _BV(PC1);
			else
				DDRC &= ~_BV(PC1);
		break;
		case 3:
			if (mode == 1)
				DDRC |= _BV(PC2);
			else
				DDRC &= ~_BV(PC2);
		break;
		case 4:
			if (mode == 1)
				DDRA |= _BV(PA1);
			else
				DDRA &= ~_BV(PA1);
		break;
	}
}

void GPIO::digitalWrite(const uint8_t pin, uint8_t value)
{
	/*
	 * will be optimized out -- see above
	 */
	switch (pin) {
		case 1:
			if (value == 1)
				PORTC |= _BV(PC0);
			else
				PORTC &= ~_BV(PC0);
		break;
		case 2:
			if (value == 1)
				PORTC |= _BV(PC1);
			else
				PORTC &= ~_BV(PC1);
		break;
		case 3:
			if (value == 1)
				PORTC |= _BV(PC2);
			else
				PORTC &= ~_BV(PC2);
		break;
		case 4:
			if (value == 1)
				PORTA |= _BV(PA1);
			else
				PORTA &= ~_BV(PA1);
		break;
	}
}

uint8_t GPIO::digitalRead(const uint8_t pin)
{
	/*
	 * will be optimized out -- see above
	 */
	switch (pin) {
		case 1:
			return (PINC & _BV(PC0));
		case 2:
			return (PINC & _BV(PC1));
		case 3:
			return (PINC & _BV(PC2));
		case 4:
			return (PINA & _BV(PA1));
	}
	return 0;
}

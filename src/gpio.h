/*
 * Copyright (C) 2016 by Birte Kristina Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#include <avr/io.h>
#include <stdlib.h>

/**
 * GPIO class for the GPIO pins E1 ... E4
 *
 * For now, we only support very simple Arduino-like digital pin operations.
 */
class GPIO {
	public:
		GPIO() {};

		void pinMode(const uint8_t pin, uint8_t mode);
		void digitalWrite(const uint8_t pin, uint8_t value);
		uint8_t digitalRead(const uint8_t pin);
};

extern GPIO gpio;

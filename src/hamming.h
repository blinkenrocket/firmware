/*
 * Copyright (C) 2016 by Daniel Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#ifndef HAMMING_H_
#define HAMMING_H_

#include <avr/pgmspace.h>

enum HammingResult : uint8_t {
	NO_ERROR = 0,
	ERROR_IN_PARITY = 0xfe,
	UNCORRECTABLE = 0xff,
};

const uint8_t hammingParityLow[] PROGMEM =
{ 0,  3,  5,  6,  6,  5,  3,  0,  7,  4,  2,  1,  1,  2,  4,  7 };

const uint8_t hammingParityHigh[] PROGMEM =
{ 0,  9, 10,  3, 11,  2,  1,  8, 12,  5,  6, 15,  7, 14, 13,  4 };

const uint8_t PROGMEM hammingParityCheck[] = {
	NO_ERROR,
	ERROR_IN_PARITY,
	ERROR_IN_PARITY,
	0x01,
	ERROR_IN_PARITY,
	0x02,
	0x04,
	0x08,
	ERROR_IN_PARITY,
	0x10,
	0x20,
	0x40,
	0x80,
	UNCORRECTABLE,
	UNCORRECTABLE,
	UNCORRECTABLE
};

#endif /* HAMMING_H_ */

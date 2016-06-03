/*
 * Copyright (C) 2016 by Daniel Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#ifndef STATIC_PATTERNS_H_
#define STATIC_PATTERNS_H_

#include <avr/pgmspace.h>

/*
 * Note: Static patterns must not be longer than 128 bytes (headers excluded).
 * See MessageSpecification.md for the meaning of their headers.
 */

const uint8_t PROGMEM shutdownPattern[] = {
	0x20, 0x40,
	0x0e, 0x0f,
	0xff, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xff,
	0x7e, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42, 0x7e,
	0x3c, 0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x3c,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00,
	0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const uint8_t PROGMEM flashingPattern[] = {
	0x20, 0x10,
	0x08, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x07, 0x33, 0x55, 0x98, 0x00, 0x00
};

#ifdef LANG_DE
const uint8_t PROGMEM emptyPattern[] = {
	0x10, 0x29,
	0xc0, 0x00,
	' ',   1, ' ', 'B', 'l', 'i', 'n', 'k', 'e', 'n', 'r', 'o', 'c', 'k', 'e',
	't', ' ', 'v', '1', '.', '0', ' ', '-', ' ', 'S', 'p', 'e', 'i', 'c', 'h',
	'e', 'r', ' ', 'i', 's', 't', ' ', 'l', 'e', 'e', 'r'
};
#else
const uint8_t PROGMEM emptyPattern[] = {
	0x10, 0x28,
	0xc0, 0x00,
	' ',   1, ' ', 'B', 'l', 'i', 'n', 'k', 'e', 'n', 'r', 'o', 'c', 'k', 'e',
	't', ' ', 'v', '1', '.', '0', ' ', '-', ' ', 'S', 't', 'o', 'r', 'a', 'g',
	'e', ' ', 'i', 's', ' ', 'e', 'm', 'p', 't', 'y'
};
#endif

#ifdef LANG_DE
const uint8_t PROGMEM timeoutPattern[] = {
	0x10, 0x16,
	0xc0, 0x00,
	' ',   2, ' ', 'U', 'e', 'b', 'e', 'r', 't', 'r', 'a', 'g', 'u', 'n', 'g',
	's', 'f', 'e', 'h', 'l', 'e', 'r'
};
#else
const uint8_t PROGMEM timeoutPattern[] = {
	0x10, 0x15,
	0xc0, 0x00,
	' ',   2, ' ', 'T', 'r', 'a', 'n', 's', 'm', 'i', 's', 's', 'i', 'o', 'n',
	' ', 'e', 'r', 'r', 'o', 'r'
};
#endif

#endif /* STATIC_PATTERNS_H_ */

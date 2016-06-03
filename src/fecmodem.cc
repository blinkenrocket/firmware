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
#include "fecmodem.h"

uint8_t FECModem::parity128(uint8_t byte)
{
	return pgm_read_byte(&hammingParityLow[byte & 0x0f]) ^ pgm_read_byte(&hammingParityHigh[byte >> 4]);
}

uint8_t FECModem::parity2416(uint8_t byte1, uint8_t byte2)
{
	return parity128(byte1) | (parity128(byte2) << 4);
}

uint8_t FECModem::correct128(uint8_t *byte, uint8_t err)
{
	uint8_t result = pgm_read_byte(&hammingParityCheck[err & 0x0f]);

	if (result != NO_ERROR) {
		if (byte == NULL)
			return 3;
		if (result == UNCORRECTABLE) {
			*byte = 0;
			return 3;
		}
		if (result != ERROR_IN_PARITY) {
			*byte ^= result;
		}
		return 1;
	}
	return 0;
}

uint8_t FECModem::hamming2416(uint8_t *byte1, uint8_t *byte2, uint8_t parity)
{
	uint8_t err;

	if (byte1 == NULL || byte2 == NULL) {
		return 3;
	}

	err = parity2416(*byte1, *byte2) ^ parity;

	if (err) {
		return correct128(byte1, err) + correct128(byte2, err >> 4);
	}

	return 0;
}

void FECModem::enable()
{
	this->Modem::enable();
	hammingState = FIRST_BYTE;
}

uint8_t FECModem::buffer_available()
{
//   XXX this reset implementation is _completely_ broken
//	if (newTransmission())
//		hammingState = FIRST_BYTE;
	if (this->Modem::buffer_available() >= 3)
		return 2;
	if (hammingState == SECOND_BYTE)
		return 1;
	return 0;
}

uint8_t FECModem::buffer_get()
{
	uint8_t byte1, parity;
	if (hammingState == SECOND_BYTE) {
		hammingState = FIRST_BYTE;
		return buf_byte;
	}
	hammingState = SECOND_BYTE;
	byte1 = this->Modem::buffer_get();
	buf_byte = this->Modem::buffer_get();
	parity = this->Modem::buffer_get();

	hamming2416(&byte1, &buf_byte, parity);

	return byte1;
}

FECModem modem;

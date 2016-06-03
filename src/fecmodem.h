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

#ifndef FECMODEM_H_
#define FECMODEM_H_

#include "hamming.h"
#include "modem.h"

/**
 * Receive-only modem with forward error correction.
 * Uses the Modem class to read raw modem data and uses the Hamming 2416
 * algorithm to detect and, if possible, correct transmission errors.
 * Exposes a global modem object for convenience.
 */
class FECModem : public Modem {
	private:
		enum HammingState : uint8_t {
			FIRST_BYTE,
			SECOND_BYTE
		};
		HammingState hammingState;
		uint8_t buf_byte;

		uint8_t parity128(uint8_t byte);
		uint8_t parity2416(uint8_t byte1, uint8_t byte2);
		uint8_t correct128(uint8_t *byte, uint8_t parity);
		uint8_t hamming2416(uint8_t *byte1, uint8_t *byte2, uint8_t parity);
	public:
		FECModem() : Modem() {};

		/**
		 * Enable the modem. Resets the internal Hamming state and calls
		 * Modem::enable().
		 */
		void enable(void);

		/**
		 * Checks if there are unprocessed bytes in the receive buffer.
		 * Parity bytes are accounted for, so if three raw bytes (two data,
		 * one parity) were received by Modem::receive(), this function will
		 * return 2.
		 * @return number of unprocessed data bytes
		 */
		uint8_t buffer_available(void);

		/**
		 * Get next byte from the receive buffer.
		 * @return received byte (0 if it contained uncorrectable errors
		 *         or the buffer is empty)
		 */
		uint8_t buffer_get(void);
};

extern FECModem modem;

#endif /* FECMODEM_H_ */

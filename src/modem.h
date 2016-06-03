/* Name: modem.h
 * Author: Jari Tulilahti
 * Copyright: 2014 Rakettitiede Oy and 2016 Daniel Friesel
 * License: LGPLv3, see COPYING, and COPYING.LESSER -files for more info
 */

#ifndef MODEM_H_
#define MODEM_H_

#include <avr/interrupt.h>
#include <stdlib.h>

/* Modem ring buffer size must be power of 2 */
#define MODEM_BUFFER_SIZE	64

/* Modem defines */
#define MODEM_SYNC_LEN		42
#define MODEM_TIMER		TCNT1L
#define MODEM_PCINT		PCINT24
#define MODEM_PCMSK		PCMSK3
#define MODEM_PCIE		PCIE3
#define MODEM_PIN		PA0
#define MODEM_DDR		DDRA

/**
 * Receive-only modem. Sets up a pin change interrupt on the modem pin
 * and receives bytes using a simple protocol. Does not detect or correct
 * transmission errors.
 */
class Modem {
	private:
		uint8_t buffer_head;
		uint8_t buffer_tail;
		uint8_t buffer[MODEM_BUFFER_SIZE];
		bool new_transmission;
		void buffer_put(const uint8_t c);
	public:
		Modem() {new_transmission = false;};

		/**
		 * Checks if a new transmission was started since the last call
		 * to this function. Returns true if that is the case and false
		 * otherwise.
		 * @return true if a new transmission was started
		 */
		bool newTransmission();

		/**
		 * Checks if there are unprocessed bytes in the modem receive buffer.
		 * @return number of unprocessed bytes
		 */
		uint8_t buffer_available(void);

		/**
		 * Get next byte from modem receive buffer.
		 * @return next unprocessed byte (0 if the buffer is empty)
		 */
		uint8_t buffer_get(void);

		/**
		 * Enable the modem. Turns on the input voltage divider on MODEM_PIN
		 * and enables the receive interrupt (MODEM_PCINT).
		 */
		void enable(void);

		/**
		 * Disable the modem. Disables the receive interrupt and turns off
		 * the input voltage divider on MODEM_PIN.
		 */
		void disable(void);

		/**
		 * Called by the pin change interrupt service routine whenever the
		 * modem pin is toggled. Detects sync pulses, receives bits and
		 * stores complete bytes in the buffer.
		 *
		 * Do not call this function yourself.
		 */
		void receive(void);
};

#endif /* MODEM_H_ */

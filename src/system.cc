/* Name: system.cc
 * Version: 2.0
 * Copyright (C) 2016 by Daniel Friesel
 *
 * Modifications for V2.0 (sine-wave-based transmission and ADC/sampling) 
 * by Chris Veigl, Overflo, Chris Hager 
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>

#include "display.h"
#include "fecmodem.h"
#include "storage.h"
#include "system.h"
#include "static_patterns.h"

#define SHUTDOWN_THRESHOLD 2048

System rocket;

animation_t active_anim;

uint8_t disp_buf[132]; // 4 byte header + 128 byte data
uint8_t *rx_buf = disp_buf + sizeof(disp_buf) - 33;

void System::initialize()
{
	// dito
	wdt_disable();

	// Enable pull-ups on PC3 and PC7 (button pins)
	PORTC |= _BV(PC3) | _BV(PC7);

	display.enable();
	modem.enable();
	storage.enable();

	//storage.reset();
	//storage.save((uint8_t *)"\x10\x0a\x11\x00nootnoot");
	//storage.save((uint8_t *)"\x10\x09\x20\x00" "fnordor");
	//storage.save((uint8_t *)"\x10\x05\x20\x00 \x01 ");
	//storage.save((uint8_t *)"\x20\x22\x08\x02"
	//		"\x00\x04\x22\x02\x22\x04\x00\x00"
	//		"\x00\x00\x00\x00\x00\x00\x00\x00"
	//		"\x00\x04\x22\x02\x22\x04\x00\x00"
	//		"\x00\x00\x00\x00");
	//storage.append((uint8_t *)"\x00\x00\x00\x00");

	sei();

	current_anim_no = 0;

        loadPattern_P(turnonPattern);

	//loadPattern(0);
}

void System::loadPattern_P(const uint8_t *pattern_ptr)
{
	uint8_t i;

	for (i = 0; i < 4; i++)
		disp_buf[i] = pgm_read_byte(pattern_ptr + i);

	for (i = 0; i < disp_buf[1]; i++)
		disp_buf[i+4] = pgm_read_byte(pattern_ptr + i + 4);

	loadPattern_buf(disp_buf);
}

void System::loadPattern_buf(uint8_t *pattern)
{
	active_anim.type = (AnimationType)(pattern[0] >> 4);
	active_anim.length = (pattern[0] & 0x0f) << 8;
	active_anim.length += pattern[1];

	if (active_anim.type == AnimationType::TEXT) {
		active_anim.speed = 250 - (pattern[2] & 0xf0);
		active_anim.delay = (pattern[2] & 0x0f );
		active_anim.direction = pattern[3] >> 4;
		active_anim.repeat = (pattern[3] & 0x0f);
	} else if (active_anim.type == AnimationType::FRAMES) {
		active_anim.speed = 250 - ((pattern[2] & 0x0f) << 4);
		active_anim.delay = pattern[3] >> 4;
		active_anim.direction = 0;
		active_anim.repeat = (pattern[3] & 0x0f);
	}

	active_anim.data = pattern + 4;
	display.show(&active_anim);
}

void System::loadPattern(uint8_t anim_no)
{
	if (storage.hasData()) {
		storage.load(anim_no, disp_buf);
		loadPattern_buf(disp_buf);
	} else {
		loadPattern_P(emptyPattern);
	}
}

void System::receive(void)
{
	static uint8_t rx_pos = 0;
	static uint16_t remaining_bytes = 0;
	uint8_t rx_byte = modem.buffer_get();

	/*
	 * START* and PATTERN* are sync signals, everything else needs to be
	 * stored on the EEPROM.
	 * (Note that the C++ standard guarantees "rxExpect > PATTERN2" to match
	 * for HEADER*, META* and DATA since they are located after PATTERN2
	 * in the RxExpect enum declaration)
	 */
	if (rxExpect > PATTERN2) {
		rx_buf[rx_pos++] = rx_byte;
		/*
		 * HEADER and META are not included in the length
		 * -> only count bytes for DATA.
		 */
		if (rxExpect > META2) {
			remaining_bytes--;
		}
	}
	
	
	// parser for new V2.0 protocol, see /docs/blinkenrocket_debugging.pdf
	switch(rxExpect) {
		case START1:
			if (rx_byte == BYTE_START1) { 
				rxExpect = START2;
			}
			break;
		case START2:
			if (rx_byte == BYTE_START2) {
				// PORTC ^= _BV(PC2);   // indicate frame start detection
				rxExpect = NEXT_BLOCK;
				storage.reset();
				loadPattern_P(flashingPattern);
				MCUSR &= ~_BV(WDRF);
				cli();
				// watchdog interrupt after 4 seconds
				WDTCSR = _BV(WDCE) | _BV(WDE);
				WDTCSR = _BV(WDIE) | _BV(WDP3);
				sei();
				} else {
				if (rx_byte == BYTE_START1)  rxExpect = START2;
				else rxExpect = START1;
			}
			break;
		case NEXT_BLOCK:
			if (rx_byte == BYTE_PATTERN1)
			rxExpect = PATTERN2;
			else if (rx_byte == BYTE_END) {
				// PORTC ^= _BV(PC2);   // indicate frame end detection 
				storage.sync();
				current_anim_no = 0;
				loadPattern(0);
				rxExpect = START1;
				wdt_disable();
				modem.buffer_clear();   // added to avoid mess with framing bytes
			} else rxExpect = START1;
			break;
		case PATTERN2:
			if (rx_byte == BYTE_PATTERN2) {
				rxExpect = HEADER1;
				rx_pos = 0;
			}
			else rxExpect = START1;
			break;
		case HEADER1:
			rxExpect = HEADER2;
			remaining_bytes = (rx_byte & 0x0f) << 8;
			break;
		case HEADER2:
			rxExpect = META1;
			remaining_bytes += rx_byte;
			wdt_reset();
			break;
		case META1:
			rxExpect = META2;
			break;
		case META2:
			rxExpect = DATA_FIRSTBLOCK;
			// skip empty patterns (would bork because of remaining_bytes otherwise)
			if (remaining_bytes == 0)
			rxExpect = NEXT_BLOCK;
			break;
		case DATA_FIRSTBLOCK:
			if (remaining_bytes == 0) {
				rxExpect = NEXT_BLOCK;
				storage.save(rx_buf);
				} else if (rx_pos == 32) {
				rxExpect = DATA;
				rx_pos = 0;
				storage.save(rx_buf);
			}
			break;
		case DATA:
			if (remaining_bytes == 0) {
				rxExpect = NEXT_BLOCK;
				storage.append(rx_buf);
				} else if (rx_pos == 32) {
				rx_pos = 0;
				storage.append(rx_buf);
				wdt_reset();
			}
			break;
		default: rxExpect=START1;
		break;
	}
}



void System::loop()
{
	// First, check for a shutdown request (long press on both buttons)
	if ((PINC & (_BV(PC3) | _BV(PC7))) == 0) {
		/*
		 * Naptime!
		 * (But not before both buttons have been pressed for at least
		 * SHUTDOWN_THRESHOLD * 0.256 ms)
		 */
		if (want_shutdown < SHUTDOWN_THRESHOLD) {
			want_shutdown++;
		}
		else {
			shutdown();
			want_shutdown = 0;
		}
	}
	else {
		want_shutdown = 0;
	}

	if (btn_debounce == 0) {
		if ((PINC & _BV(PC3)) == 0) {
			btnMask = (ButtonMask)(btnMask | BUTTON_RIGHT);
		}
		if ((PINC & _BV(PC7)) == 0) {
			btnMask = (ButtonMask)(btnMask | BUTTON_LEFT);
		}
		/*
		* Only handle button presses when they are released to avoid
		* double actions, such as switching to the next/previous pattern
		* when the user actually wants to press the shutdown combo.
		*/
		if ((PINC & (_BV(PC3) | _BV(PC7))) == (_BV(PC3) | _BV(PC7))) {
			cli();
			if (btnMask == BUTTON_RIGHT) {
				current_anim_no = (current_anim_no + 1) % storage.numPatterns();
				loadPattern(current_anim_no);
			} else if (btnMask == BUTTON_LEFT) {
				if (current_anim_no == 0)
					current_anim_no = storage.numPatterns() - 1;
				else
					current_anim_no--;
				loadPattern(current_anim_no);
			}
			btnMask = BUTTON_NONE;
			sei();
			/*
			 * Ignore keypresses for 25ms to work around bouncing buttons
			 */
			btn_debounce = 100;
		}
	} else {
		btn_debounce--;
	}

	while (modem.buffer_available()) {
		receive();
	}

	display.update();
}

void System::shutdown()
{
	uint8_t i;

	modem.disable();

	// show power down image
	loadPattern_P(shutdownPattern);

	// wait until both buttons are released
	while (!((PINC & _BV(PC3)) && (PINC & _BV(PC7))))
		display.update();

	// and some more to debounce the buttons (and finish powerdown animation)
	for (i = 0; i < 100; i++) {
		display.update();
		_delay_ms(1);
	}

	// turn off display to indicate we're about to shut down
	display.disable();

	// disable ADC to save power
	PRR |= _BV(PRADC); 

	// actual naptime

	// enable PCINT on PC3 (PCINT11) and PC7 (PCINT15) for wakeup
	PCMSK1 |= _BV(PCINT15) | _BV(PCINT11);
	PCICR |= _BV(PCIE1);
	
	// go to power-down mode
	SMCR = _BV(SM1) | _BV(SE);
	asm("sleep");

	// execution will resume here - disable PCINT again.
	// Don't disable PCICR, something else might need it.
	PCMSK1 &= ~(_BV(PCINT15) | _BV(PCINT11));

	// turn on display
	loadPattern(current_anim_no);
	display.enable();

	/*
	 * Wait for wakeup button(s) to be released to avoid accidentally
	 * going back to sleep again or switching the active pattern.
	 */
	while (!((PINC & _BV(PC3)) && (PINC & _BV(PC7))))
		display.update();

	// debounce
	for (i = 0; i < 100; i++) {
		display.update();
		_delay_ms(1);
	}

	// enable the ADC !
	PRR &= ~_BV(PRADC); 

	// finally, turn on the modem...
	modem.enable();

	// ... and reset the receive state machine
	rxExpect = START1;
}

void System::handleTimeout()
{
	modem.disable();
	modem.buffer_clear();   // added to avoid mess with framing bytes
	modem.enable();
	rxExpect = START1;
	current_anim_no = 0;
	loadPattern_P(timeoutPattern);
}

ISR(PCINT1_vect)
{
	// we use PCINT1 for wakeup, so we need an (empty) ISR for it
}

ISR(WDT_vect)
{
	/*
	 * Modem transmission was interrupted without END byte. Reset state
	 * machine and show timeout message.
	 */
	wdt_disable();
	rocket.handleTimeout();
}

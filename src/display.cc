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
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdlib.h>

#include "display.h"
#include "font.h"
#include "storage.h"

Display display;

Display::Display()
{
	char_pos = -1;
}

void Display::disable()
{
	TIMSK0 &= ~_BV(TOIE0);
	PORTB = 0;
	PORTD = 0;
}

void Display::enable()
{
	// Ports B and D drive the dot matrix display -> set all as output
	DDRB = 0xff;
	DDRD = 0xff;

	// Enable 8bit counter with prescaler=8 (-> timer frequency = 1MHz)
	TCCR0A = _BV(CS01);
	// raise timer interrupt on counter overflow (-> interrupt frequency = ~4kHz)
	TIMSK0 = _BV(TOIE0);
}

void Display::multiplex()
{
	/*
	 * To avoid flickering, do not put any code (or expensive index
	 * calculations) between the following three lines.
	 */
	PORTB = 0;
	PORTD = disp_buf[active_col];
	PORTB = _BV(active_col);

	if (++active_col == 8) {
		active_col = 0;
		if (++update_cnt == update_threshold) {
			update_cnt = 0;
			need_update = 1;
		}
	}
}

void Display::update() {
	uint8_t i, glyph_len;
	uint8_t *glyph_addr;
	if (need_update) {
		need_update = 0;

		if (status == RUNNING) {
			if (current_anim->type == AnimationType::TEXT) {

				/*
				 * Scroll display contents to the left/right
				 */
				if (current_anim->direction == 0) {
					for (i = 0; i < 7; i++) {
						disp_buf[i] = disp_buf[i+1];
					}
				} else if (current_anim->direction == 1) {
					for (i = 7; i > 0; i--) {
						disp_buf[i] = disp_buf[i-1];
					}
				}

				/*
				 * Load current character
				 */
				glyph_addr = (uint8_t *)pgm_read_ptr(&font[current_anim->data[str_pos]]);
				glyph_len = pgm_read_byte(&glyph_addr[0]);
				char_pos++;

				if (char_pos > glyph_len) {
					char_pos = 0;
					if (current_anim->direction == 0)
						str_pos++;
					else
						str_pos--; // may underflow, but that's okay
				}

				/*
				 * Append one character column (or whitespace if we are
				 * between two characters)
				 */
				if (current_anim->direction == 0) {
					if (char_pos == 0) {
						disp_buf[7] = 0xff; // whitespace
					} else {
						disp_buf[7] = ~pgm_read_byte(&glyph_addr[char_pos]);
					}
				} else {
					if (char_pos == 0) {
						disp_buf[0] = 0xff; // whitespace
					} else {
						disp_buf[0] = ~pgm_read_byte(&glyph_addr[glyph_len - char_pos + 1]);
					}
				}

			} else if (current_anim->type == AnimationType::FRAMES) {
				for (i = 0; i < 8; i++) {
					disp_buf[i] = ~current_anim->data[str_pos+i];
				}
				str_pos += 8;
			}

			if (current_anim->direction == 0) {
				/*
				 * Check whether we reached the end of the pattern
				 * (that is, we're in the last chunk and reached the
				 * remaining pattern length)
				 */
				if ((str_chunk == ((current_anim->length - 1) / 128))
						&& (str_pos > ((current_anim->length - 1) % 128))) {
					str_chunk = 0;
					str_pos = 0;
					if (current_anim->delay > 0) {
						status = PAUSED;
						update_threshold = 244;
					}
					if (current_anim->length > 128) {
						storage.loadChunk(str_chunk, current_anim->data);
					}
				/*
				 * Otherwise, check whether the pattern is split into
				 * several chunks and we reached the end of the chunk
				 * kept in current_anim->data
				 */
				} else if ((current_anim->length > 128) && (str_pos >= 128)) {
					str_pos = 0;
					str_chunk++;
					storage.loadChunk(str_chunk, current_anim->data);
				}
			} else {
				/*
				 * In this branch we keep doing str_pos--, so check for
				 * underflow
				 */
				if (str_pos >= 128) {
					/*
					 * Check whether we reached the end of the pattern
					 * (and whether we need to load a new chunk)
					 */
					if (str_chunk == 0) {
						if (current_anim->length > 128) {
							str_chunk = (current_anim->length - 1) / 128;
							storage.loadChunk(str_chunk, current_anim->data);
						}
						if (current_anim->delay > 0) {
							str_pos = 0;
							status = PAUSED;
							update_threshold = 244;
						} else {
							str_pos = (current_anim->length - 1) % 128;
						}
					/*
					 * Otherwise, we reached the end of the active chunk
					 */
					} else {
						str_chunk--;
						storage.loadChunk(str_chunk, current_anim->data);
						str_pos = 127;
					}
				}
			}
		} else if (status == PAUSED) {
			str_pos++;
			if (str_pos >= current_anim->delay) {
				if (current_anim->direction == 0)
					str_pos = 0;
				else if (current_anim->length <= 128)
					str_pos = current_anim->length - 1;
				else
					str_pos = (current_anim->length - 1) % 128;
				status = RUNNING;
				update_threshold = current_anim->speed;
			}
		}
	}
}

void Display::reset()
{
	for (uint8_t i = 0; i < 8; i++)
		disp_buf[i] = 0xff;
	update_cnt = 0;
	str_pos = 0;
	str_chunk = 0;
	char_pos = -1;
	need_update = 1;
	status = RUNNING;
}

void Display::show(animation_t *anim)
{
	current_anim = anim;
	reset();
	update_threshold = current_anim->speed;
	if (current_anim->direction == 1) {
		if (current_anim->length > 128) {
			str_chunk = (current_anim->length - 1) / 128;
			storage.loadChunk(str_chunk, current_anim->data);
		}
		str_pos = (current_anim->length - 1) % 128;
	}
}

/*
 * Current configuration:
 * One interrupt per 256 microseconds. The whole display is refreshed every
 * 2048us, giving a refresh rate of ~500Hz
 */
ISR(TIMER0_OVF_vect)
{
	display.multiplex();
}

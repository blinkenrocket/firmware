/*
 * Copyright (C) 2016 by Daniel Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#include <util/delay.h>
#include <avr/io.h>
#include <stdlib.h>

#include "storage.h"

Storage storage;

/*
 * EEPROM data structure ("file system"):
 *
 * Organized as 32B-pages, all animations/texts are page-aligned.  Byte 0 ..
 * 255 : storage metadata. Byte 0 contains the number of animations, byte 1 the
 * page offset of the first animation, byte 2 of the second, and so on.
 * Byte 256+: texts/animations without additional storage metadata, aligned
 * to 32B. So, a maximum of 256-(256/32) = 248 texts/animations can be stored,
 * and a maximum of 255 * 32 = 8160 Bytes (almost 8 kB / 64 kbit) can be
 * addressed.  To support larger EEPROMS, change the metadate area to Byte 2 ..
 * 511 and use 16bit page pointers.
 *
 * The text/animation size is not limited by this approach.
 *
 * Example:
 * Byte     0 = 3 -> we've got a total of three animations
 * Byte     1 = 0 -> first text/animation starts at byte 256 + 32*0 = 256
 * Byte     2 = 4 -> second starts at byte 256 + 32*4 = 384
 * Byte     3 = 5 -> third starts at 256 + 32*5 * 416
 * Byte     4 = whatever
 *            .
 *            .
 *            .
 * Byte 256ff = first text/animation. Has a header encoding its length in bytes.
 * Byte 384ff = second
 * Byte 416ff = third
 *            .
 *            .
 *            .
 */

void Storage::enable()
{
	/*
	 * Set I2C clock frequency to 100kHz.
	 * freq = F_CPU / (16 + (2 * TWBR * TWPS) )
	 * let TWPS = "00" = 1
	 * -> TWBR = (F_CPU / 100000) - 16 / 2
	 */
	TWSR = 0; // the lower two bits control TWPS
	TWBR = ((F_CPU / 100000UL) - 16) / 2;

	i2c_read(0, 0, 1, &num_anims);
}


/*
 * Send an I2C (re)start condition and the EEPROM address in read mode. Returns
 * after it has been transmitted successfully.
 */
uint8_t Storage::i2c_start_read()
{
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));
	if (!(TWSR & 0x18)) // 0x08 == START ok, 0x10 == RESTART ok
		return I2C_START_ERR;

	// Note: The R byte ("... | 1") causes the TWI momodule to switch to
	// Master Receive mode
	TWDR = (I2C_EEPROM_ADDR << 1) | 1;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));
	if (TWSR != 0x40) // 0x40 == SLA+R transmitted, ACK receveid
		return I2C_ADDR_ERR;

	return I2C_OK;
}

/*
 * Send an I2C (re)start condition and the EEPROM address in write mode.
 * Returns after it has been transmitted successfully.
 */
uint8_t Storage::i2c_start_write()
{
	TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));
	if (!(TWSR & 0x18)) // 0x08 == START ok, 0x10 == RESTART ok
		return I2C_START_ERR;

	TWDR = (I2C_EEPROM_ADDR << 1) | 0;
	TWCR = _BV(TWINT) | _BV(TWEN);
	while (!(TWCR & _BV(TWINT)));
	if (TWSR != 0x18) // 0x18 == SLA+W transmitted, ACK received
		return I2C_ADDR_ERR;

	return I2C_OK;
}

/*
 * Send an I2C stop condition.
 */
void Storage::i2c_stop()
{
	TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
}

/*
 * Sends len bytes to the EEPROM. Note that this method does NOT
 * send I2C start or stop conditions.
 */
uint8_t Storage::i2c_send(uint8_t len, uint8_t *data)
{
	uint8_t pos = 0;

	for (pos = 0; pos < len; pos++) {
		TWDR = data[pos];
		TWCR = _BV(TWINT) | _BV(TWEN);
		while (!(TWCR & _BV(TWINT)));
		if (TWSR != 0x28) // 0x28 == byte transmitted, ACK received
			return pos;
	}

	return pos;
}

/*
 * Receives len bytes from the EEPROM into data. Note that this method does
 * NOT send I2C start or stop conditions.
 */
uint8_t Storage::i2c_receive(uint8_t len, uint8_t *data)
{
	uint8_t pos = 0;

	for (pos = 0; pos < len; pos++) {
		if (pos == len-1) {
			// Don't ACK the last byte
			TWCR = _BV(TWINT) | _BV(TWEN);
		} else {
			// Automatically send ACK
			TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
		}
		while (!(TWCR & _BV(TWINT)));
		data[pos] = TWDR;
		/*
		 * No error handling here -- We send the acks, the EEPROM only
		 * supplies raw data, so there's no way of knowing whether it's still
		 * talking to us or we're just reading garbage.
		 */
	}

	return pos;
}

/*
 * Writes len bytes of data into the EEPROM, starting at byte number pos.
 * Does not check for page boundaries.
 * Includes a complete I2C transaction.
 */
uint8_t Storage::i2c_write(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
	uint8_t addr_buf[2];
	uint8_t num_tries;

	addr_buf[0] = addrhi;
	addr_buf[1] = addrlo;

	/*
	 * The EEPROM might be busy processing a write command, which can
	 * take up to 10ms. Wait up to 16ms to respond before giving up.
	 * All other error conditions (even though they should never happen[tm])
	 * are handled the same way.
	 */
	for (num_tries = 0; num_tries < 32; num_tries++) {
		if (num_tries > 0)
			_delay_us(500);

		if (i2c_start_write() != I2C_OK)
			continue; // EEPROM is busy writing

		if (i2c_send(2, addr_buf) != 2)
			continue; // should not happen

		if (i2c_send(len, data) != len)
			continue; // should not happen

		i2c_stop();
		return I2C_OK;
	}

	i2c_stop();
	return I2C_ERR;
}

/*
 * Reads len bytes of data from the EEPROM, starting at byte number pos.
 * Does not check for page boundaries.
 * Includes a complete I2C transaction.
 */
uint8_t Storage::i2c_read(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data)
{
	uint8_t addr_buf[2];
	uint8_t num_tries;

	addr_buf[0] = addrhi;
	addr_buf[1] = addrlo;

	/*
	 * See comments in i2c_write.
	 */
	for (num_tries = 0; num_tries < 32; num_tries++) {
		if (num_tries > 0)
			_delay_us(500);

		if (i2c_start_write() != I2C_OK)
			continue; // EEPROM is busy writing

		if (i2c_send(2, addr_buf) != 2)
			continue; // should not happen

		if (i2c_start_read() != I2C_OK)
			continue; // should not happen

		if (i2c_receive(len, data) != len)
			continue; // should not happen

		i2c_stop();
		return I2C_OK;
	}

	i2c_stop();
	return I2C_ERR;
}

void Storage::reset()
{
	first_free_page = 0;
	num_anims = 0;
}

void Storage::sync()
{
	i2c_write(0, 0, 1, &num_anims);
}

bool Storage::hasData()
{
	// Unprogrammed EEPROM pages always read 0xff
	if (num_anims == 0xff)
		return false;
	return num_anims;
}

void Storage::load(uint8_t idx, uint8_t *data)
{
	i2c_read(0, 1 + idx, 1, &page_offset);

	/*
	 * Unconditionally read 132 bytes. The data buffer must hold at least
	 * 132 bytes anyways, and this way we can save one I2C transaction. If
	 * there is any speed penalty cause by this I wasn't able to notice it.
	 * Also note that the EEPROM automatically wraps around when the end of
	 * memory is reached, so this edge case doesn't need to be accounted for.
	 */
	i2c_read(1 + (page_offset / 8), (page_offset % 8) * 32, 132, data);
}

void Storage::loadChunk(uint8_t chunk, uint8_t *data)
{
	uint8_t this_page_offset = page_offset + (4 * chunk);

	// Note that we do not load headers here -> 128 instead of 132 bytes
	i2c_read(1 + (this_page_offset / 8), (this_page_offset % 8) * 32 + 4, 128, data);
}

void Storage::save(uint8_t *data)
{
	/*
	 * Technically, we can store up to 255 patterns. However, Allowing
	 * 255 patterns (-> num_anims = 0xff) means we can't easily
	 * distinguish between an EEPROM with 255 patterns and a factory-new
	 * EEPROM (which just reads 0xff everywhere). So only 254 patterns
	 * are allowed.
	 */
	if (num_anims < 254) {
		/*
		* Bytes 0 .. 255 (pages 0 .. 7) are reserved for storage metadata,
		* first_free_page counts pages starting at byte 256 (page 8).
		* So, first_free_page == 247 addresses EEPROM bytes 8160 .. 8191.
		* first_free_page == 248 would address bytes 8192 and up, which don't
		* exist -> don't save anything afterwards.
		*
		* Note that at the moment (stored patterns are aligned to page
		* boundaries) this means we can actually only store up to 248
		* patterns.
		*/
		if (first_free_page < 248) {
			num_anims++;
			i2c_write(0, num_anims, 1, &first_free_page);
			append(data);
		}
	}
}

void Storage::append(uint8_t *data)
{
	// see comment in Storage::save()
	if (first_free_page < 248) {
		// the header indicates the length of the data, but we really don't care
		// - it's easier to just write the whole page and skip the trailing
		// garbage when reading.
		i2c_write(1 + (first_free_page / 8), (first_free_page % 8) * 32, 32, data);
		first_free_page++;
	}
}

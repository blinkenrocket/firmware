/*
 * Copyright (C) 2016 by Daniel Friesel
 *
 * License: You may use, redistribute and/or modify this file under the terms
 * of either:
 * * The GNU LGPL v3 (see COPYING and COPYING.LESSER), or
 * * The 3-clause BSD License (see COPYING.BSD)
 *
 */

#include <stdlib.h>

#define I2C_EEPROM_ADDR 0x50

class Storage {
	private:
		/**
		 * Number of animations on the storage, AKA contents of byte 0x0000.
		 * A value of 0xff indicates that the EEPROM was never written to
		 * and therefore contains no animations.
		 */
		uint8_t num_anims;

		/**
		 * Page offset of the pattern read by the last load() call. Used to
		 * calculate the read address in loadChunk(). The animation this
		 * offset refers to starts at byte 256 + (32 * page_offset).
		 */
		uint8_t page_offset;

		/**
		 * First free page (excluding the 256 byte metadata area) on the
		 * EEPROM. Used by save() and append(). This value refers to the
		 * EEPROM bytes 256 + (32 * first_free_page) and up.
		 */
		uint8_t first_free_page;

		enum I2CStatus : uint8_t {
			I2C_OK,
			I2C_START_ERR,
			I2C_ADDR_ERR,
			I2C_ERR
		};

		/**
		 * Sends an I2C start condition and the I2C_EEPROM_ADDR with the
		 * write flag set.
		 *
		 * @return An I2CStatus value indicating success/failure
		 */
		uint8_t i2c_start_write(void);

		/**
		 * Sends an I2C start condition and the I2C_EEPROM_ADDR with the
		 * read flag set.
		 *
		 * @return An I2CStatus value indicating success/failure
		 */
		uint8_t i2c_start_read(void);

		/**
		 * Sends an I2C stop condition. Does not check for errors.
		 */
		void i2c_stop(void);

		/**
		 * Sends up to len bytes of data via I2C. i2c_start_write() must
		 * have been called successfully for this to work.
		 *
		 * @param len number of bytes to send
		 * @param data pointer to data buffer, must be at least len bytes
		 * @return number of bytes which were successfully sent (0 .. len)
		 */
		uint8_t i2c_send(uint8_t len, uint8_t *data);

		/**
		 * Receives up to len bytes of data via I2C. i2c_start_read() must
		 * have been called successfully for this to work.
		 *
		 * @param len number of bytes to receive
		 * @param data pointer to data buffer, must be at least len bytes
		 * @return number of bytes which were successfully received (0 .. len)
		 */
		uint8_t i2c_receive(uint8_t len, uint8_t *data);

		/**
		 * Reads len bytes of data stored on addrhi, addrlo from the EEPROM
		 * into the data buffer. Does a complete I2C transaction including
		 * start and stop conditions. addrlo may start at an arbitrary
		 * position, page boundaries are irrelevant for this function. If a
		 * read reaches the end of the EEPROM memory, it will wrap around to
		 * byte 0x0000.
		 *
		 * @param addrhi upper address byte. Must be less than 32
		 * @param addrlo lower address byte
		 * @param len number of bytes to read
		 * @param data pointer to data buffer, must be at least len bytes
		 * @return An I2CStatus value indicating success/failure
		 */
		uint8_t i2c_read(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);

		/**
		 * Writes len bytes of data from the data buffer into addrhi, addrlo
		 * on the EEPROM. Does a complete I2C transaction including start and
		 * stop conditions. Note that the EEPROM consists of 32 byte pages,
		 * so it is not possible to write more than 32 bytes in one
		 * operation.  Also note that this function does not check for page
		 * boundaries.  Starting a 32-byte write in the middle of a page will
		 * put the first 16 bytes where they belong, while the second 16
		 * bytes will wrap around to the first 16 bytes of the page.
		 *
		 * @param addrhi upper address byte. Must be less than 32
		 * @param addrlo lower address byte
		 * @param len number of bytes to write
		 * @param data pointer to data buffer, must be at least len bytes
		 * @return An I2CStatus value indicating success/failure
		 */
		uint8_t i2c_write(uint8_t addrhi, uint8_t addrlo, uint8_t len, uint8_t *data);

	public:
		Storage() { num_anims = 0; first_free_page = 0;};

		/**
		 * Enables the storage hardware: Configures the internal I2C
		 * module and reads num_anims from the EEPROM.
		 */
		void enable();

		/**
		 * Prepares the storage for a complete overwrite by setting the
		 * number of stored animations to zero. The next save operation
		 * will get pattern id 0 and overwrite the first stored pattern.
		 *
		 * Note that this function does not write anything to the
		 * EEPROM. Use Storage::sync() for that.
		 */
		void reset();

		/**
		 * Writes the current number of animations (as set by reset() or
		 * save() to the EEPROM. Required to get a consistent storage state
		 * after a power cycle.
		 */
		void sync();

		/**
		 * Checks whether the EEPROM contains animathion data.
		 *
		 * @return true if the EEPROM contains valid-looking data
		 */
		bool hasData();

		/**
		 * Accessor for the number of saved patterns on the EEPROM.
		 * A return value of 255 (0xff) means that there are no patterns
		 * on the EEPROM (and hasData() returns false in that case).
		 *
		 * @return number of patterns
		 */
		uint8_t numPatterns() { return num_anims; };

		/**
		 * Loads pattern number idx from the EEPROM. The 
		 *
		 * @param idx pattern index (starting with 0)
		 * @param pointer to the data structure for the pattern. Must be
		 *        at least 132 bytes
		 */
		void load(uint8_t idx, uint8_t *data);

		/**
		 * Load partial pattern chunk (without header) from EEPROM.
		 *
		 * @param chunk 128 byte-offset inside pattern (starting with 0)
		 * @param data pointer to data structure for the pattern. Must be
		 *        at least 128 bytes
		 */
		void loadChunk(uint8_t chunk, uint8_t *data);

		/**
		 * Save (possibly partial) pattern on the EEPROM. 32 bytes of
		 * dattern data will be read and stored, regardless of the
		 * pattern header.
		 *
		 * @param data pattern data. Must be at least 32 bytes
		 */
		void save(uint8_t *data);

		/**
		 * Continue saving a pattern on the EEPROM. Appends 32 bytes of
		 * pattern data after the most recently written block of data
		 * (i.e., to the pattern which is currently being saved).
		 *
		 * @param data pattern data. Must be at least 32 bytes
		 */
		void append(uint8_t *data);
};

extern Storage storage;

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

/**
 * Describes the type of an animation object. The Storage class reserves four
 * bits for the animation type, so up to 16 types are supported.
 */
enum class AnimationType : uint8_t {
	TEXT = 1,
	FRAMES = 2
};

/**
 * Generic struct for anything which can be displayed, e.g. texts or
 * sequences of frames.
 */
struct animation {
	/**
	 * Type of patern/animation described in this struct. Controls the
	 * behaviour of Display::multiplex() and Display::update().
	 */
	AnimationType type;

	/**
	 * Length of animation in bytes. Also controls the behaviour of
	 * Display::update().
	 *
	 * For length <= 128, the whole animation is stored in data and
	 * Display::update() simply traverses the data array.
	 *
	 * For length > 128, only up to 128 bytes of animation data are stored
	 * in data. When Display::update() reaches the end of the array, it will
	 * use Storage::loadChunk() to load the next 128 byte chunk of animation
	 * data into the data array.
	 */
	uint16_t length;

	/**
	 * * If type == AnimationType::TEXT: Text scroll speed in columns per TODO
	 * * If type == AnimationType::FRAMES: Frames per TODO
	 */
	uint8_t speed;

	/**
	 * Delay after the last text symbol / animation frame.
	 */
	uint8_t delay;

	/**
	 * Scroll mode / direction. Must be set to 0 if type != TEXT.
	 */
	uint8_t direction;

	/**
	 * * If type == AnimationType::TEXT: pointer to an arary containing the
	 *   animation text in standard ASCII format (+ special font chars)
	 * * If type == AnimationType::FRAMES: Frame array. Each element encodes
	 *   a display column (starting with the leftmost one), each group of
	 *   eight elements is a frame.
	 *
	 * The data array must always hold at least 128 elements.
	 */
	uint8_t *data;
};

typedef struct animation animation_t;

/**
 * Controls the display. Handles multiplexing, scrolling and supports loading
 * arbitrary animations. Also handles partial animations and chunk loading.
 */
class Display {
	private:

		/**
		 * The currently active animation
		 */
		animation_t *current_anim;

		/**
		 * Internal display update counter. Incremented by multiplex().
		 * update() is called (and the counter reset) whenever
		 * update_cnt == need_update.
		 */
		uint8_t update_cnt;

		/**
		 * Set to a true value by multiplex() if an update (that is,
		 * a scroll step or a new frame) is needed. Checked and reset to
		 * false by update().
		 */
		uint8_t need_update;

		/**
		 * Number of frames after which update() is called. This value
		 * holds either the current animation's speed or its delay.
		 */
		uint8_t update_threshold;

		/**
		 * The currently active column in multiplex()
		 */
		uint8_t active_col;

		/**
		 * The current display content which multiplex() will show
		 */
		uint8_t disp_buf[8];

		/**
		 * The current position inside current_anim->data. For a TEXT
		 * animation, this indicates the currently active character.
		 * In case of FRAMES, it indicates the leftmost column of an
		 * eight-column frame.
		 *
		 * This variable is also used as delay counter for status == PAUSED,
		 * so it must be re-initialized when the pause is over.
		 */
		uint8_t str_pos;

		/**
		 * The currently active animation chunk. For an animation which is
		 * not longer than 128 bytes, this will always read 0. Otherwise,
		 * it indicates the offset in 128 byte-chunks from the start of the
		 * animation which is currently held in memory. So, str_chunk == 1
		 * means current->anim_data contains animation bytes 129 to 256,
		 * and so on. The current position in the complete animation is
		 * str_chunk * 128 + str_pos.
		 */
		uint8_t str_chunk;

		/**
		 * If current_anim->type == TEXT: The column of the character
		 * pointed to by str_pos which was last added to the display.
		 *
		 * For current_anim->direction == 0, this refers to the character's
		 * left border and counts from 0 onwards. so char_pos == 0 means the
		 * leftmost column of the character was last added to the display.
		 *
		 * For current_anim->direction == 1, this refers to the character's
		 * right border and also counts from 0 onwards, so char_pos == 0
		 * means the rightmost column of the character was last added to the
		 * display.
		 */
		int8_t char_pos;

		enum AnimationStatus : uint8_t {
			RUNNING,
			SCROLL_BACK,
			PAUSED
		};

		/**
		 * The current animation status: RUNNING (text/frames are being
		 * displayed) or PAUSED (the display isn't changed until the
		 * delay specified by current_anim->delay has passed)
		 */
		AnimationStatus status;

	public:
		Display();

		/**
		 * Enables the display driver.
		 * Configures ports B and D as output and enables the display
		 * timer and corresponding interrupt.
		 */
		void enable(void);

		/**
		 * Disables the display driver.
		 * Turns off both the display itself and the display timer.
		 */
		void disable(void);

		/**
		 * Draws a single display column. Called every 256 microseconds
		 * by the timer interrupt (TIMER0_OVF_vect), resulting in
		 * a display refresh rate of ~500Hz (one refresh per 2048µs)
		 */
		void multiplex(void);

		/**
		 * Reset display and animation state. Fills the screen with "black"
		 * (that is, no active pixels) and sets the animation offset to zero.
		 */
		void reset(void);

		/**
		 * Update display content.
		 * Checks current_anim->speed and current_anim->type and scrolls
		 * the text / advances a frame when appropriate. Also uses
		 * Storage::loadChunk() to load the next 128 pattern bytes if
		 * current_anim->length is greater than 128 and the end of the
		 * 128 byte pattern buffer is reached.
		 */
		void update(void);

		/**
		 * Sets the active animation to be shown on the display. Automatically
		 * calls reset(). If direction == 1, uses Storage::loadChunk() to
		 * load the last 128 byte-chunk of anim (so that the text can start
		 * scrolling from its last position). If direction == 0, the first
		 * 128 bytes of animation data are expected to already be present in
		 * anim->data.
		 *
		 * @param anim active animation. Note that the data is not copied,
		 *        so anim has to be kept in memory until a new one is loaded
		 */
		void show(animation_t *anim);
};

extern Display display;

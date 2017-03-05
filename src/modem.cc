/* Name: modem.cc
 * Version: 2.0
 *
 * Audio modem for Attiny85 & other AVR chips with modifications
 *
 * Author: Jari Tulilahti
 * 
 * Modifications for V2.0 (sine-wave-based transmission and ADC/sampling) 
 * by Chris Veigl, Overflo, Chris Hager 
 *
 * Copyright: 2014 Rakettitiede Oy and 2016 Daniel Friesel
 * License: LGPLv3, see COPYING, and COPYING.LESSER -files for more info
 */

#include <avr/io.h>
#include <stdlib.h>
#include "modem.h"
#include "fecmodem.h"

extern FECModem modem;


// #define SPI_DBG
// uncomment this for debug output via SPI 
// use arduino-sketch in folder /utilities/SPI_debug to see serial console output

bool Modem::newTransmission()
{
	if (new_transmission) {
		new_transmission = false;
		return true;
	}
	return false;
}

/*
 * Returns number of available bytes in ringbuffer or 0 if empty
 */
uint8_t Modem::buffer_available() {
	return buffer_head - buffer_tail;
}

/*
 * Store 1 byte in ringbuffer
 */
inline void Modem::buffer_put(const uint8_t c) {
	if (buffer_available() != MODEM_BUFFER_SIZE) {
		buffer[buffer_head++ % MODEM_BUFFER_SIZE] = c;
	}
}

/*
 * Fetch 1 byte from ringbuffer
 */
uint8_t Modem::buffer_get() {
	uint8_t b = 0;
	if (buffer_available() != 0) {
		b = buffer[buffer_tail++ % MODEM_BUFFER_SIZE];
	}
	return b;
}


void Modem::buffer_clear() {                 // needed that to avoid mess with framing bytes ....
	buffer_head=buffer_tail=0;
}



/*
 * Start the modem by enabling Pin Change Interrupts & Timer
 */
void Modem::enable()  {
	/* Enable R1 */
	DDRA  |= _BV(PA3);
	PORTA |= _BV(PA3);	
	DDRC |= _BV(PC2);  // use E3 and E2 to indicate bit detection 

	/* configure and enable ADC   */
	ADCSRA = _BV(ADEN) + _BV(ADIE) + _BV(ADPS2) +  _BV(ADPS0) +_BV(ADATE) ; 
	//  ADC prescaler 32 = 250KhZ - actually a bit overclocked ...
	ADMUX = _BV(REFS0) + 6;  // chn6 = PA0 / ADC6
	/*  start free running mode ** */
	ADCSRA |= _BV(ADSC);	

#ifdef SPI_DBG
	TIMSK0 &= ~_BV(TOIE0); // disable Led display! (use SPI for debugging output)
	PORTB=0; PORTD=255;
	PORTA &= ~_BV(PA1);   // slave select enable
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPR1);
#endif

	/* Timer: TCCR1: CS10 and CS11 bits: 8MHz clock with Prescaler 64 = 125kHz timer clock */
	TCCR1B = _BV(CS11) | _BV(CS10);
	/* Modem pin as input */
	MODEM_DDR &= ~_BV(MODEM_PIN);
	/* Enable Pin Change Interrupts and PCINT for MODEM_PIN */
	//MODEM_PCMSK |= _BV(MODEM_PCINT);
	//PCICR |= _BV(MODEM_PCIE);
}

void Modem::disable()
{
	PORTA &= ~_BV(PA3);
	DDRA  &= ~_BV(PA3);

	// disable ADC
	DDRC &= ~ _BV(PC2);
	ADCSRA &= ~ _BV(ADEN);

#ifdef SPI_DBG
	PORTA |= _BV(PA1);  // slave select disable
	SPCR=0;
	TIMSK0 |= _BV(TOIE0); // enable Led display !
#endif

}

void Modem::receive() {
	/* Static variables instead of globals to keep scope inside ISR */
	static uint8_t modem_bit = 0;
	static uint8_t modem_bitlen = 0;
	static uint8_t modem_byte = 0;

	/* Read & Zero Timer/Counter 1 value */
	uint8_t modem_pulselen = MODEM_TIMER;
	MODEM_TIMER = 0;

	/*
	 * Check if we received Start/Sync -pulse.
	 * Calculate bit signal length middle point from pulse.
	 * Return from ISR immediately.
	 */
	if (modem_pulselen > MODEM_SYNC_LEN) {
		modem_bitlen = (modem_pulselen >> 2);
		modem_bit = 0;
		new_transmission = true;
		return;
	}

	/*
	 * Shift byte and set high bit according to the pulse length.
	 * Long pulse = 1, Short pulse = 0
	 */
	modem_byte = (modem_byte >> 1) | (modem_pulselen < modem_bitlen ? 0x00 : 0x80);

	/* Check if we received complete byte and store it in ring buffer */
	if (!(++modem_bit % 0x08)) {
		buffer_put(modem_byte);
	}
}

#define FREQ_NONE 0
#define FREQ_LOW 1
#define FREQ_HIGH 2
#define NUMBER_OF_SAMPLES 8
#define BITLEN_THRESHOLD 6

void Modem::receiveADC() {

    static uint16_t ACTIVITY_THRESHOLD=80;
	static uint8_t modem_bit = 0;
	static uint8_t modem_byte = 0;

	// some variables for sampling / frequency detection
	uint16_t activity;
	static uint8_t bitlength=0, cnt=0;
	static uint16_t sampleValue=512, max_sampleValue=550; 
	static uint16_t prevValue, accu;
	static uint8_t actFrequency=FREQ_NONE, prevFrequency=FREQ_NONE;
	
	prevValue=sampleValue;
	sampleValue=ADC;
	if (sampleValue > max_sampleValue) {
	  max_sampleValue=sampleValue;
	  ACTIVITY_THRESHOLD=50+(max_sampleValue>>3);  //  activity threshold related to max amplitude !
	}
	  
	accu += (sampleValue>prevValue) ? sampleValue-prevValue : prevValue-sampleValue;
	if (++cnt < NUMBER_OF_SAMPLES) return;   // accumulate NUMBER_OF_SAMPLES values

	activity=accu; 
	cnt=0; accu=0;

	if (bitlength<100) bitlength++; 

	if ((activity < ACTIVITY_THRESHOLD) && (bitlength > BITLEN_THRESHOLD<<2)) {    // no active sine wave detected
		prevFrequency=FREQ_NONE;
		max_sampleValue=550;       // reset calculation of max amplitude
		modem_bit = 0;
		modem_byte=0;
		modem.buffer_clear();
		PORTC &= ~ _BV(PC2);      // keep test signal low during idle phase
		return;
	} 

	if (activity >= ACTIVITY_THRESHOLD) 
		actFrequency=FREQ_HIGH;
	else actFrequency=FREQ_LOW;

	if (actFrequency != prevFrequency)  // bit change detected !
	{
		if (prevFrequency!=FREQ_NONE) {   // skip first edge (no valid bitlength yet!)

				modem_byte = (modem_byte >> 1) | (bitlength < BITLEN_THRESHOLD ? 0x00 : 0x80);
				PORTC ^= _BV(PC2);   // show actual bit detection for debugging
				
				// Check if we received complete byte and store it in ring buffer
				if (!(++modem_bit % 0x08)) 
				{
					buffer_put(modem_byte);
					#ifdef SPI_DBG
						SPDR = modem_byte;  // output detected byte to SPI for debugging
					#endif
				}
		}
		prevFrequency=actFrequency;
		bitlength=0;
	}
}


/*
 * Pin Change Interrupt Vector. This is for wakeup.
 */
ISR(PCINT3_vect) {
}

/*
 * ADC Interrupt Vector.  This is used by te modem. 
 */
ISR(ADC_vect) {
	modem.receiveADC();
}
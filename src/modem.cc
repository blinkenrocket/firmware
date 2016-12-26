/* Name: modem.c
 *
 * Audio modem for Attiny85 & other AVR chips with modifications
 *
 * Author: Jari Tulilahti
 * Copyright: 2014 Rakettitiede Oy and 2016 Daniel Friesel
 * License: LGPLv3, see COPYING, and COPYING.LESSER -files for more info
 */

#include <avr/io.h>
#include <stdlib.h>
#include "modem.h"
#include "fecmodem.h"

extern FECModem modem;

#define USE_ADC
// #define SPI_DBG


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
	
#ifdef USE_ADC
	DDRC |= _BV(PC2);  // use E3 and E2 to indicate bit detection 
	// PORTC |=  _BV(PC2);

	/* configure and enable ADC   */
	ADCSRA = _BV(ADEN) + _BV(ADIE) + _BV(ADPS2) +  _BV(ADPS0) +_BV(ADATE) ;      //  ADC prescaler 32 = 250KhZ - actually a bit overclocked ...
	ADMUX = _BV(REFS0) + 6;  // chn6 = PA0 / ADC6
	/*  start free running mode ** */
	ADCSRA |= _BV(ADSC);	
#endif
#ifdef SPI_DBG
	TIMSK0 &= ~_BV(TOIE0); // disable display! (use SPI for debugging output)
	PORTB=0; PORTD=255;
	PORTA &= ~_BV(PA1);   // slave select enable
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0)|(1<<SPR1);
#endif

	/* Timer: TCCR1: CS10 and CS11 bits: 8MHz clock with Prescaler 64 = 125kHz timer clock */
	TCCR1B = _BV(CS11) | _BV(CS10);
	/* Modem pin as input */
	MODEM_DDR &= ~_BV(MODEM_PIN);
	/* Enable Pin Change Interrupts and PCINT for MODEM_PIN */
	MODEM_PCMSK |= _BV(MODEM_PCINT);
	PCICR |= _BV(MODEM_PCIE);
}

void Modem::disable()
{
	PORTA &= ~_BV(PA3);
	DDRA  &= ~_BV(PA3);

#ifdef USE_ADC
	DDRC &= ~ _BV(PC2);
	ADCSRA &= ~ _BV(ADEN);
#endif
#ifdef SPI_DBG
	PORTA |= _BV(PA1);  // slave select disable
	SPCR=0;
	TIMSK0 |= _BV(TOIE0); // enable display !
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
#define ACTIVITY_LOW_HIGH_THRESHOLD 700
#define MIN_ACTIVITY 150
#define MIN_PULSELEN 1
#define MAX_PULSELEN 10
#define PULSELEN_THRESHOLD 6

void Modem::receiveADC() {

	static uint8_t modem_bit = 0;
	static uint8_t modem_byte = 0;
	static uint8_t sampleblocks=0;

	// some variables for sampling / frequency detection
	int16_t delta;
	static uint32_t activity=0;
	static uint16_t sampleValue=0, prevValue=0;
	static uint8_t cnt=0,actFrequency=FREQ_NONE,prevFrequency=FREQ_NONE, cancel=0;
	
	prevValue=sampleValue;
	sampleValue=ADC;		
	delta= (sampleValue>prevValue) ? sampleValue-prevValue : prevValue-sampleValue;
	if (delta > 200) delta = 200;   // optional: ignore too steep deltas 
	activity+=delta;
	
	if (++cnt==NUMBER_OF_SAMPLES) {
		cnt=0;
		sampleblocks++;

		if (activity < MIN_ACTIVITY) {        // no active sine wave detected
			activity=0;
			prevFrequency=FREQ_NONE;
			modem_bit = 0;
			modem_byte=0;
			modem.buffer_clear();
			PORTC &= ~ _BV(PC2);      // keep low during idle phase
			sampleblocks=0;
			return;
		} 

		if (activity > ACTIVITY_LOW_HIGH_THRESHOLD) 
			actFrequency=FREQ_HIGH; 
		else actFrequency=FREQ_LOW;
		activity=0;  // reset activity every NUMBER_OF_SAMPLES

		if (actFrequency != prevFrequency)
		{
			uint8_t modem_pulselen = sampleblocks;  // pluselen is expressed in sampleblocks
			sampleblocks=0;

			if (prevFrequency==FREQ_NONE) cancel=1; else cancel=0;
			prevFrequency=actFrequency;
			if (cancel) return;
			if ((modem_pulselen > MIN_PULSELEN) && (modem_pulselen < MAX_PULSELEN))  // sanity check for valid bit times
			{
				PORTC ^= _BV(PC2);   // show actual bit detection
				modem_byte = (modem_byte >> 1) | (modem_pulselen < PULSELEN_THRESHOLD ? 0x00 : 0x80);
				
				// Check if we received complete byte and store it in ring buffer
				if (!(++modem_bit % 0x08)) 
				{
					buffer_put(modem_byte);
					#ifdef SPI_DBG
						SPDR = modem_byte;
						// SPDR = modem_pulselen;
						// SPDR = sav_act;
					#endif
				}
			}
		}
	}
}


/*
 * Pin Change Interrupt Vector. This is The Modem.
 */
ISR(PCINT3_vect) {
#ifndef USE_ADC
	modem.receive();
#endif
}

/*
 * ADC Interrupt Vector. 
 */
ISR(ADC_vect) {
	modem.receiveADC();
}
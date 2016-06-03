# Audio transmission protocol definition


This document describes the communication between the blinkenrocket and the audio generating device attached to it. It relies on the implementation of the [tagsu-avr-modem](https://github.com/Jartza/tagsu-avr-modem) for low-level communication.

The communication relies on multiple components:

##### START 
A *`START`* signal which indicates the start of a transmission. It consists of two times a specific 8-bit binary pattern ( `10011001` respectively `0x99`).

##### PATTERN 
A *`PATTERN`* signal which indicates that either the start of an animation or text pattern. It consists of two times the 8-bit binary pattern ( `10101001` respectively `0xA9`)

##### END
The *`END`* signal indicates End Of Transmission. It consists of two times the 8-bit binary pattern `10000100` respectively `0x84`.

##### HEADER 
A generic *`HEADER`* which contains two byte of metadata to describe the data that follows. The two byte contain 12 bit of length information and 4 bit of data type information.

```
XXXXYYYY YYYYYYYY
<-->
    <----------->
TYPE    LENGTH
```

Thus the data length can be up to 4kByte of data (4096 byte). A type of `0001` denotes a `TEXT` type pattern, a type `0010` denotes an `ANIMATION` type pattern.
The modem only receives data for this pattern until length is exceeded. E.g. when a *`HEADER`* with the contents `00011111 11111111` is received by the modem it will read 4098 byte for the current pattern (2 byte header, 4096 byte of data).  The maximum length for texts is 4096 characters and 512 frames for animation.

##### TEXT METADATA 

A *`TEXTMETA`* is a two byte (16 bit) length metadata field for text type pattern. It encodes the speed (first nibble), the delay (second nibble) and the direction (third nibble). The fourth nibble is reserved for future use.

```
XXXX XXXX XXXX XXXX (MSB -> LSB)
---- (speed)
     ---- (delay)
          ---- (direction)
               ---- (reserved)
```

The speed and delay ranges from a numeric value from 0 (0000) to 15 (1111). The higher the number, the faster the speed and the longer the delay. A direction of 0 (0000) specifies a left direction, a direction of 1 (0001) specifies a right direction.

The scroll rate is about 1 / (0.5 - (0.032 * speed)) columns per second (or,
precisely, 1 / (0.002048 * (250 - (16 * speed))) columns per second). This
means that it can range from about 2 columns per second (speed=0) to almost 49
columns per second (speed=15). Note that it is not a linear function -- see the
following speed <-> columns per second translation list for details:

* speed =  0 :  1.95 cps
* speed =  1 :  2.09 cps
* speed =  2 :  2.24 cps
* speed =  3 :  2.42 cps
* speed =  4 :  2.63 cps
* speed =  5 :  2.87 cps
* speed =  6 :  3.17 cps
* speed =  7 :  3.54 cps
* speed =  8 :  4.00 cps
* speed =  9 :  4.61 cps
* speed = 10 :  5.43 cps
* speed = 11 :  6.60 cps
* speed = 12 :  8.42 cps
* speed = 13 : 11.6  cps
* speed = 14 : 18.8  cps
* speed = 15 : 48.8  cps

The delay takes 0.5 * delay seconds.

##### ANIMATION METADATA

A *`ANIMMETA`* is a two byte (16 bit) length metadata field for animation type pattern. It encodes the frame rate in the lower nibble of the first byte and the delay in the lower nibble of the second byte.

```
0000XXXX 0000XXXX
<------> <------>
 SPEED     DELAY
```

The speed and delay ranges from a numeric value from 0 (0000) to 15 (1111)
and are calculated as described in TEXT METADATA (except that the speed
now refers to frames per second).

## Message format

The message transmitted has to follow the following diagram:

```
       +--------------------------------------------------------+
       |                                                        |
       |                                         +--------+     |
       |                                         v        |     |
       |                         +--> TEXTMETA +--> DATA -+--+  |
       v                         |                           |  |
START +--> PATTERN +--> HEADER +-|                           +---> END
                                 |                           |
                                 +--> ANIMMETA +--> DATA -+--+
                                                 ^        |
                                                 +--------+
```

First there is a `START` signal to indicate that the transmission is about to start. This should initialize the internal state machine. After that a `PATTERN` indicator follows to indicate that a pattern is following. This is followed by the generic `HEADER` to set the size and type of the following data. Now depending on the type of the transmission there is either (XOR) the `TEXTMETA` or `ANIMMETA` fields followed by `DATA` fields that may repeat up to the length specified in the `HEADER`. After this the transmission either encounters an `END` or a repetition of the sequence starting with `PATTERN`.

Important note: the length of the data must be even (e.g. `length(data) % 2 == 0`) to make the Hamming(24,16) error correction work properly.

## Error detection and correction

The error correction is performed using the Hamming-Code. For this application the Hamming(24,16) code is used, which contains 2 bytes of data with a ECC of 1 byte. The error correction is capabable of correcting up to two bit flips, which should be sufficient for this application.

```
XXXXXXXX YYYYYYYY EEEEEEEE
-------- -------- --------
 First    Second   Hamming
```

The encoding of the Hamming code is performed using [this (click me)](https://github.com/RobotRoom/Hamming) avr-specific library and [this particular](https://github.com/RobotRoom/Hamming/blob/master/HammingCalculateParitySmallAndFast.c) implementation optimized for speed and code footprint.



# firmware
Firmware for blinkenrocket

Use `make && sudo make program` to flash a blinkenrocket and use the web
editor on <http://blinkenrocket.de/> to load patterns.

You can also use `cd utilities; ./modem_transmit string1 string2 string3 ...`,
though this only supports simple string patterns and a few fixed example
animations.

# Usage

## Sleep / Wakeup

* Press both buttons for at least 500ms to put the rocket into deep sleep
  (~1µW power consumption)
* Press any key (or send a modem transmission with 100ms of extra sync pulses)
  to turn it back on

## Normal operation

* Left button: Switch to previous pattern
* Right button: Switch to next pattern

The new pattern will not be loaded before the button has been released.

# Error messages / conditions

## "Transmission failure"

A modem transmission was started, but not properly terminated. Make sure that
your audio volume is set to 100%.

## "Storage is empty"

The storage does not contain any patterns yet. Use `modem_transmit` or
<http://blinkenrocket.de/> to fill it with patterns of your choice.

## Modem transmissions don't work at all

Make sure that your audio volume is set to 100%. If possible, try transmitting
from another device.

## Rocket does not turn on

This probably means that either your MCU (U1) is not properly powered,
or it is unable to communicate with the storage (U2).

Make sure that the right parts are soldered in the right position and check
your soldering. Double-check parts and soldering for U1, U2, C3, R4 and R5.

Make sure that the battery is correctly inserted. The plus pins must lay on TOP
of the battery where the minus pins are below on the BOTTOM of the battery. 
Insert the battery correctly from one side and then slide it in.

## Rocket hangs during patern display / when switching patterns

If the display still displays something, but does not scroll/advance the
animation anymore and the rocket does not respond to key presses, it means that
it is unable to communicate with the EEPROM storage.

Double-check parts and soldering (especially for U1, U2, C3, R4 and R5). If you
are really sure that everything is soldered correctly, your EEPROM might be
faulty (not as in data corruption, but as in "does not even acknowledge its
presence anymore"). This is quite improbable, though.

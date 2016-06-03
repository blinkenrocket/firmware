MCU ?= attiny88
AVRDUDE_PROGRAMMER ?= usbasp

AVRCC ?= avr-gcc
AVRCXX ?= avr-g++
AVRFLASH ?= avrdude
AVRNM ?= avr-nm
AVROBJCOPY ?= avr-objcopy
AVROBJDUMP ?= avr-objdump

MCU_FLAGS = -mmcu=attiny88 -DF_CPU=8000000UL

SHARED_FLAGS = ${MCU_FLAGS} -I. -Os -Wall -Wextra -pedantic
SHARED_FLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
SHARED_FLAGS += -flto -mstrict-X

ifeq (${LANG},DE)
	SHARED_FLAGS += -DLANG_DE
endif

CFLAGS += ${SHARED_FLAGS} -std=c11
CXXFLAGS += ${SHARED_FLAGS} -std=c++11 -fno-rtti -fno-exceptions

ASFLAGS += ${MCU_FLAGS} -wA,--warn
LDFLAGS += -Wl,--gc-sections

AVRFLAGS += -U lfuse:w:0xee:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
AVRFLAGS += -U flash:w:build/main.hex
#AVRFLAGS += -U eeprom:w:main.eep

HEADERS  = $(wildcard src/*.h)
ASFILES  = $(wildcard src/*.S)
CFILES   = $(wildcard src/*.c)
CXXFILES = $(wildcard src/*.cc)
OBJECTS  = ${CFILES:src/%.c=build/%.o} ${CXXFILES:src/%.cc=build/%.o} ${ASFILES:src/%.S=build/%.o}

all: build build/main.elf

build:
	mkdir -p build

build/%.hex: build/%.elf
	${AVROBJCOPY} -O ihex -R .eeprom $< $@

build/%.eep: build/%.elf
	${AVROBJCOPY} -j .eeprom --set-section-flags=.eeprom="alloc,load" \
	--change-section-lma .eeprom=0 -O ihex $< $@

build/%.o: src/%.cc ${HEADERS}
	${AVRCXX} ${CXXFLAGS} -o $@ $< -c -Wl,-Map=main.map,--cref

build/%.o: src/%.c ${HEADERS}
	${AVRCC} ${CFLAGS} -o $@ $< -c -Wl,-Map=main.map,--cref

build/main.elf: ${OBJECTS}
	${AVRCXX} ${CXXFLAGS} -o $@ $^ ${LDFLAGS}
	@echo
	@avr-size --format=avr --mcu=${MCU} $@

massprogram: all
	python utilities/flasher.py "${AVRFLASH} -p ${MCU} -c ${AVRDUDE_PROGRAMMER} ${AVRFLAGS}"

flash: program

program: build/main.hex #main.eep
	${AVRFLASH} -p ${MCU} -c ${AVRDUDE_PROGRAMMER} ${AVRFLAGS}

secsize: build/main.elf
	${AVROBJDUMP} -hw -j.text -j.bss -j.data $<

funsize: build/main.elf
	${AVRNM} --print-size --size-sort $<

.PHONY: all program secsize funsize

#!/usr/bin/env python

import sys, wave

class modem:

	# Modem specific constants
	bits = [[3 * chr(0), 5 * chr(0)], [3 * chr(255), 5 * chr(255)]]
	sync = [17 * chr(0), 17 * chr(255)]
	# Variable to alternate high and low
	hilo = 0
	supportedFrequencies = [16000,22050,24000,32000,44100,48000]
	cnt = 0
	# Data variables
	data = []
	parity = True
	frequency = 48000

	# Hamming code translation table
	_hammingCalculateParityLowNibble = [0,  3,  5,  6,  6,  5,  3,  0,  7,  4,  2,  1,  1,  2,  4,  7]
	_hammingCalculateParityHighNibble = [0,  9, 10,  3, 11,  2,  1,  8, 12,  5,  6, 15,  7, 14, 13,  4]

	# Almost nothing here
	def __init__(self, data=[], parity=True, frequency=48000):
		self.data = data
		self.parity = parity
		self.frequency = frequency if frequency in self.supportedFrequencies else 48000
		self.cnt = 0

	# Calculate Hamming parity for 12,8 code (12 bit of which 8bit data)
	def hammingCalculateParity128(self, byte):
		return self._hammingCalculateParityLowNibble[byte&0x0F] ^ self._hammingCalculateParityHighNibble[byte >> 4]

	# Calculate Hamming parity for 24,16 code (24 bit of which 16 bit are data)
	def hammingCalculateParity2416(self, first, second):
		return self.hammingCalculateParity128(second) << 4 | self.hammingCalculateParity128(first)

	# Generate one sync-pulse
	def syncsignal(self):
		self.hilo ^= 1
		return self.sync[self.hilo]

	# Generate a number of sync signals
	def generateSyncSignal(self, number):
		sound = ""
		for i in xrange(number):
			sound += self.syncsignal()
		return sound

	# Decode bits to modem signals
	def modemcode(self, byte):
		bleep = ""
		for x in xrange(8):
			self.hilo ^= 1
			bleep += self.bits[self.hilo][byte & 0x01]
			byte >>= 1
		return bleep

	# Return <length> samples of silence
	def silence(self, length):
		return chr(127) * length

	# Set data for modem code
	def setData(self, data):
		self.data = data

	# Set whether to use parity or not
	def setParity(self, parity):
		self.parity = parity

	# Set the frequency for the audio
	def setFrequency(self, frequency):
		self.frequency = frequency if frequency in self.supportedFrequencies else 48000

	# Generates the audio frames based on the data
	def generateAudioFrames(self):
		if self.parity:
			tmpdata = []
			# for uneven length data, we have to append a null byte
			if not len(self.data) % 2 == 0:
				self.data.append(chr(0))
			# insert the parity information every two bytes, sorry for the heavy casting
			for index in range(0, len(self.data), 2):
				tmpdata.extend(self.data[index:index+2])
				tmpdata.append(chr(self.hammingCalculateParity2416(ord(self.data[index]),ord(self.data[index+1]))))
			self.data = tmpdata
		# generate the audio itself
		# add 1000ms of sync signal before the data
		# (some sound cards take a while to produce a proper output signal)
		sound = self.generateSyncSignal(3000)
		# process the data and insert sync signal every 10 bytes
		for byte in self.data:
			sound += self.modemcode(ord(byte))
			self.cnt += 1
			if self.cnt == 9: # ! do not send sync inside (byte1 byte2 parity) triples
				sound += self.generateSyncSignal(4)
				self.cnt = 0
		# add some sync signals in the end
		sound += self.generateSyncSignal(4)
		return sound

	def saveAudio(self,filename):
		wav = wave.open(filename, 'wb')
		wav.setparams((1, 1, self.frequency, 0, "NONE", None))
		wav.writeframes(self.generateAudioFrames())
		wav.close()

class Frame( object ):
	""" Returns the frame information """
	def getFrameHeader(self):
		raise NotImplementedError("You should implement this!")

	""" Returns the representation """
	def getRepresentation(self):
		raise NotImplementedError("Should have implemented this")

class textFrame(Frame):
	text = ""
	speed = 0
	delay = 0
	direction = 0
	# identifier as of message specification: 0001
	identifier = 0x01

	def __init__(self,text,speed=13,delay=0,direction=0):
		self.text = text
		self.setSpeed(speed)
		self.setDelay(delay)
		self.setDirection(direction)

	def setSpeed(self,speed):
		self.speed = speed if speed < 16 else 1

	def setDelay(self,delay):
		self.delay = delay if delay < 16 else 0

	def setDirection(self,direction):
		self.direction = direction if direction in [0,1] else 0

	# Frame header: 4 bit type + 12 bit length
	def getFrameHeader(self):
		return [chr(self.identifier << 4 | len(self.text) >> 8), chr(len(self.text) & 0xFF) ]

	# Header -> 4bit speed, 4 bit delay, 4 bit direction, 4 bit zero
	def getHeader(self):
		return [chr(self.speed << 4 | self.delay), chr(self.direction << 4 | 0x00)]

	def getRepresentation(self):
		retval = []
		retval.extend(self.getFrameHeader())
		retval.extend(self.getHeader())
		retval.extend(list(self.text))
		return retval

class animationFrame(Frame):
	animation = []
	speed = 0
	delay = 0
	# identifier as per specification: 0010	
	identifier = 0x02

	def __init__(self,animation,speed=13,delay=0):
		self.setAnimation(animation)
		self.setSpeed(speed)
		self.setDelay(delay)

	def setAnimation(self,animation):
		if len(animation) % 8 is not 0:
			raise Exception
		else:
			self.animation = animation

	def setSpeed(self,speed):
		self.speed = speed if speed < 16 else 1

	def setDelay(self,delay):
		self.delay = delay if delay < 16 else 0

	# Frame header: 4 bit type + 12 bit length
	def getFrameHeader(self):
		return [chr(self.identifier << 4 | len(self.animation) >> 8), chr(len(self.animation) & 0xFF) ]

	# Header -> 4bit zero, 4bit speed, 4 bit zero, 4 bit direction
	def getHeader(self):
		return [chr(self.speed), chr(self.delay)]

	def getRepresentation(self):
		retval = []
		retval.extend(self.getFrameHeader())
		retval.extend(self.getHeader())
		retval.extend(self.animation)
		return retval


class blinkenrocket():

	eeprom_size = 65536
	startcode = chr(0x99)
	patterncode = chr(0xA9)
	endcode = chr(0x84)
	frames = []

	def __init__(self,eeprom_size=65536):
		self.eeprom_size = eeprom_size if eeprom_size < 256*1024*1024 else 65536
	
	def addFrame(self, frame):
		if not isinstance(frame, Frame):
			raise RuntimeError("Incorrect frame supplied")
		else:
			self.frames.append(frame)

	def getMessage(self):
		output = [self.startcode, self.startcode]
		for frame in self.frames:
			output.extend([self.patterncode,self.patterncode])
			output.extend(frame.getRepresentation())
		output.extend([self.endcode,self.endcode])
		return output




if __name__ == '__main__':
	m = modem(parity=True, frequency=48000)
	b = blinkenrocket()

	for message in sys.argv[2:]:
		b.addFrame(textFrame(message, speed=13))
	b.addFrame(textFrame(" \x04 "))
	b.addFrame(animationFrame(map(lambda x : chr(x), [0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255]), speed=0))
	b.addFrame(animationFrame(map(lambda x : chr(x), [
	  1,   0,   0,   0,   0,   0,   0,   0,
	  0,   1,   0,   0,   0,   0,   0,   0,
	  0,   0,   1,   0,   0,   0,   0,   0,
	  0,   0,   0,   1,   0,   0,   0,   0,
	  0,   0,   0,   0,   1,   0,   0,   0,
	  0,   0,   0,   0,   0,   1,   0,   0,
	  0,   0,   0,   0,   0,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,   0,   2,
	  0,   0,   0,   0,   0,   0,   0,   4,
	  0,   0,   0,   0,   0,   0,   0,   8,
	  0,   0,   0,   0,   0,   0,   0,   16,
	  0,   0,   0,   0,   0,   0,   0,   32,
	  0,   0,   0,   0,   0,   0,   0,   64,
	  0,   0,   0,   0,   0,   0,   0,   128,
	  0,   0,   0,   0,   0,   0, 128,   0,
	  0,   0,   0,   0,   0, 128,   0,   0,
	  0,   0,   0,   0, 128,   0,   0,   0,
	  0,   0,   0, 128,   0,   0,   0,   0,
	  0,   0, 128,   0,   0,   0,   0,   0,
	  0, 128,   0,   0,   0,   0,   0,   0,
	128,   0,   0,   0,   0,   0,   0,   0,
	 64,   0,   0,   0,   0,   0,   0,   0,
	 32,   0,   0,   0,   0,   0,   0,   0,
	 16,   0,   0,   0,   0,   0,   0,   0,
	  8,   0,   0,   0,   0,   0,   0,   0,
	  4,   0,   0,   0,   0,   0,   0,   0,
	  2,   0,   0,   0,   0,   0,   0,   0,
	  0,   2,   0,   0,   0,   0,   0,   0,
	  0,   0,   2,   0,   0,   0,   0,   0,
	  0,   0,   0,   2,   0,   0,   0,   0,
	  0,   0,   0,   0,   2,   0,   0,   0,
	  0,   0,   0,   0,   0,   2,   0,   0,
	  0,   0,   0,   0,   0,   0,   2,   0,
	  0,   0,   0,   0,   0,   0,   4,   0,
	  0,   0,   0,   0,   0,   0,   8,   0,
	  0,   0,   0,   0,   0,   0,  16,   0,
	  0,   0,   0,   0,   0,   0,  32,   0,
	  0,   0,   0,   0,   0,   0,  64,   0,
	  0,   0,   0,   0,   0,  64,   0,   0,
	  0,   0,   0,   0,  64,   0,   0,   0,
	  0,   0,   0,  64,   0,   0,   0,   0,
	  0,   0,  64,   0,   0,   0,   0,   0,
	  0,  64,   0,   0,   0,   0,   0,   0,
	  0,  32,   0,   0,   0,   0,   0,   0,
	  0,  16,   0,   0,   0,   0,   0,   0,
	  0,   8,   0,   0,   0,   0,   0,   0,
	  0,   4,   0,   0,   0,   0,   0,   0,
	  0,   0,   4,   0,   0,   0,   0,   0,
	  0,   0,   0,   4,   0,   0,   0,   0,
	  0,   0,   0,   0,   4,   0,   0,   0,
	  0,   0,   0,   0,   0,   4,   0,   0,
	  0,   0,   0,   0,   0,   8,   0,   0,
	  0,   0,   0,   0,   0,  16,   0,   0,
	  0,   0,   0,   0,   0,  32,   0,   0,
	  0,   0,   0,   0,  32,   0,   0,   0,
	  0,   0,   0,  32,   0,   0,   0,   0,
	  0,   0,  32,   0,   0,   0,   0,   0,
	  0,   0,  16,   0,   0,   0,   0,   0,
	  0,   0,   8,   0,   0,   0,   0,   0,
	  0,   0,   0,   8,   0,   0,   0,   0,
	  0,   0,   0,   0,   8,   0,   0,   0,
	  0,   0,   0,   0,  16,   0,   0,   0,
	  0,   0,   0,  16,   0,   0,   0,   0,
	  0,   0,   0,  24,  24,   0,   0,   0,
	  0,   0,  60,  36,  36,  60,   0,   0,
	  0, 126,  66,  66,  66,  66, 126,   0,
	255, 129, 129, 129, 129, 129, 129, 255,
	  0,   0,   0,   0,   0,   0,   0,   0,
	]), speed=15, delay=1))
	b.addFrame(animationFrame(map(lambda x : chr(x), [
	  0,   0,   0,  24,  24,   0,   0,   0,
	  0,   0,  60,  36,  36,  60,   0,   0,
	  0, 126,  66,  66,  66,  66, 126,   0,
	255, 129, 129, 129, 129, 129, 129, 255,
	  0,   0,   0,   0,   0,   0,   0,   0,
	]), speed=14, delay=1))

	m.setData(b.getMessage())
	m.saveAudio(sys.argv[1])

	#print b.getMessage()



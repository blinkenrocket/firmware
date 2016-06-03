#!/usr/bin/python

import sys
import os
import re
import json

result = {}

with open('font.h') as font:
	for line in font:
		if 'PROGMEM' in line:
			hexes = re.findall(r'(0x[0-9a-fA-F]+)',line)
			prefix = hexes[:1]
			contents = hexes[1:]
			length = len(contents)
			literal = int(re.findall(r'chr_([0-9]+)',line)[0])
			description = re.findall(r'\/\/(.*)$',line)[0].strip()
			newhexes = []
			newhexes.append(prefix[0])
			for row in contents:
				bitstring = '{0:08b}'.format(int(row,16))
				newbits = bitstring[::-1]
				newhexes.append( format(int(newbits,2), '#04x'))
			print "const unsigned char PROGMEM chr_%s[] = {%s}; // %s" % (literal,','.join(newhexes),description)

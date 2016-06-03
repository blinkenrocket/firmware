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
			contents = hexes[1:]
			length = len(contents)
			literal = int(re.findall(r'chr_([0-9]+)',line)[0])
			description = re.findall(r'\/\/(.*)$',line)[0].strip()
			#for row in contents:
			#	print '{0:08b}'.format(int(row,16)) #.replace("1",u"\u2588").replace("0",u"\u25A2")
			result[str(literal)] = {
				'literal' : literal,
				'description' : description,
				'hexcolumns' : contents,
				'length' : length
			}

print json.dumps(result)

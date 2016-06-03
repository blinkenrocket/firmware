#!/usr/bin/python
import unittest
from blinkenrocket import *

class TestFrame(unittest.TestCase):

  def test_notImplemented(self):
      frame = Frame()
      with self.assertRaises(NotImplementedError):
        frame.getRepresentation()

class TestAnimation(unittest.TestCase):

  def test_speedDefault(self):
    anim = animationFrame([])
    self.assertEquals(ord(anim.getHeader()[0]),1)

  def test_speedOkay(self):
    anim = animationFrame([],speed=7)
    self.assertEquals(ord(anim.getHeader()[0]),7)

  def test_speedNotOkay(self):
    anim = animationFrame([],speed=70)
    self.assertEquals(ord(anim.getHeader()[0]),1)

  def test_delayDefault(self):
    anim = animationFrame([])
    self.assertEquals(ord(anim.getHeader()[1]),0)

  def test_delayOkay(self):
    anim = animationFrame([],delay=7)
    self.assertEquals(ord(anim.getHeader()[1]),7)

  def test_delayNotOkay(self):
    anim = animationFrame([],delay=70)
    self.assertEquals(ord(anim.getHeader()[1]),0)

  def test_illegalLength(self):
    with self.assertRaises(Exception):
      anim = animationFrame([0x11,0x12,0x13,0x14])

  def test_allowedLength(self):
    anim = animationFrame([0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18])

  def test_defaultHeaderOK(self):
    anim = animationFrame([])
    self.assertEquals(anim.getHeader(),[chr(1),chr(0)])

  def test_differentHeaderOK(self):
    anim = animationFrame([],speed=7,delay=8)
    self.assertEquals(anim.getHeader(),[chr(7),chr(8)])

class TestText(unittest.TestCase):

  def test_speedDefault(self):
    text = textFrame([])
    self.assertEquals(ord(text.getHeader()[0]),(1 << 4 | 0))

  def test_speedOkay(self):
    text = textFrame([],speed=7)
    self.assertEquals(ord(text.getHeader()[0]),(7 << 4 | 0))

  def test_speedNotOkay(self):
    text = textFrame([],speed=70)
    self.assertEquals(ord(text.getHeader()[0]),(1 << 4 | 0))

  def test_delayDefault(self):
    text = textFrame([])
    self.assertEquals(ord(text.getHeader()[0]),(1 << 4 | 0))

  def test_delayOkay(self):
    text = textFrame([],delay=7)
    self.assertEquals(ord(text.getHeader()[0]),(1 << 4 | 7))

  def test_delayNotOkay(self):
    text = textFrame([],delay=70)
    self.assertEquals(ord(text.getHeader()[0]),(1 << 4 | 0))

  def test_directionDefault(self):
    text = textFrame([])
    self.assertEquals(ord(text.getHeader()[1]),0)

  def test_directionOkay(self):
    text = textFrame([],direction=1)
    self.assertEquals(ord(text.getHeader()[1]),(1 << 4 | 0))

  def test_directionNotOkay(self):
    text = textFrame([],direction=7)
    self.assertEquals(ord(text.getHeader()[1]),0)

  def test_defaultHeaderOK(self):
    text = textFrame([])
    self.assertEquals(text.getHeader(),[chr(1 << 4 | 0),chr(0)])

  def test_differentHeaderOK(self):
    text = textFrame([],speed=7,delay=8,direction=1)
    self.assertEquals(text.getHeader(),[chr(7 << 4 | 8),chr(1 << 4 | 0)])

class TestBlinkenrocket(unittest.TestCase):

  def test_addFrameFail(self):
    br = blinkenrocket()
    with self.assertRaises(Exception):
      br.addFrame([])

  def test_addFrameText(self):
    text = textFrame("MUZY",speed=7,delay=8,direction=1)
    self.assertEquals(text.getFrameHeader(),[chr(0x01 << 4), chr(4)])
    self.assertEquals(text.getHeader(),[chr(7 << 4 | 8),chr(1 << 4 | 0)])
    self.assertEquals(text.getRepresentation(),[chr(0x01 << 4), chr(4),chr(7 << 4 | 8),chr(1 << 4 | 0),'M','U','Z','Y'])
    br = blinkenrocket()
    br.addFrame(text)
    expect = [chr(0x99),chr(0x99),chr(0xA9),chr(0xA9),chr(0x01 << 4), chr(4),chr(7 << 4 | 8),chr(1 << 4 | 0),'M','U','Z','Y',chr(0x84),chr(0x84)]
    self.assertEquals(br.getMessage(),expect)

if __name__ == '__main__':
    unittest.main()

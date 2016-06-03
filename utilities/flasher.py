#!/usr/bin/python

import sys
import os
from termcolor import colored

try:
    import tty, termios
except ImportError:
    try:
        import msvcrt
    except ImportError:
        raise ImportError('getch not available')
    else:
        getch = msvcrt.getch
else:
    def getch():
        """getch() -> key character

        Read a single keypress from stdin and return the resulting character. 
        Nothing is echoed to the console. This call will block if a keypress 
        is not already available, but will not wait for Enter to be pressed. 

        If the pressed key was a modifier key, nothing will be detected; if
        it were a special function key, it may return the first character of
        of an escape sequence, leaving additional characters in the buffer.
        """
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

code_okay = """
 #######  ##    ##    ###    ##    ## 
##     ## ##   ##    ## ##    ##  ##  
##     ## ##  ##    ##   ##    ####   
##     ## #####    ##     ##    ##    
##     ## ##  ##   #########    ##    
##     ## ##   ##  ##     ##    ##    
 #######  ##    ## ##     ##    ##  
"""

code_error = """
######## ########  ########   #######  ########  
##       ##     ## ##     ## ##     ## ##     ## 
##       ##     ## ##     ## ##     ## ##     ## 
######   ########  ########  ##     ## ########  
##       ##   ##   ##   ##   ##     ## ##   ##   
##       ##    ##  ##    ##  ##     ## ##    ##  
######## ##     ## ##     ##  #######  ##     ## 
"""

def printOkay():
	print colored(code_okay, 'green')

def printError():
	print colored(code_error, 'red')

flash_command = sys.argv[1]

print colored("Using the following command to flash: %s" % flash_command, "green")

while True:
	print colored("Press any key to continue or 'q' to quit.","yellow")
	char = getch()
	print char
	if ord(char) is ord('q'):
		sys.exit(0)
	return_code = os.system(flash_command)
	if return_code > 0:
		printError()
	else:
		printOkay()


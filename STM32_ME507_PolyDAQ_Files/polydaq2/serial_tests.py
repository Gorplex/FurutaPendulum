# Experiment with serial ports in Python
import sys
import glob
import serial
import time
import tty
try:
	import termios
except ImportError:
	print 'Non-POSIX; what is going on?'
	# Non-POSIX. Return msvcrt's (Windows') getch.
	#import msvcrt


#--------------------------------------------------------------------------------------

def list_serial_ports ():
	"""Returns a list of serial communication ports which can be opened.
	"""
	if sys.platform.startswith ('win'):
		result = []
		for i in range (32):
			try:
				s = serial.Serial (i)
				s.close ()
				result.append ('COM' + str(i + 1))
			except serial.SerialException:
				pass
		return result

	elif sys.platform.startswith ('linux'):
		temp_list = glob.glob ('/dev/tty[A-Za-z]*')
		result = []
		for a_port in temp_list:
			try:
				s = serial.Serial (a_port)
				s.close ()
				result.append (a_port)
			except serial.SerialException:
				pass
		return result

	elif sys.platform.startswith ('darwin'):
		return glob.glob ('/dev/tty.*')


#--------------------------------------------------------------------------------------

#def scan_a_port (a_port):
	"""Watch a serial port. Report when it becomes usable; also report when it
	   becomes unusable.
	"""

	#ser_port = None
	#port_open = False

	#while True:
		## If the port isn't open, try to open it; on success, say we did
		#if port_open == False:
			#try:
				#ser_port = serial.Serial (a_port, timeout = 0)
				#port_open = True
				#print 'Port ', a_port, ' open'

			#except serial.SerialException:
				#pass

			#time.sleep (1.0)

		## If the port is open, attempt to read from the port with a timeout of 0 
		#else:
			## Attempt to read one character; respond if one is found
			#try:
				#ser_ch = ser_port.read (1)

			#except serial.SerialException:
				#port_open = False
				#ser_port.close ()
				#print 'Port ', a_port, ' closed'

			#time.sleep (0.2)

	#print ('OK, we are done.')


#--------------------------------------------------------------------------------------

def analog_read_once (ser_port, channel):
	""" Ask the microcontroller to read its analog input from the given channel by
	    communicating through the given serial port, and display the result.
	"""

	print 'Analog ', channel, '\r'
	ser_port.write (str (channel))

	ch_in = '\0'
	while ch_in != '\n':
		ch_in = ser_port.read ()
		print ch_in


#--------------------------------------------------------------------------------------

def main_loop (port_name):
	""" Program loop in which commands are sent to the PolyDAQ, replies are received,
	    and if the serial port isn't working, we scan to see if we can reconnect
	"""

	ser_port = None
	port_open = False

	# Set up the terminal (not the serial port, the user terminal) for raw I/O
	f_des = sys.stdin.fileno ()
	old_settings = termios.tcgetattr (f_des)
	tty.setraw (f_des)

	while True:

		# Check for user input
		user_ch = sys.stdin.read (1)

		if user_ch != None:
			ana_channel = None

			# Quit the program
			if user_ch == 'Q' or user_ch == 'q' or ord (user_ch) == 3:
				termios.tcsetattr (f_des, termios.TCSADRAIN, old_settings)
				sys.exit ()

			# Print a help screen
			if user_ch == 'H' or user_ch == 'h' or user_ch == '?':
				print '0-9, a-f: analog channel read\r'
				print 'H,h,?:    this help text\r'
				print 'Q,q,^C:   quit\r'

			# Ask for and display an analog reading from channel 0 - 9
			elif user_ch >= '0' and user_ch <= '9':
				ana_channel = ord (user_ch) - ord ('0')
				analog_read_once (ser_port, ana_channel)

			# Ask for and display an analog reading from channel 10 - 15
			elif user_ch >= 'a' and user_ch <= 'f':
				ana_channel = ord (user_ch) - ord ('a') + 10
				analog_read_once (ser_port, ana_channel)

			# This character makes no sense; ask WTF?  (What's That Function?)
			else:
				print 'Whaddya mean, ', user_ch, '?\r'

		#else:
			#print ord (user_ch), '\r'

		#------------------------------------------------------------------------------
		# If the port isn't open, try to open it; on success, say we did
		if port_open == False:
			time.sleep (1.0)                # Wait a second before retrying

			try:
				ser_port = serial.Serial (port_name, timeout = 0)
				port_open = True
				print 'Port', port_name, 'open\r'

			except serial.SerialException:
				pass

		# If the port is open, attempt to read from the port with a timeout of 0 
		else:
			time.sleep (0.1)

			# Attempt to read a character from the port
			try:
				ser_ch = ser_port.read (1)

			except serial.SerialException:
				port_open = False
				ser_port.close ()
				print 'Port', port_name, 'closed\r'

			time.sleep (0.1)



#--------------------------------------------------------------------------------------

if __name__ == '__main__':

	avail_ports = list_serial_ports ()
	print (list_serial_ports ())

	print ('Entering main loop...')
	main_loop ('/dev/ttyACM0')

	print ('...done.')

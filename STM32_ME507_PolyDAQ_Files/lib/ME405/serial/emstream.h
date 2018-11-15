//*************************************************************************************
/** @file    emstream.h
 *  @brief   Headers for a base class that imitates C++ I/O streams on serial devices. 
 *  @details This file contains a base class for devices which send information in 
 *           text form over serial devices. Example devices are serial ports (both 
 *           traditional RS-232 ports and USB-serial adapters) and wireless modems. 
 *
 *  Revised:
 *    \li 04-03-2006 JRR For updated version of compiler
 *    \li 06-10-2006 JRR Ported from C++ to C for use with some C-only projects
 *    \li 08-11-2006 JRR Some bug fixes
 *    \li 03-02-2007 JRR Ported back to C++. I've had it with the limitations of C.
 *    \li 04-16-2007 JO  Added write (unsigned long)
 *    \li 07-19-2007 JRR Changed some character return values to bool, added m324p
 *    \li 01-12-2008 JRR Added code for the ATmega128 using USART number 1 only
 *    \li 02-13-2008 JRR Split into base class and device specific classes; changed
 *                       from write() to overloaded << operator in the "cout" style
 *    \li 05-12-2008 ALS Fixed bug in which signed numbers came out unsigned
 *    \li 07-05-2008 JRR Added configuration macro by which to change what "endl" is
 *    \li 07-05-2008 JRR Added 'ascii' and 'numeric' format codes
 *    \li 11-24-2009 JRR Changed operation of 'clrscr' to a function to work with LCD
 *    \li 11-26-2009 JRR Integrated floating point support into this file
 *    \li 12-16-2009 JRR Improved support for constant strings in program memory
 *    \li 10-22-2012 JRR Fixed (OK, hacked around) bug which caused spurious warning 
 *                       for all Program Memory Strings
 *    \li 11-12-2012 JRR Made puts() non-virtual; made ENDL_STYLE() a function macro
 *    \li 12-21-2013 JRR Ported to ChibiOS
 *    \li 10-17-2014 JRR Made compatible with FreeRTOS for Cal Poly class use
 *
 *  License:
 *    This file released under the Lesser GNU Public License, version 2. This program
 *    is intended for educational use only, but it is not limited thereto. This code
 *    incorporates elements from Xmelkov's ftoa_engine.h, part of the avr-libc source,
 *    and users must accept and comply with the license of ftoa_engine.h as well. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//-------------------------------------------------------------------------------------
/* The following is taken from ftoa_engine.h, part of the avr-libc source. 
   It is subject to the following copyright notice: 
    Copyright (c) 2005, Dmitry Xmelkov
    All rights reserved.
  
    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:
 
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    * Neither the name of the copyright holders nor the names of
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
 
    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
    IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
    INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
    NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
    POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _EMSTREAM_H_
#define _EMSTREAM_H_

#include <stdint.h>                         // Needed for standard integer types(?!)


/** \brief This define selects what will be sent when a program sends out "endl".
 *  \details Different recieving programs want different end-of-line markers. 
 *  Traditionally, UNIX uses "\r" while PC's use "\r\n" and Macs use "\n" (I think). 
 */
#define ENDL_STYLE()        putchar ('\r'); putchar ('\n')


/** \brief This define selects the character which asks a terminal to clear its screen.
 *  \details The clear-screen character for an ANSII standard terminal is a control-L, 
 *  which is ASCII character number 12.
 */
#define CLRSCR_STYLE        "\033[2J\033[H"       // ((unsigned char)12)


/** \brief This define allows strings to be kept in AVR program (flash) memory only.
 *  \details It tells the puts() method to find them in program memory. This saves 
 *  SRAM memory compared to copying all the strings to SRAM, which is the rather 
 *  wasteful default method used by the compiler.  See the following link for more
 *  information about strings in program memory:
 *  \li http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=57011
 */
#ifdef __AVR
	#define PMS(s)  _p_str << \
		__extension__({static const char __attribute((section(".progmem.something"))) \
		__c[] = s; &__c[0]; })
#else
	#define PMS(s)  s
#endif


/** \brief This macro simplifies the writing of code to conditionally print debugging 
 *  output. 
 *  \details
 *  If SERIAL_DEBUG hasn't been defined, it replaces a debugging command with nothing.
 *  
 *  If SERIAL_DEBUG has been defined, it drops in a command that checks the given
 *  serial device pointer to make sure the device isn't NULL, and if the device isn't
 *  NULL then debugging output is printed.
 */
#ifdef SERIAL_DEBUG
	#define DBG(ptr,stuff) if (ptr) { *ptr << stuff; }
#else
	#define DBG(ptr,stuff) 
#endif


/** This definition is used to allow variables of type @c size_t to be somewhat
 *  conveniently transmitted by descendents of class @c emstream. One needs to 
 *  put a typecast before each such variable, because the normal automatic type
 *  detection doesn't always work for variables of type @c size_t. The usage is
 *  @c a_serial_device @c << @c (ems_size_t) @c thing;
 */
#if __SIZEOF_SIZE_T__ == 2
	#define ems_size_t uint16_t
#elif __SIZEOF_SIZE_T__ == 4
	#define ems_size_t uint32_t
#elif __SIZEOF_SIZE_T__ == 8
	#define ems_size_t uint64_t
#endif


/** @brief   String of characters used to convert numbers into printable characters.
 *  @details This string is used by functions in class @c emstream which convert
 *           numbers into sets of ASCII characters to be sent to serial devices. It
 *           allows the use of fairly efficient versions of @c itoa() and related
 *           functions which perform the translation through a simple table lookup.
 */
const char EMSTR_ASCII_CHARS[] = "FEDCBA9876543210123456789ABCDEF";


//-------------------------------------------------------------------------------------
/** @brief   Modifiers used to adjust how things are printed with the @c << operator.
 *  @details This enumeration is used to modify the way in which various things are 
 *           printed by descendent classes of \c emstream and to insert special 
 *           characters into what is being printed out. Values can change the display
 *           base for the output stream from the default of base 10 (decimal) to and 
 *           from any base between 2 (binary), and 16 (hexadecimal), inclusive. The 
 *           precision for printing floating point numbers can be adjusted with
 *           @b setprecision(), which emits the @c manip_set_precision modifier.
 *           Also defined are @c endl to send an end-of-line character, @c clrscr to
 *           send a clear-screen character which works with some terminal emulators 
 *           and not others, and @c send_now to tell some devices to transmit data as
 *           soon as possible. 
 */

typedef enum
{
	/// Print following numbers in base 2 (binary) until another base is specified
	bin,
	/// Print following numbers in base 8 (octal) until another base is specified
	oct,
	/// Print following numbers in base 10 (decimal) until another base is specified
	dec,
	/// Print following numbers in base 16 (hexadecimal) until another base is specified
	hex,
	/// Print following integers in Roman numerals rather than Arabic
	roman,
	fortran,
	/// Print a carriage return and/or linefeed as specified in \c ENDL_STYLE
	endl,
	/// Send a character specified in \c CLRSCR_STYLE to clear some terminal screens
	clrscr,
	/// If relevant to a device, tell it to send or save data immediately
	send_now,
	/// Set the precision (numbers after decimal) for printing floating point numbers
	manip_set_precision,
	/// Set the numeric base in which to print numbers
	manip_set_base,
	#ifdef __AVR
		/** \cond NO_DOXY Specifies that the following string is in program (flash)
		*  memory.  This modifier is not used directly by user-written programs
		*/
		_p_str,
		/// \endcond
	#endif
	/// Placeholder manipulator that causes nothing to happen
	manip_do_nothing
}
ser_manipulator;


//-------------------------------------------------------------------------------------
// This function sets the number of digits to be printed after the decimal point. 

ser_manipulator setprecision (uint8_t);


//-------------------------------------------------------------------------------------
// This function sets the numeric base in which to print numbers. 

ser_manipulator setbase (uint8_t new_base);


//-------------------------------------------------------------------------------------
/** @brief   Base class for serial devices which print using a @c << operator.
 *  @details This is a base class for serial devices which use an overloaded left 
 *           shift operator @c << to convert data into a character stream and send the
 *           characters to some communication or data storage channel. Descendents of
 *           this class are able to send text over serial lines, radio modems or 
 *           modules, and whatever other communication devices might be appropriate. 
 *           There is also a descendent which saves data to an SD card file as a 
 *           stream of characters. 
 *
 *           The most important methods of this class are the overloaded @c << 
 *           operators which convert different types of numbers into printable strings
 *           and then print them using methods which are overloaded by descendent 
 *           classes. Methods are provided which convert most types of integers and 
 *           floating point numbers into streams of bytes; floating point numbers are
 *           only handled if @c <math.h> is included before or near the beginning of 
 *           @c emstream.h because one often wants to turn off floating point support
 *           in order to save memory. Other classes may overload the @c << operator to
 *           print @e themselves to any descendent of this class; see, for example, 
 *           @c adc_driver.h/cpp for examples in which drivers "print themselves" 
 *           conveniently to any serial device.
 *
 *           The ability to transmit characters through some physical device is 
 *           provided by descendent classes. Methods which should be overridden by 
 *           descendents include the following: 
 *      \li @c ready_to_send() - Checks if the port is ready to transmit a character
 *      \li @c putchar() - Sends a single character over the communications line
 *      \li @c check_for_char() - Checks if a character is ready to be read
 *      \li @c getchar() - Reads a character from the device when one becomes available
 * 
 *           Other methods may optionally be overridden; for example @c clear_screen()
 *           needs only be overridden by devices which have a screen physical screen
 *           that requires a set of actions to be cleared, rather than just sending an
 *           ASCII standard clear-screen character. 
 */

class emstream
{
	// Private data and methods are accessible only from within this class and 
	// cannot be accessed from outside -- even from descendents of this class
	private:
		/** This constructor is "poisoned", meaning it can't be used. This is done to
		 *  prevent copies of a serial device object from being made, as copying a
		 *  device driver doesn't make any sense and wastes lots of memory
		 *  @param other_one A reference to a serial device which ought not be copied
		 */
		emstream (const emstream& other_one)
		{
			(void)other_one;
		}

		/** This assignment operator is "poisoned" by being declared private so that
		 *  it can't be used to copy the contents of one serial device into another. 
		 *  There is no legitimate, morally acceptable reason to copy data that way. 
		 *  @param other A reference to a serial device which ought not be copied
		 */
		emstream& operator= (const emstream& other)
		{
			(void)other;         // The entire body of this function exists only to
			return *this;        // shut up compiler warnings
		}

	// Protected data and methods are accessible from this class and its descendents
	// only
	protected:
		/** @brief   The base for displaying numbers.
		 *  @details This is the currently used base for converting numbers to text.
		 *           It must be between 2 (binary) and 16 (hexadecimal) inclusive.
		 */
		uint8_t base;

		/** @brief   Causes integers to be displayed as Roman rather than Arabic.
		 *  @details If this variable is true, the serial device will display 8 or 16
		 *           bit integers as Roman numerals, @e e.g. MCMLXXXIV. */
		bool roman_numerals;

		#ifdef __AVR
			/** If this variable is true, the next string to be printed will be found
			 *  in program (flash) memory, not data memory.
			 */
			bool pgm_string;
		#endif

		/** @brief   The number of digits after a decimal point to print.
		 *  @details This is the number of digits after a decimal point to be printed
		 *           when a floating point number is being converted to text. */
		uint8_t precision;

		// Method to print an integer as Roman numerals
		void print_roman (uint16_t num);

		// Method used to print each power of 10 in a Roman numeral
		void print_roman_digits (uint8_t digitus, uint8_t order);

		// Method to convert an unsigned integer from text to a numeric value
		uint32_t cin_uint_convert (void);

		// Method to convert a signed integer from text to a numeric value
		int32_t cin_int_convert (void);

		// Method to finish converting integers from text to a numeric value
		uint32_t cin_finish_conversion (char in_ch);

	// Public methods can be called from anywhere in the program where there is a 
	// valid pointer or reference to an instantiated object of this class
	public:
		emstream (void);                    // Simple constructor doesn't do much
		virtual bool ready_to_send (void);  // Virtual and not defined in base class

		/** @brief   Base method for sending one character.
		 *  @details This is a pure virtual base method for the \c putchar() method 
		 *           which must be overridden in every descendent of this class. 
		 *           Since \c putchar() has no implementation here, any attempt to 
		 *           instantiate an object of type \c emstream will create a compiler
		 *           error. This is appropriate because an object of this class 
		 *           wouldn't be able to do anything; it would be as useless as a 
		 *           prom dress for a pelican.
		 *  @param   a_char A character to be sent to the serial device
		 */
		virtual void putchar (char a_char) = 0;

		void puts (const char*);            // Write a string to the serial device

		virtual bool check_for_char (void); // Check if a character is in the buffer
		virtual char peek (void) = 0;       // Look at next character to be read
		virtual char getchar (void);        // Get a character; wait if none is ready
		virtual void transmit_now (void);   // Immediately transmit any buffered data
		virtual void clear_screen (void);   // Clear a display screen if there is one

		// This overloaded left-shift operator writes a boolean to the serial device
		emstream& operator << (bool);

		/** This operator writes the string whose first character is pointed to by the
		 *  given character pointer to the serial device. It acts in about the same 
		 *  way as \c puts(); in fact, all it really does it call \c puts(), but this
		 *  operator is more convenient to use because it allows a bunch of stuff to 
		 *  be printed in one line with \c << operators between the items.  As with 
		 *  \c puts(), the string to be printed must have a null character (ASCII zero)
		 *  at the end, per the standard C/C++ method for using strings. 
		 *  @param p_string Pointer to the string to be written
		 *  @return A reference to the serial device to which the data was printed. This
		 *          reference is used to string (bad pun) printable items together with 
		 *          many "<<" operators
		 */
		emstream& operator << (const char* p_string)
		{
			puts (p_string);
			return (*this);
		}

		/** This operator writes a \c char variable to a serial device. It's distinct
		 *  from the operators that write \c int8_t and \c uint8_t. Because the 
		 *  \c char data type is supposed to be used for printable characters, this
		 *  method calls \c putchar() to print the character out, regardless of the
		 *  value of the \c print_ascii variable.
		 *  @param ch The character to be printed
		 *  @return A reference to the serial device on which the printing is done
		 */
		emstream& operator << (char ch)
		{
			putchar (ch); 
			return (*this);
		}

		// This operator writes an 8-bit unsigned number to a serial device
		emstream& operator << (uint8_t);

		// This operator writes an 8-bit signed number to a serial device
		emstream& operator << (int8_t);

		// This operator writes a 16-bit unsigned number to a serial device
		emstream& operator << (uint16_t);

		// This operator writes a 16-bit number to a serial device
		emstream& operator << (int16_t);

		// This operator writes a 32-bit number to a serial device
		emstream& operator << (uint32_t);

		// This operator writes a 32-bit number to a serial device
		emstream& operator << (int32_t);

		// This operator writes a 64-bit number to a serial device
		emstream& operator << (uint64_t);

		// This operator writes a 64-bit number to a serial device
		emstream& operator << (int64_t);

		// This operator writes a pointer to a serial device
		emstream& operator << (void*);

		// These operators write floating point numbers to a serial device
		emstream& operator << (float);
		emstream& operator << (double);

		// This operator applies manipulators to the serial device to control how it
		// converts things to a stream of characters or to insert special characters
		emstream& operator << (ser_manipulator);

		/** @brief   Stream input operator which reads a single character.
		*  @details This operator reads one character from a serial device. It calls
		*           @c getchar(), so it's just syntactic sugar that makes it easy to 
		*           insert a single character read into a line of reads strung 
		*           together with @c >> operators. 
		*  @return  A reference to the serial device from which the data was read. This
		*           reference is used to string many read commands together if needed
		*  @param   ch The character to be filled with the data that has been read
		*/
		emstream& operator >> (char& ch)
		{
			ch = getchar ();
			return (*this);
		}

		// This operator reads an unsigned 8-bit number from a serial device
		emstream& operator >> (uint8_t& number);

		// This operator reads a signed 8-bit number from a serial device
		emstream& operator >> (int8_t& number);

		// This operator reads an unsigned 16-bit number from a serial device
		emstream& operator >> (uint16_t& number);

		// This operator reads a signed 16-bit number from a serial device
		emstream& operator >> (int16_t& number);

		// This operator reads an unsigned 32-bit number from a serial device
		emstream& operator >> (uint32_t& number);

		// This operator reads a signed 32-bit number from a serial device
		emstream& operator >> (int32_t& number);

		// This operator reads a floating point number from a serial device
		emstream& operator >> (float& number);
};


// This function displays memory contents in hexadecimal and text format
void hex_dump_memory (uint8_t*, uint8_t*, emstream*);


#endif  // _EMSTREAM_H_

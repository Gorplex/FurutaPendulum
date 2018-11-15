//*************************************************************************************
/** \file emstream_int16_t.cpp
 *    This file contains an operator which prints a 16-bit signed integer using the
 *    \c emstream class. 
 *
 *  Revised:
 *    \li 12-02-2012 JRR Split this file off from the main \c emstream.cpp to
 *                       allow smaller machine code if stuff in this file isn't used
 *
 *  License:
 *    This file released under the Lesser GNU Public License, version 2. This program
 *    is intended for educational use only, but it is not limited thereto. This code
 *    incorporates elements from Xmelkov's ftoa_engine.h, part of the avr-libc source,
 *    and users must accept and comply with the license of ftoa_engine.h as well. See
 *    emstream.h for a copy of the relevant license terms. */
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
//*************************************************************************************

#include <stdlib.h>
#include "emstream.h"


//-------------------------------------------------------------------------------------
/** @brief   Write a signed 16-bit number to a serial device. 
 *  @details This operator writes a signed short integer (@c int16_t) to the serial 
 *           device as a stream of characters. Descendent classes of @c emstream will
 *           provide methods @c putchar() and @c puts() to allow the characters to be
 *           physically printed or sent. The code for this function was written by 
 *           Lukás Chmela and Released under GPLv3; it was found at 
 *           http://www.jb.man.ac.uk/~slowe/cpp/itoa.html and modified for use with 
 *           class \c emstream. 
 *  @return  A reference to the serial device to which the data was printed. This
 *           reference is used to string printable items together with "<<" operators
 *  @param   num The 16-bit number to be sent out
 */

emstream& emstream::operator<< (int16_t num)
{
	char buffer[17];                        // Buffer to hold the printed string
	char* ptr = buffer;                     // Extra pointer to the buffer
	int16_t temp_num;                       // Saves an extra copy of the number

	// If the number is negative, make a negative sign
	if (num < 0)
	{
		putchar ('-');
		num = -num;
	}

	// Check if we're to print Roman rather than Arabic numerals
	if (roman_numerals)
	{
		print_roman (num);
	}
	// Regular formatted printing
	else
	{
		do
		{
			temp_num = num;
			num /= base;
			*ptr++ = EMSTR_ASCII_CHARS[15 + (temp_num - num * base)];
		} while (num);

		while (ptr > buffer)                    // Send the characters to putchar()
		{                                       // in reverse order
			putchar (*--ptr);
		}
	}

	return (*this);
}


//-------------------------------------------------------------------------------------
/** @brief   Read a 16-bit signed decimal integer from a serial device.
 *  @details This low-budget imitation of @c istream reads a signed integer. The
 *           integer must be in decimal form. If the integer is too large to fit in an
 *           16-bit number, there will be silent overflow and problems; this operator
 *           is designed to work in a small microcontroller which doesn't have room for
 *           fancy stuff such as exception handling. 
 *           @li @b FEATURE: If a negative sign is found @e anywhere before the 
 *               beginning of the number (not just immediately to the left of the first
 *               numeric digit), the number is considered negative. 
 *  @return  A reference to the serial device from which the data was read. This
 *           reference is used to string many read commands together if needed
 *  @param   number The 16-bit number to be filled with the data that has been read
 */

emstream& emstream::operator>> (int16_t& number)
{

	number = (int16_t)(cin_int_convert ());

	return (*this);
}

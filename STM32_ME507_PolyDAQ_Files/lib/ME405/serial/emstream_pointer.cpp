//*************************************************************************************
/** \file emstream_pointer.cpp
 *    This file contains an operator which prints pointers using the \c emstream class. 
 *
 *  Revised:
 *    \li 12-02-2012 JRR Split this file off from the main \c emstream.cpp to
 *                       allow smaller machine code if stuff in this file isn't used
 *    \li 06-07-2014 JRR Changed numeric type to work with STM32's as well as AVR's
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
/** @brief   Write a pointer to a serial device.
 *  @details This operator writes a pointer to the serial device. The pointer is shown 
 *           between square brackets to indicate that it is a pointer. Pointers are
 *           always printed in hexadecmial, regardless of the value in the numeric 
 *           base variable \c base. Pointers are also shown as a full set of digits 
 *           for the size of pointers on a given computer; leading zeros are printed
 *           if a pointer contains a number which wouldn't fill up all the digits. 
 *           If one needs to print a pointer using a different format, the pointer
 *           can be typecast to an appropriate integer type and printed as such. 
 *  @return  A reference to the serial device to which the data was printed. This
 *           reference is used to string printable items together with "<<" operators.
 *  @param   a_pointer The pointer to be sent to the serial device
 */

emstream& emstream::operator<< (void* a_pointer)
{
	char buffer[9];                         // Buffer to hold the printed string
	char* ptr;                              // Extra pointer to the buffer
	size_t temp_num;                        // Saves an extra copy of the number
	size_t pnum = (size_t)a_pointer;        // Copy pointer into an integer

	putchar ('[');

	// Print out all the digits in the pointer, even leading zeros
	ptr = buffer;
	for (uint8_t count = (sizeof (void*) * 2); count; count--)
	{
		temp_num = pnum;
		pnum >>= 4;
		*ptr++ = EMSTR_ASCII_CHARS[15 + (temp_num - (pnum << 4))];
	} 

	while (ptr > buffer)                    // Send the characters to putchar()
	{                                       // in reverse order
		putchar (*--ptr);
	}

	putchar (']');

	return (*this);
}

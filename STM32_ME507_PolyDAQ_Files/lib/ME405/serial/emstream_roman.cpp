//*************************************************************************************
/** \file emstream_roman.cpp
 *    This file contains an operator which prints an unsigned integer as Roman numerals
      using the \c emstream class. It is provided for complete Fortran compatibility.
 *
 *  Revised:
 *    \li I-JAN-MMXV JRR Original file
 *
 *  License:
 *    This file released under the Lesser GNU Public License, version 2. This program
 *    is intended for educational use only, but it is not limited thereto.  */
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
/** @brief   Write an unsigned integer to a serial device in Roman numerals.
 *  @details This operator writes an unsigned 8-bit or 16-bit integer to a serial 
 *           device as a stream of Roman numeral characters. Fortran programmers should
 *           find this capability important for (way) backwards compatibility. 
 *           Descendent classes of @c emstream will provide methods @c putchar() and 
 *           @c puts() to allow the characters to be physically printed or sent. 
 *  @param   num The number to be sent to the serial device
 */

void emstream::print_roman (uint16_t num)
{
	// Although there's officially no zero in Roman, we have to print something, so
	// write "Nil" in case of zero
	if (num == 0)
	{
		puts (PMS ("Nil"));
	}

	// Print all the thousands (there might be a lot of them; we can't use overbars
	// to multiply things by thousands in ASCII, so just print lots of M's)
	while (num >= 1000)
	{
		putchar ('M');
		num -= 1000;
	}

	// For each digit in the hundreds, tens, and ones use Latin style notation
	print_roman_digits (num / 100, 0);
	num %= 100;
	print_roman_digits (num / 10, 1);
	num %= 10;
	print_roman_digits (num, 2);
}


//-------------------------------------------------------------------------------------
/** @brief   Print the digit(s) associated with one power of ten in a Roman numeral.
 */

void emstream::print_roman_digits (uint8_t digitus, uint8_t order)
{
	// Array of characters to print; each 3-character substring is a power of 10
	const char characters[] = "MDC""CLX""XVI";

	if (digitus == 9)
	{
		putchar (characters[3 * order + 2]);               // The (-1) symbol
		putchar (characters[3 * order + 0]);               // The (10) symbol
	}
	else if (digitus == 4)
	{
		putchar (characters[3 * order + 2]);               // The (-1) symbol
		putchar (characters[3 * order + 1]);               // The (5) symbol
	}
	else if (digitus > 0)
	{
		// If we have 5 through 8, print the 5 symbol 
		if (digitus > 4)
		{
			putchar (characters[3 * order + 1]);           // The (5) symbol
			digitus -= 5;
		}
		// Now we just have some ones to fill in as necessary
		while (digitus--)
		{
			putchar (characters[3 * order + 2]);           // The (1) symbol
		}
	}
}

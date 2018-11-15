//*************************************************************************************
/** \file emstream_float.cpp
 *    This file contains the operators which print floating-point numbers using the
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

#include <stdint.h>
#include <stdlib.h>
#include "emstream.h"


//-------------------------------------------------------------------------------------
/** @brief   Single precision float accessed as a float or a set of 32 bits. 
 *  @details This union is used to hold a single precision floating point number in 
 *           its normal 32 bit format, but also allow the same 32 bits to be used in
 *           the format of an integer so that the sign bit, exponent, and mantissa can
 *           be accessed separately.
 */

typedef union
{
	float   number;     ///< The floating-point number seen as a floating-point number
	int32_t bits;       ///< The floating-point number seen as a 32-bit integer
} float_with_bits_t;


//-------------------------------------------------------------------------------------
/** @brief   Write a single precision floating point number to a serial device. 
 *  @details This operator writes a single precision floating point number to a serial 
 *           device as a stream of characters. Descendent classes of @c emstream 
 *           provide methods @c putchar() and @c puts() to allow the characters to be
 *           physically printed or sent. 
 *  @return  A reference to the serial device to which the data was printed. This
 *           reference is used to string printable items together with "<<" operators
 *  @param   num The floating point number to be sent to the serial device
 */

emstream& emstream::operator << (float num)
{
#ifdef __AVR
	uint8_t digit = precision;
	char buf[20];
	char* p_buf = buf;

	int exponent = __ftoa_engine ((double)num, buf, digit, 16);
	uint8_t vtype = *p_buf++;
	if (vtype & FTOA_NAN)
	{
		*this << "NaN";
		return (*this);
	}

	// Display the sign if it's negative
	if (vtype & FTOA_MINUS)
	{
		putchar ('-');
	}

	// Show the mantissa
	putchar (*p_buf++);
	if (digit)
	{
		putchar ('.');
	}
	while ((digit-- > 0) && *p_buf)
	{
		putchar (*p_buf++);
	}

	// Now display the exponent
	putchar ('E');
	if (exponent > 0)
	{
		putchar ('+');
	}
	*this << exponent;
#else  // It's not __AVR

	/** This code posted to @c http://www.edaboard.com/thread5585.html by @c cb30
	 *  and subsequently modified a bunch by JR */

	float_with_bits_t the_number;           // Number as float and 32 bit integer
	int32_t mantissa, frac_part;

	the_number.number = num;                // Populate the union with floaty bits

	// The following fix was on the thread by jim6669jim with the comment,
	// "JEB fixed for 16-bit char F2xxx"
	// Old (unfixed) code:  exp2 = (unsigned char)(x.L >> 23) - 127;
	int16_t exp2 = (the_number.bits >> 23) & 0x00FF;

	// Check for exceptions such as not-a-number (NaN)
	if (exp2 == 0xFF)
	{
		if (the_number.bits & 0x007FFFFF)
		{
			puts ("NaN");
		}
		else
		{
			puts ("inf");
		}
		return (*this);
	}
	if (exp2 == 0)
	{
		if (the_number.bits & 0x007FFFFF)
		{
			puts ("0.00");
		}
		else
		{
			puts ("0.0");
		}
		return (*this);
	}

	// OK, not an exception; find the actual exponent and mantissa
	exp2 -= 127;

	mantissa = (the_number.bits & 0xFFFFFF) | 0x800000;
	frac_part = 0;
	int64_t int_part = 0;

	if (exp2 < -23)
	{
		puts ("tiny");
		return (*this);
	}
	else if (exp2 >= 23)
	{
		int_part = (int64_t)mantissa << (exp2 - 23);
	}
	else if (exp2 >= 0)
	{
		int_part = (int64_t)mantissa >> (23 - exp2);
		frac_part = (mantissa << (exp2 + 1)) & 0xFFFFFF;
	}
	else          /* if (exp2 < 0) */
		frac_part = (mantissa & 0xFFFFFF) >> -(exp2 + 1);

	if (the_number.bits < 0)
	{
		putchar ('-');
	}

	if (int_part == 0)
	{
		putchar ('0');
	}
	else
	{
		*this << int_part;
	}
	putchar ('.');

	if (frac_part == 0)
	{
		putchar ('0');
	}
	else
	{
		for (uint8_t after_dp = 0; after_dp < precision; after_dp++)
		{
			frac_part = (frac_part << 3) + (frac_part << 1);    // frac_part *= 10
			putchar ((frac_part >> 24) + '0');
			frac_part &= 0xFFFFFF;
		}
	}
#endif // the not __AVR part

	return (*this);
}


//-------------------------------------------------------------------------------------
/** This operator writes a double-precision floating point number to the serial port in
 *  exponential format (always with the 'e' notation). It calls the utility function 
 *  __ftoa_engine, which is hiding in the AVR libraries, used by the Xprintf() 
 *  functions when they need to convert a double into text. 
 *  @return A reference to the serial device to which the data was printed. This
 *          reference is used to string printable items together with "<<" operators
 *  @param num The double-precision floating point number to be sent out
 */

emstream& emstream::operator << (double num)
{
#ifdef __AVR
	uint8_t digit = precision;
	char buf[20];
	char* p_buf = buf;

	int exponent = __ftoa_engine (num, buf, digit, 16);
	uint8_t vtype = *p_buf++;
	if (vtype & FTOA_NAN)
	{
		*this << "  NaN";
		return (*this);
	}

	// Display the sign if it's negative
	if (vtype & FTOA_MINUS)
		putchar ('-');

	// Show the mantissa
	putchar (*p_buf++);
	if (digit)
		putchar ('.');
	do
		putchar (*p_buf++);
	while (--digit && *p_buf);

	// Now display the exponent
	putchar ('E');
	if (exponent > 0)
		putchar ('+');
	*this << exponent;
#else
	*this << (float)num;
#endif

	return (*this);
}

//-------------------------------------------------------------------------------------
/** @brief   Read a floating point number from a serial device.
 *  @details This low-budget imitation of @c istream reads a floating point number. 
 *           Digits before and after the decimal point are read and converted until a
 *           non-numeric, non-decimal-point character is found, at which (not)point 
 *           the conversion is finished.
 *           @li @b FEATURE: If a negative sign is found @e anywhere before the 
 *               beginning of the number (not just immediately to the left of the first
 *               numeric digit), the number is considered negative. 
 *           @li @b FEATURE: The number must begin with a numeric digit, so numbers
 *               smaller than 1.0 must begin with a 0, not a decimal point.  The 
 *               string "0.1" is allowed, but ".1" is @b not @b allowed.
 *  @return  A reference to the serial device from which the data was read. This
 *           reference is used to string many read commands together if needed
 *  @param   number The number to be filled with the data that has been read
 */

emstream& emstream::operator >> (float& number)
{
	char in_ch;
	bool negative = false;

	// Skip any non-numeric characters until we get to the numeric ones or a sign
	do
	{
		in_ch = getchar ();

		if (in_ch == '-')                   // If we've found a minus sign,
		{                                   // the number is considered negative
			negative = true;
		}
	}
	while (in_ch < '0' || in_ch > '9');

	// Beginning with the character read by the function which has called this one, 
	// get all the digits until we run out of numeric characters or encounter a
	// decimal point; put each into the result number
	float temp_number = (float)(in_ch - '0');

	for (;;)
	{
		in_ch = peek ();
		if (in_ch == 127 || in_ch == 8)        // If a backspace, erase last digit
		{
			temp_number /= 10.0;
		}
		else if (in_ch < '0' || in_ch > '9')   // Anything else: end of integer part
		{
			break;
		}
		else
		{
			temp_number *= 10.0;
			temp_number += (uint16_t)(in_ch - '0');
		}
		getchar ();                            // If we're here, we got another number
	}

	// If we just ran into a decimal point, begin translating the fractional part. If 
	// it wasn't a decimal point, we're done converting the number
	if (in_ch == '.')
	{
		getchar ();
		float fraction_order = 0.1;

		for (;;)
		{
			in_ch = peek ();
			if (in_ch == 127 || in_ch == 8)    // If a backspace, erase last digit
			{
				temp_number /= 10.0;
			}
			else if (in_ch < '0' || in_ch > '9')   // Anything else: end of number
			{
				break;
			}
			else
			{
				temp_number += (float)(in_ch - '0') * fraction_order;
				fraction_order /= 10.0;
			}
			getchar ();                        // If we're here, we got another digit
		}
	}

	// Return the number we've found, attaching the correct sign
	if (negative)
	{
		number = -temp_number;
	}
	else
	{
		number = temp_number;
	}

	return *this;
}


//*************************************************************************************
/** @file    config_file.h
 *  @brief   Header for a base class for small program configuration files.
 *  @details This file contains headers for a base class used to create a parser for 
 *           really simple configuration files which are to be kept on an SD cards or
 *           similar devices. The SD card (or other device) is expected to be used in 
 *           a small microcontroller based data logger or similar device. This base 
 *           class provides generic configuration file reading functions; it is 
 *           expected that descendents will be designed to read specific files in 
 *           specific projects. 
 *
 *  Revisions:
 *    \li 01-15-2008 JRR Original (somewhat useful) file
 *    \li 12-27-2009 JRR Added tools for reading booleans and integers
 *    \li 10-19-2014 JRR Made compatible with all @c emstream descendents
 *
 *  License:
 *    This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

/// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include "sdio_card.h"                      // The driver class for SD card devices


/** This definition specifies which character will be seen as beginning a comment. The 
 *  most common choice is usually the pound sign (or number sign, or hash). 
 */
const char CFG_COMMENT_CHAR = '#';

/** This constant specifies the character which is the last character on a line. It is
 *  usually a newline '\n'.
 */
const char CFG_END_OF_LINE = '\n';

/** @brief   Debugging printout for the A/D converter.
 *  @details If @c CFGF_DBG is defined as a statement which prints something, this 
 *           function macro writes debugging information to the serial device pointed
 *           to by @c p_serial, if that serial device pointer points to an actual 
 *           device. If the @c CFGF_DBG is defined as nothing, this macro evaluates 
 *           to nothing, turning debugging statements off. 
 *  @param   line A line of printable stuff, separated by @c << operators, which 
 *           follows the first @c << in a printing line
 */
#define CFGF_DBG(line) if (p_serial) *p_serial << line
// #define CFGF_DBG(line)


//=====================================================================================
/** @brief   Base class for a configuration file reader and parser.
 *  @details This class parses a configuration file which is kept on some type of 
 *           serially accessed device such as an SD card. The configuration file is 
 *           made up in sections, with sections being indicated by a text header in 
 *           brackets; the file contains items which map to variables that will be 
 *           kept in the configuration, generally numbers and flags (boolean values) 
 *           and, in the future, perhaps text strings and comments. The items are 
 *           distinguished as follows: 
 *       \li Comments are put in the file line by line; they begin with a number sign 
 *           (@c #). When a number sign is found, the rest of the line is ignored. A 
 *           comment may be on a line after a data item. 
 *       \li Numbers are expected to be integers (floats may be supported in the 
 *           future). They are given one per line. It's OK to have comments after the 
 *           numbers. 
 *       \li Boolean values can be given as text which begins with @c Y, @c y, @c T, 
 *           @c t, or @c On for values which are true; @c N, @c n, @c F, @c f, and 
 *           @c Off represent false values. 
 *
 *  An example file looks like the following:
 *    @code
 *    # Sample configuration file
 *    # This comment explains something really important.
 *    157            # Numerator
 *    100            # Denominator
 *    -13            # Offset
 *    
 *    # End of file
 *    @endcode
 */

class config_file
{
	protected:
		/** @brief   A pointer to the SD card driver.
		 *  @details This pointer points to the SD card driver which is used to
		 *           access the configuration file.
		 */
		sd_card* p_card;

		/** @brief   Pointer to a serial device used for debugging messages.
		 *  @details This pointer points to a serial port which can be used for 
		 *           debugging. If left at @c NULL, debugging printouts won't take 
		 *           place.
		*/
		emstream* p_serial;

	public:
		// The constructor allocates memory and saves some pointers
		config_file (sd_card* p_sd_card, emstream* p_ser_dev);

		// The read function is to be overridden in descendent classes
		virtual void read (char const*);

		// Skip to the beginning of the next non-comment line
		int16_t skip_to_next_line (void);

		// Ignore the rest of what's on the current line (usually a comment)
		void skip_to_EOL (void);

		// Read a Boolean item from a configuration file
		uint8_t read_bool (uint16_t);
};

#endif // _CONFIG_FILE_H_

//*************************************************************************************
/** @file    config_file.cpp
 *  @brief   Source code for a base class for small program configuration files.
 *  @details This file contains the source for a base class used to create a parser for
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

#include "config_file.h"


//-------------------------------------------------------------------------------------
/** @brief   Constructor for a base configuration file parser.
 *  @details This constructor creates the base class portion of a configuration file 
 *           parser which reads files on a given serial decice. 
 *  @param   p_sd_card Pointer to a device such as an SD card on which the file is kept
 *  @param   p_ser_dev Serial device (such as a port) on which to show debugging 
 *                     information
 */

config_file::config_file (sd_card* p_sd_card, emstream* p_ser_dev) 
{
	// Save the SD card pointer
	p_card = p_sd_card;

	// ...and the serial device pointer
	p_serial = p_ser_dev;
}


//-------------------------------------------------------------------------------------
/** @brief   Read a configuration file, parsing each line to find settings.
 *  @details This is a base class method to read a configuration file. It only reads 
 *           and prints out the file, if debugging is properly enabled. Descendent 
 *           classes need to implement their own versions of @c read() which are 
 *           appropriate for configurations of specific systems. 
 *  @param   file_name Pointer to a string containing the name of the file to be read
 */

void config_file::read (char const* file_name)
{
	#ifndef TEST_BY_PRINTING
		(void)file_name;
	#else
		// Open the configuration file if possible
		if (p_card->open_file_readonly (file_name) != 0)
		{
			CFGF_DBG ("Cannot open config file " << file_name << endl);
			return;
		}

		// Read each character in the file; run a state machine based on characters which
		// have been found
		uint16_t file_size = 0;
		char a_ch;

		// In this simple test version, just print out the file
		while ((a_ch = (char)p_card->getchar ()) != (-1))
		{
			p_glb_dbg_port->putchar (a_ch);
			file_size++;
		}
		CFGF_DBG (endl << "Config file size: " << file_size << endl);
	#endif
}


//-------------------------------------------------------------------------------------
/** @brief   Skip whitespace characters and comments until data is found.
 *  @details This method reads characters, skipping whitespace (spaces and returns) 
 *           and ignoring lines which are comments, until it has found the first 
 *           character on the next active (non-comment) line. That first character is 
 *           returned, and the file pointer is left pointing to the character after 
 *           that first character.
 *  @return  The first non-comment character found, or (-1) if error or end of file
 */

int16_t config_file::skip_to_next_line (void)
{
	char a_ch;

	// Skip the rest of the current line
	skip_to_EOL ();

	// Grab characters; respond to each appropriately until we get to the desired first
	// character on a line or we run out of characters in the file
	while (true)
	{
		a_ch = p_card->peek ();

		switch (a_ch)
		{
			// If a comment character is seen, skip the rest of this line too
			case (CFG_COMMENT_CHAR):
				skip_to_EOL ();
				break;

			// The carriage return should be right before a linefeed which ends the
			// line; just go on to the next character
			case ('\r'):

			// A carriage return means we need to go around the loop and examine the 
			// next character to be found
			case (CFG_END_OF_LINE):
				p_card->getchar ();
				break;

			// A (0xFF) means that we've come to the end of the file or an error
			case (0xFF):
				return (-1);

			// Any other character should be the start of usable data
			default:
				return ((char)a_ch);
		};
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Skip from a comment delimiter character to the end of a line.
 *  @details This method is called when a comment character has been found. It reads 
 *           and ignores all characters up to the end of the current line. The file 
 *           pointer is moved forward in the file, but nothing else is changed. 
 */

void config_file::skip_to_EOL (void)
{
	uint16_t a_ch;
	while ((a_ch = p_card->getchar ()) != (0xFF) && a_ch != CFG_END_OF_LINE)
	{
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Read one boolean value from a configuration file.
 *  @details This method reads a Boolean value from the configuration file. The value 
 *           may be represented by various symbols; anything beginning with @c [YyTt] 
 *           is taken as meaning true; anything beginning with @c [NnFf] is taken as 
 *           false; if the first letter is @c [Oo] then the second letter is checked, 
 *           as we've probably got @c On or @c Off.
 *  @param   a_char The first character of the item, often found by skip_to_next_line()
 *  @return  0 for false, 1 for true, and 0xFF for undetermined (not a valid Boolean)
 */

uint8_t config_file::read_bool (uint16_t a_char)
{
	uint8_t to_return = 0xFF;				// Value to return, error by default

	switch (a_char)
	{
		// If we have the first letter in an obvious true word, set the return value
		case 'Y':
		case 'y':
		case 'T':
		case 't':
			to_return = 1;
			break;

		// If we have the first letter in what must be false, set the return value
		case 'N':
		case 'n':
		case 'F':
		case 'f':
			to_return = 0;
			break;

		// If we have an 'O' or an 'o', it could be on or off; find out which one
		// by looking at the second character
		case 'O':
		case 'o':
			a_char = p_card->getchar ();

			if (a_char == 'N' || a_char == 'n')
			{
				to_return = 1;
			}
			else if (a_char == 'F' || a_char == 'f')
			{
				to_return = 0;
			}
			else
			{
				to_return = 0xFF;
			}
			break;

		// Any other character in this location means we're reading junk; return an 
		// error code
		default:
			to_return = 0xFF;
			break;
	};

	// Return the value 
	return (to_return);
}


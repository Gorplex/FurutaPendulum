//*************************************************************************************
/** @file    logger_config.cpp
 *  @brief   Source code for a parser for data logging configuration files.
 *  @details This file contains source code for a parser for simple configuration files
 *           that set up channels on a simple data logger using a PolyDAQ 2 data 
 *           acquisition board. 
 *
 *  Revisions:
 *    \li 12-30-2009 JRR Original file
 *    \li 05-23-2011 JRR Modified for use with interrupt timed data acquisition
 *    \li 02-23-2015 JRR Rearranged for PolyDAQ 2 with more channels and calibration
 *
 *  License:
 *    This file is copyright 2015 by JR Ridgely and released under the Lesser GNU 
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

#include "logger_config.h"                  // The header for the class in this file


//-------------------------------------------------------------------------------------
/** @brief   Construct a logger configuration file reader object.
 *  @details This constructor creates a strain logger configuration file reader but 
 *           does not yet read the configuration file. That's done in @c read(), so
 *           please be patient and scroll down to that function, and quit complaining.
 *  @param   p_sd_card A pointer to the SD card on which the configuration is stored
 *  @param   p_ser_dev Serial device (such as a port) on which to show debugging 
 *                     information
 */

logger_config::logger_config (sd_card* p_sd_card, emstream* p_ser_dev) 
{
	p_card = p_sd_card;                     // Save the SD card pointer
	p_serial = p_ser_dev;                   // ...and the serial device pointer

	config_valid = false;                   // No valid configuration has yet been read

	p_first_config_data = NULL;                   // Begin with no line configurations
	p_current_config_data = p_first_config_data;  // and set pointers to NULL

	ticks_per_reading = 100;                // Take data at 0.1 s per point by default
}


//-------------------------------------------------------------------------------------
/** @brief   Read a data logger configuration file and figure out what it means.
 *  @details This method reads configuration data from a file of the given name and 
 *           stores the configuration data in member variables belonging to this class. 
 *  @param   file_name The name of the file in which the configuration is stored
 */

void logger_config::read (char const* file_name)
{
	// Open the file if possible; if not, exit this function
	if (p_card->open_file_readonly (file_name) != 0)
	{
		CFGF_DBG (PMS ("Cannot open config file ") << file_name << endl);
		return;
	}

	// The configuration will be considered false until we've read good data
	config_valid = false;

	// Clear the configuration so we can fill the new one from scratch
	clear ();

    // Call read_line() to read each line in the file and parse it 
	while (read_line ())
	{
	}

	// Done!  Close the file
	if (p_card->close_file () != 0)
	{
		CFGF_DBG (PMS ("Problem closing config. file") << endl);
	}

	// We seem to have a valid configuration now
	config_valid = true;
}


//-------------------------------------------------------------------------------------
/** @brief   Read one line from the configuration file and respond to it.
 *  @details This method reads a active line from the PolyDAQ 2 configuration file. It 
 *           runs a state machine to handle the various different types of characters
 *           that might be found on any given line. 
 *  @returns @c true if there's more data to be read in the file, @c false if not
 */

bool logger_config::read_line (void)
{
	char ch_in;                             // A character read from the config. file
	uint8_t rl_state = 0;                   // Runs a line reading state machine

	while (!(p_card->eof ()) && (ch_in = p_card->getchar ()) != '\n')
	{
		// All newlines are ignored no matter when
		if (ch_in != '\r')
		{
			// This state machine controls the reading of the line. The reader will be
			// waiting to find out what's on the line, or reading particular content,
			// or skipping over comments, etc. 
			switch (rl_state)
			{
				// Case 0: Just beginning to read the line; examine first character
				case 0:
					if (ch_in == 'T')      // A 'T' line contains time per data point
					{
						rl_state = 1;
					}
					else if (ch_in == 'C')  // A 'C' means channel configuration line
					{
						rl_state = 2;
					}
					else if (ch_in == ' ' || ch_in == '\t')   // Skip spaces and tabs
					{
					}
					else          // If any other character, skip the rest of the line
					{
						rl_state = 9;
					}
					break;

				// Case 1: Read an integer which holds ms per data point, dump the rest
				case 1:
					*p_card >> ms_per_reading;
					ticks_per_reading = ms_per_reading * configTICK_RATE_HZ / 1000;
					rl_state = 9;
					break;

				// Case 2: Read a channel configuration line
				case 2:
					read_channel_config ();
					rl_state = 9;
					break;

				// Case 9: Skip comments until end of line (carriage return)
				case 9:
// 					if (p_card->eof ())
// 					{
// 						CFGF_DBG ("(eof)");
// 						return false;
// 					}
					if (ch_in == '\n')
					{
						rl_state = 0;
					}
					break;

				// Default:  We shouldn't ever get here
				default:
					CFGF_DBG (PMS ("ERROR: Log config reader in state ") 
							  << rl_state << endl);
					break;
			};
		}                                   // if !newline
	}                                       // while !eol && !eof()

	// If we're at the end of file, return false; otherwise return true
	if (p_card->eof ())
	{
		return false;
	}
	else
	{
		return true;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Read a channel configuration line from the configuration file.
 *  @details This method reads a single active line from the PolyDAQ 2 configuration
 *           file. It looks at the first non-whitespace character and calls the 
 *           appropriate other method to deal with that character.
 */

void logger_config::read_channel_config (void)
{
	char ch_in;                             // Character read from the SD card
	char label_buf[MAX_COL_LABEL_LEN + 1];  // Temporary storage for column label

	// Save a copy of the previous column configuration pointer
	logger_col_cfg* p_previous_config_data = p_current_config_data;

	// Create a new data column configuration object
	p_current_config_data = new logger_col_cfg;

	// Skip colons, spaces and tabs, then read the channel command
	while ((ch_in = p_card->peek ()) == ' ' || ch_in == '\t' || ch_in == ':')
	{
		p_card->getchar ();
	}
	*p_card >> p_current_config_data->command;

	// Read the slope, then offset
	*p_card >> p_current_config_data->slope
			>> p_current_config_data->offset;

	// Now read the label.  First, look for the beginning quotation mark(s)
	while ((ch_in = p_card->getchar ()) != '"' && ch_in != '\'' && ch_in)
	{
	}

	// Read up to MAX_COL_LABEL_LEN characters into a temporary buffer
	uint8_t index;
	for (index = 0; index < MAX_COL_LABEL_LEN; index++)
	{
		ch_in = p_card->getchar ();
		if (ch_in == '\r' || ch_in == '\n' || ch_in == '\"' || ch_in == '\'' || !ch_in)
		{
			break;
		}
		else
		{
			label_buf[index] = ch_in;
		}
	}
	label_buf[index] = '\0';

	// Copy the string we've grabbed into a buffer owned by the line configuration
	if ((p_current_config_data->p_label = new char[strlen (label_buf) + 1]) != NULL)
	{
		strcpy (p_current_config_data->p_label, label_buf);
	}

	// If this is the first configuration, set the first configuration pointer
	if (p_first_config_data == NULL)
	{
		p_first_config_data = p_current_config_data;
	}
	else    // Not the first configuration; make the previous one point to this one
	{
		p_previous_config_data->p_next = p_current_config_data;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Clear the logger configuration so that it can be re-read from scratch.
 *  @details This method clears out all the logger configuration data. It deletes the
 *           memory used by the channel configurations if deletion is permitted by the
 *           memory model being used. 
 */

void logger_config::clear (void)
{
	config_valid = false;                   // No valid configuration has yet been read

	ticks_per_reading = 100;                   // Take data at 0.1 s per point by default

	// Delete the column data, but only if the memory manager permits deletion
	#undef MMANG_ALLOWS_DELETION
	#ifdef MMANG_ALLOWS_DELETION
		logger_col_cfg* p_col_cfg = p_first_config_data;
		logger_col_cfg* p_temp_cfg;
		while (p_col_cfg != NULL)
		{
			p_temp_cfg = p_col_cfg->p_next;
			delete p_col_cfg;
			p_col_cfg = p_temp_cfg;
		}
	#endif

	p_first_config_data = NULL;                   // Begin with no line configurations
	p_current_config_data = p_first_config_data;  // and set pointers to NULL
}


//-------------------------------------------------------------------------------------
/// @brief   Diagnostic printout of all the data from a logger column configuration.
emstream& operator << (emstream& str, logger_col_cfg& log_col_cfg)
{
	str << PMS ("Command: '") << log_col_cfg.command 
		<< PMS ("', Slope: ") << log_col_cfg.slope
		<< PMS (", Offset: ") << log_col_cfg.offset 
		<< PMS (", Label: \"") << log_col_cfg.p_label
		<< '"';

	return str;
}


//-------------------------------------------------------------------------------------
/// @brief   Diagnostic printout of all the data from a logger configuration.
emstream& operator << (emstream& str, logger_config& log_cfg)
{
	
	str << PMS ("Logger config: ") << log_cfg.ticks_per_reading << PMS (" ticks = ") 
		<< log_cfg.ms_per_reading << PMS (" ms") << endl;

	logger_col_cfg* p_col_cfg = log_cfg.p_first_config_data;
	while (p_col_cfg != NULL)
	{
		str << *p_col_cfg << endl;
		p_col_cfg = p_col_cfg->p_next;
	}

	return str;
}


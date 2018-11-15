//*************************************************************************************
/** @file    logger_config.h
 *  @brief   Headers for a parser for data logging configuration files.
 *  @details This file contains headers for a parser for simple configuration files 
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

/// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _LOGGER_CONFIG_H_
#define _LOGGER_CONFIG_H_

#include "string.h"                         // For C string handling functions
#include "FreeRTOS.h"                       // This version relies on FreeRTOS config.
#include "sdio_card.h"                      // The card on which config files stay
#include "config_file.h"                    // The base for configuration file parsers


/** This is the number of milliseconds per interrupt. It's controlled elsewhere, and
 *  the correct value needs to be set here for proprely calibrated timing. As of the
 *  creation of this variable, interrupt timing is set in task_sd_card.cpp.
 */
const uint8_t MS_PER_READING = 5;

/** @brief The maximum number of milliseconds per data point, giving the slowest
 *  acceptable sample rate. 
 */
const uint16_t MAX_MS_PER_READING = 60000;  // 1 data line per minute

/** @brief The minimum number of milliseconds per data point, giving the fastest
 *  acceptable sample rate. 
 */
const uint16_t MIN_MS_PER_READING = 1;      // 100 data lines per second

/** @brief The number of A/D channels in the on/off list; for AVR processors this
 *  should almost always be 8 and for ARM processors 18 or so. 
 */
const uint8_t N_A2D_CHANNELS = 16;

/** @brief The maximum length of a column's label (i.e. the name of one column of 
 * data).
 */
const uint8_t MAX_COL_LABEL_LEN = 24;


//-------------------------------------------------------------------------------------
/** @brief   Data for one sensor line in the logger configuration file.
 *  @details This structure holds the data which is read from a single line of the
 *           logger configuration file. Each line configures one sensor with the
 *           following data:
 *
 *           | Item    | Description |
 *           |:--------|:----|
 *           | Command | A one-character command that causes logging of this channel |
 *           | Slope   | The slope for conversions from raw data to what is saved |
 *           | Offset  | The number added to the scaled (by slope) data before saving |
 * 
 *           This class is used as a simple data structure, so its data is public and
 *           methods aren't needed. 
 */

class logger_col_cfg
{
public:
	/// @brief   The command character which causes the PolyDAQ to read the channel
	char command;

	/// @brief   The slope for converting raw data to what is saved
	float slope;

	/// @brief   The offset for converting the scaled data to what is saved
	float offset;

	/// @brief   The label (name) of the given column of data
	char* p_label;

	/// @brief   Pointer to the next configuration, or @c NULL if this is the last one.
	logger_col_cfg* p_next;

	/// @brief   The constructor sets default values for everything
	logger_col_cfg (void)
	{
		command = '0';
		slope = 1.0;
		offset = 0.0;
		p_label = NULL;
		p_next = NULL;
	}
};

// Diagnostic printout of all the data from the logger configuration.
emstream& operator << (emstream& str, logger_col_cfg& log_cfg);


//-------------------------------------------------------------------------------------
/** @brief   Parser for simple data logger configuration files.
 *  @details This class parses simple configuration files, kept on SD cards, for 
 *           Swoop or PolyDAQ 2 data loggers. Data is recorded by taking measurements 
 *           from the specified channels, at the given rate in milliseconds per sample.
 *           The data is scaled by a simple linear gain and offset, then written onto
 *           the SD card. 
 *
 *           The configuration file can contain the following: 
 *           \li Comments which describe what's in the file. A comment begins with a 
 *               @c "#" and lasts to the end of a line.
 *           \li Empty lines, which are ignored.
 *           \li Data lines which begin with a single letter followed by a colon.
 *               Valid data lines include: 
 * 
 *               | Letter | Meaning |
 *               |:------:|:--------|
 *               |   H    | Header line showing type of PolyDAQ (not used)   |
 *               |   B    | Baud rate for serial interface to GUI (not used) |
 *               |   T    | Time per data sample, in milliseconds            |
 *               |   C    | Channel configuration line, as detailed below    |
 * 
 *           Channel configuration lines look like the following: 
 *           @code 
 *           C: 9, 1.0, 0.0, "Cow Strain"
 *           @endcode
 *           The items in the channel configuration line are as follows:
 * 
 *           |     |          |
 *           |:---:|:---------|
 *           | @c 9 | The single-letter command to read data, such as @c 5 or @c X |
 *           | @c 1.0 | Gain by which raw data is multiplied before being saved |
 *           | @c 0.0 | Offset added to data after gain and before data is saved |
 *           | @c "Cow Strain" | Channel name written to data file as column header |
 */

class logger_config         // : public config_file
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

	/** @brief   Number of RTOS ticks per sample set.
	 *  @details The RTOS timer which controls data recording runs much faster 
	 *           than we usually save data; data is only saved once in every 
	 *           this-many RTOS timer ticks.  With an ordinary RTOS timer
	 *           setting of one tick per millisecond, data is taken once in every
	 *           this many milliseconds. 
	 */
	uint16_t ticks_per_reading;

	/** @brief   Number of milliseconds per sample set.
	 *  @details The number of milliseconds per sample set is what's read from the 
	 *           configuration file; it's converted into RTOS ticks to be used by the
	 *           data acquisition task and kept in @c ticks_per_reading.
	 */
	uint16_t ms_per_reading;

	/** @brief   Pointer to the first structure of channel configuration data.
	 *  @details This pointer points to the first of the channel settings: which 
	 *           data channels are to be measured using what commands, and the 
	 *           calibrations for each channel. A linked list of channel 
	 *           configuration structures will hold the channel data to be taken.
	 */
	logger_col_cfg* p_first_config_data;

	/** @brief   Pointer to the channel configuration currently being used.
	 *  @details This pointer is used to move through the list of channel
	 *           configurations as a line of data is being saved to a file.
	 */
	logger_col_cfg* p_current_config_data;

	/** @brief   This boolean is true only if a valid configuration has been read.
	 */
	bool config_valid;

public:
	// The constructor allocates memory and saves pointers
	logger_config (sd_card* p_sdc, emstream* p_ser_dev);

	// Read the file (this overrides the parent class's method)
	void read (char const* file_name);

	// Read one line of information from the file
	bool read_line (void);

	// Read a channel configuration line from the file
	void read_channel_config (void);

	// Clear a channel configuration before re-reading it
	void clear (void);

	/** @brief   Get the number of RTOS ticks per sample set.
	 *  @details This method returns the number of RTOS ticks per data row specified
	 *           in the configuration file.
	 *  @return  The number of RTOS ticks per sample set
	 */
	uint16_t get_ticks_per_sample (void)
	{
		return (ticks_per_reading);
	}

	/** @brief   Get the number of milliseconds per sample set.
	 *  @details This method returns the number of milliseconds per data row
	 *           specified in the configuration file. Each data row typically has
	 *           several readings. 
	 *  @return  The number of milliseconds per sample set
	 */
	uint16_t get_ms_per_sample (void)
	{
		return (ms_per_reading);
	}

	/** @brief   Find out whether a valid configuration has been read.
	 *  @details This method returns a Boolean that indicates if a valid and
	 *           complete configuration has been successfully read from the
	 *           configuration file.
	 *  @return  @b true if the configuration data is valid, @b false if not
	 */
	bool is_valid (void)
	{
		return (config_valid);
	}

	/** @brief   Return a pointer to the first channel configuration.
	 */
	logger_col_cfg* get_first_channel (void)
	{
		p_current_config_data = p_first_config_data;
		return (p_first_config_data);
	}

	/** @brief   Return a pointer to the next channel configuration in the list.
	 */
	logger_col_cfg* get_next_channel (void)
	{
		if (p_current_config_data)
		{
			p_current_config_data = p_current_config_data->p_next;
			return (p_current_config_data);
		}
		else
		{
			return NULL;
		}
	}

	// The logger configuration printout needs to be a friend function
	friend emstream& operator << (emstream& str, logger_config& log_cfg);
};

// Diagnostic printout of all the data from the logger configuration
emstream& operator << (emstream& str, logger_config& log_cfg);

#endif // _LOGGER_CONFIG_H_

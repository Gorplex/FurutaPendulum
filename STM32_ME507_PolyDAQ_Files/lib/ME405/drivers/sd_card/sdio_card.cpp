//*************************************************************************************
/** @file    sdio_card.cpp
 *  @brief   Source code for a class which reads and writes SD cards.
 *  @details This file contains a class which allows SD cards to be read and written, 
 *           using the SDIO protocol, on STM32F4 processors.  The intent is that SD 
 *           cards can be convenient mechanisms for the storage of lots and lots of 
 *           data. The @c emstream base class is used so the SD card driver can write 
 *           data as text using easy-to-use @c << operators. 
 *
 *           This version of the sd_card driver is based on ELM-FAT-FS, a package of 
 *           DOS file system drivers found at 
 *           @c http://elm-chan.org/fsw/ff/00index_e.html. ELM-FAT-FS was chosen 
 *           because it is open source and is currently being maintained, while some 
 *           other packages are in a more static state, their authors having presumably 
 *           moved on. 
 *
 *  Revisions:
 *    \li 06-09-2008 JRR Class created, based on C program example found at
 *                       http://www.captain.at/electronic-atmega-mmc.php
 *    \li 08-17-2008 JRR Actually found the time to get it working
 *    \li 12-09-2008 JRR Added FAT16 filesystem support using DOSFS package from
 *                       http://www.larwe.com/zws/products/dosfs/index.html
 *    \li 02-26-2009 JRR Found time to fix lots of bugs in DOSFS version
 *    \li 11-21-2009 JRR The DOSFS code is still buggy; trying ELM-FAT-FS version from
 *                       http://elm-chan.org/fsw/ff/00index_e.html
 *    \li 12-16-2009 JRR The ELM-FAT-FS version seems reasonably debugged and usable
 *    \li 10-27-2012 JRR Updated with ELM FatFs version 0.09a which is dated 8-27-2012
 *    \li 10-30-2012 JRR Fixed a bug which was causing extra memory usage
 *    \li 09-15-2014 JRR Version created that works with STM32F4xx processor
 *    \li 10-14-2014 JRR Actually got the "working" version for STM32F4xx working, using
 *                       code at @c http://stm32f4-discovery.com/2014/07/
 *                               @c library-21-read-sd-card-fatfs-stm32f4xx-devices/
 *
 *  License:
 *    This file is copyright 2014 by JR Ridgely, whose contributions are released 
 *    under the Lesser GNU Public License, version 3. This software is intended for 
 *    educational use only, but its use is not limited thereto. The ELM-FAT-FS package
 *    on which this program relies is subject to its own free software license, which
 *    is nearly unrestricted; see the ELM-FAT-FS link above for details about the 
 *    license governing FatFS use. Software from the ST Microelectronics STM32 
 *    Standard Peripheral Library is used, and users must abide by that license in 
 *    using this software. In addition, software by Tilen Majerle is included; 
 *    Majerle's components are licensed under the GNU General Public License, version 
 *    3, which is more restrictive than other licenses which apply to this code. A 
 *    result of all these licenses is that this software is free software, and any 
 *    software and/or systems in which it is used (derivative works) must also be free
 *    software and systems, and anyone who uses or distributes such software or 
 *    systems must make this software freely available to users or customers. */
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

#include <string.h>                         // Header for handling file name strings
#include "sdio_card.h"                      // Pull in this class's header file
#include "FreeRTOS.h"                       // Use RTOS to get system time stamps
#include "task.h"                           // FreeRTOS task functions 


/// @brief    The year used for time stamps on SD card data files. 
#define SD_CARD_YEAR        2015

/** @brief    The month used for time stamps on SD card data files.
 *  @details  The date is set to January 1 to prevent files' timestamps from having
 *            future times, which can confuse operating systems that read the files.
 */
#define SD_CARD_MONTH       1

/** @brief    The day used for time stamps on SD card data files.
 *  @details  The date is set to January 1 to prevent files' timestamps from having
 *            future times, which can confuse operating systems that read the files.
 */
#define SD_CARD_DAY         1


//-------------------------------------------------------------------------------------
/** @brief   Create a timestamp for files.
 *  @details This is a real time clock function to be called from the FatFs module. 
 *           It uses the best approximation to a real-time clock which is available on
 *           the computer in use. For a typical embedded module running FreeRTOS, the
 *           only real-time number available is the number of RTOS ticks which have
 *           occurred since the RTOS was started, so we make that number into a 
 *           timestamp. The date is entirely arbitrary; it's set at January 1 so that
 *           the file's timestamp can show a later date of file creation when the RTOS
 *           has been running for many days. 
 *  @return  A 32-bit number containing the date and time in FAT file system format
 */

DWORD get_fattime (void)
{
	// Find the number of seconds since the RTOS was started
	uint32_t seconds = xTaskGetTickCount () / configTICK_RATE_HZ;

	// Pack date and time into a DWORD variable
	return ((DWORD)((SD_CARD_YEAR - 1980) << 25)
			| ((DWORD)(SD_CARD_MONTH + (seconds / 2678400)) << 21)         // Months
			| ((DWORD)((SD_CARD_DAY + ((seconds / 86400) % 31)) << 16))    // Days
			| ((DWORD)(seconds / 3600) << 11)                              // Hours
			| ((DWORD)((seconds / 60) % 60) << 5)                          // Minutes
			| ((DWORD)(seconds % 60) >> 1));                               // Seconds
}


//-------------------------------------------------------------------------------------
/** @brief   Create an SD card interface.
 *  @details This constructor sets up an SD card interface. It calls the constructor 
 *           of its base class, @c emstream, and initializes internal variables so that
 *           the driver can't write anything until a card has been mounted. 
 * 
 *           If the AVR version of this driver, it initializes a data buffer so that 
 *           the buffer is ready to accept some data; it also configures SPI port pins
 *           so that they are ready to communicate with the card. The constructor does
 *           not initialize the card; initialization is performed in @c mount(). 
 *  @param   p_ser_dev Pointer to a serial device which is used to display or log
 *           debugging output. If not given, the pointer defaults to NULL and no
 *           debugging output is displayed or logged
 */

sd_card::sd_card (emstream* p_ser_dev) : emstream ()
{
	// Save the serial device pointer, or NULL if no pointer was given
	p_serial = p_ser_dev;

	// Say hello if in debugging mode
	SD_DBG (PMS ("sd_card constructor") << endl);

	// Since no card has been mounted and no file opened, set error flags to prevent
	// any attempts at writing until a file has been opened
	mounted = false;
	dir_file_result = FR_NOT_READY;

	// Initialize the data buffer to its default size and mark it as empty
	p_buffer = new char[SD_SECTORS_TO_WRITE * _MAX_SS];
	chars_in_buffer = 0;
}


//-------------------------------------------------------------------------------------
/** @brief   Check if the SD card is mounted and a file is open and ready for data.
 *  @details This method checks if the card is ready, with a file opened, for some  
 *           data to be sent to it. If a file is opened, the status flags will have 
 *           been set to indicate that file writing can be done. 
 *  @return  @c true if there's a card ready for data and @c false if not
 */

bool sd_card::ready_to_send (void)
{
	return (mounted && (dir_file_result == 0));
}


//-------------------------------------------------------------------------------------
/** @brief   Write one character to the SD card.
 *  @details This method puts one character into the buffer of data to be written to 
 *           the card. If the buffer becomes full through this write, the buffer is 
 *           automatically written to the card. If there is not a card open and ready
 *           to receive data, this method just discards the data. 
 *  @param   to_put The character to be written into the buffer
 */

void sd_card::putchar (char to_put)
{
	if (!ready_to_send ())
	{
		return;
	}

	// Put the character into the sector buffer; if the buffer is full, write it
	if (p_buffer)
	{
		p_buffer[chars_in_buffer++] = to_put;

		if (chars_in_buffer >= (SD_SECTORS_TO_WRITE * _MAX_SS))
		{
// 			SD_DBG ("<put:" << chars_in_buffer);                              /////////////////////////////////
// 			uint32_t begin_time = (uint32_t)xTaskGetTickCount ();             /////////////////////////////////

			#ifdef SD_USE_BLINKY
				sd_card_LED = true;
			#endif

			UINT n_written;
			f_write (&the_file, p_buffer, chars_in_buffer, &n_written);
			dir_file_result = f_sync (&the_file);
			if (dir_file_result)
			{
				SD_DBG (PMS ("SD sync problem") << endl);
			}
			chars_in_buffer = 0;

			#ifdef SD_USE_BLINKY
				sd_card_LED = false;
			#endif

// 			SD_DBG (" in:" << (uint32_t)xTaskGetTickCount () - begin_time << '>' << endl);  /////////////////////////////////
		}
	}

// 	// Give the character to the existing library function
// 	f_putc (to_put, &the_file);
}


//-------------------------------------------------------------------------------------
/** @brief   Write a character string to the SD card.
 *  @details This method writes all the characters in a string to the SD card. The
 *           end of the string is recognized when a @c '\0' character is found. 
 *  @param   str The string to be written to the card
 */

void sd_card::puts (char const* str)
{
	// Call f_puts() from the library
	if (ready_to_send ())
	{
		f_puts (str, &the_file);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Read one character from the file.
 *  @details This method reads one character from the file, if there is one available.
 *           It returns the character if a character was successfully read; if no 
 *           character could be read it returns 0.
 *  @return  The character which was read, or 0 if at end of file or nothing's there
 */

char sd_card::getchar (void)
{
	if ((!mounted) || dir_file_result || f_eof (&the_file))
	{
		return (0);
	}

	// OK, there's a file open here; get something from it
	char ch_to_ret[2];
	UINT bytes_read;

	dir_file_result = f_read (&the_file, ch_to_ret, 1, &bytes_read);
	if (bytes_read != 1 || dir_file_result != FR_OK)
	{
		return (0);
	}

	return (ch_to_ret[0]);
}


//-------------------------------------------------------------------------------------
/** @brief   Read one character from the file, but leave it to be read again.
 *  @details This method reads one character from the file, if there is one available;
 *           it does not advance the file pointer, so a subsequent call to @c getchar()
 *           will return the character read here, not the next character.  This method
 *           returns the character if a character was successfully read; if no 
 *           character could be read it returns 0.
 *  @return  The character which was read, or 0 if at end of file or nothing's there
 */

char sd_card::peek (void)
{
	if ((!mounted) || dir_file_result)
	{
		return (0);
	}

	// OK, there's a file open here; get something from it
	char ch_to_ret[2];
	UINT bytes_read;
	dir_file_result = f_read (&the_file, ch_to_ret, 1, &bytes_read);
	if (bytes_read != 1 || dir_file_result != FR_OK)
	{
		return (0);
	}

	// We've just advanced the file pointer; we need to move it back so that this
	// character can be read again
	dir_file_result = f_lseek (&the_file, f_tell (&the_file) - 1);

	return (ch_to_ret[0]);
}


//-------------------------------------------------------------------------------------
/** @brief   Cause SD card data to be written to the card immediately.
 *  @details This method writes the data from the buffer to the card. It may take a 
 *           comparatively long time to write data to the card, so this function might
 *           slow down the task in which it runs. 
 */

void sd_card::transmit_now (void)
{
	// If there's not a mounted card present, we can't transmit anything
	if ((!mounted) || dir_file_result || (chars_in_buffer == 0))
	{
		return;
	}

	// Write all the characters currently in the buffer to the card
	UINT n_written;
	f_write (&the_file, p_buffer, chars_in_buffer, &n_written);
	chars_in_buffer = 0;

	// Call the synchronization function to make sure data is physically written to
	// the card, not just held in the filesystem's memory buffer (which is separate
	// from the buffer accessed as p_buffer)
	dir_file_result = f_sync (&the_file);

	if (dir_file_result)
	{
		SD_DBG (PMS ("SD sync problem") << endl);
		return;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Mount the SD card, preparing it for reading and/or writing.
 *  @details This method "mounts" the SD card (in POSIX terminology). This means that
 *           the card interface is initialized, then the card's boot record and 
 *           partition table (and all that oddball stuff) is scanned so that we will
 *           be able to find files on the card. Partition number 0 is always mounted. 
 *  @return  The result code from attempting to open the root directory; 0 means OK
 */

FRESULT sd_card::mount (void)
{
	// This causes ELM-FAT-FS to use the given file system object for storing data
	// about the current file system; no disk access occurs yet. Because the only
	// possible error is "invalid drive number" and we don't use multiple drives, 
	// the return value is ignored. 
	dir_file_result = f_mount (&the_fat_fs, "", 1);

	// Check the result to see if the card has been mounted; 0 means it was mounted
	mounted = (dir_file_result == FR_OK);

// 	SD_DBG ("sd_card::mount() returns " << (uint8_t)dir_file_result << endl);

	return (dir_file_result);
}


//-------------------------------------------------------------------------------------
/** @brief   Unmount the SD card if it is mounted.
 *  @details This method un-mounts the SD card, meaning that it writes any buffered 
 *           but unwritten data to the card, then makes it impossible to write data to
 *           the card until the card has been mounted again. 
 *  @return  The result code from attempting to unmount the card; 0 means OK
 */

FRESULT sd_card::unmount (void)
{
	// Call the FATFS driver to do the actual un-mounting
	dir_file_result = f_mount (NULL, "", 1);

	// Set the flag so that the software knows no card is mounted
	mounted = false;

// 	SD_DBG ("sd_card::unmount() returns " << (uint8_t)dir_file_result << endl);

	return (dir_file_result);
}


//-------------------------------------------------------------------------------------
/** @brief   Open a directory on the SD card.
 *  @details This method opens the given directory. The directory is identified by a 
 *           path name which may go into several layers of subdirectories but must be 
 *           less than 64 characters long. A valid path name might be 
 *           @c /data/strain.dat/recent -- note that forward slashes are used (POSIX 
 *           style), not backslashes in the Windows(tm) style, and that 8.3 names are 
 *           required unless long file names are enabled in FatFS (which they usually
 *           aren't because of the extra memory required). 
 *           TODO:  Test this thing thoroughly. 
 *  @param   path A character string containing the full path name of the directory
 *  @return  The result code (see @c ff.h) indicating how directory opening went; 
 *           0 means things went OK
 */

FRESULT sd_card::open_directory (const char* path)
{
	DIR a_dir;
	dir_file_result = f_opendir (&a_dir, path);

	return (dir_file_result);
}


//-------------------------------------------------------------------------------------
/** @brief   Open a file in read-only mode.
 *  @details This method opens the given file in read-only mode. The file must exist, 
 *           and the volume in which the file exists must have already been mounted. 
 *  @param   path_name A path to the file to be opened
 *  @return  A value (see @c ff.h) which tells if the file was opened successfully; 
 *           0 means things went OK
 */

FRESULT sd_card::open_file_readonly (const char* path_name)
{
	// Check the status of mounting the card
	if (mounted == false)
	{
		SD_DBG (PMS ("Can't open SD card: Not mounted") << endl);
		return (FR_NOT_READY);
	}

	// Call the file opening function to do the real work
	dir_file_result = f_open (&the_file, path_name, (FA_READ | FA_OPEN_EXISTING));

	// Check the result code
	if (dir_file_result)
	{
		SD_DBG (PMS ("Can't open file ") << path_name << PMS (", code ") 
			 << (uint8_t)dir_file_result << endl);
	}

	// If we get here, the file was opened successfully
	return (dir_file_result);
}


//-------------------------------------------------------------------------------------
/** @brief   Open a file such that the file's data will be overwritten.
 *  @details This method opens the given file for writing (the file can also be read).
 *           It leaves the read/write pointer at the beginning of the file so that old
 *           data will be overwritten.
 *  @param   path_name The path name to the file, including the directory
 *  @return  The result code from the @c f_open() call attempting to open the file
 */

FRESULT sd_card::open_file_overwrite (const char* path_name)
{
	// Check the status of mounting the card
	if (!mounted)
	{
		SD_DBG (PMS ("Can't open SD card: Not mounted") << endl);
		return (FR_NOT_READY);
	}

	// Call the file opening function to do the real work
	dir_file_result = f_open (&the_file, path_name, (FA_WRITE | FA_CREATE_ALWAYS));

	// Check the result code
	if (dir_file_result)
	{
		SD_DBG (PMS ("Can't open file ") << path_name << PMS (", code ") 
				<< (uint8_t)dir_file_result << endl);
	}

	// If we get here, the file was opened successfully
	return (dir_file_result);
}


//-------------------------------------------------------------------------------------
/** @brief   Open a file so that data can be appended to the end.
 *  @details This method opens the given file for writing (the file can also be read).
 *           If the file doesn't yet exist, a new empty file is created; if it exists, 
 *           it's opened and the write pointer is put to the end of the file so that
 *           whatever is written will be added to the file. 
 *  @param   path_name The path name to the file, including the directory
 *  @return  A result code from the calls attempting to open the file
 */

FRESULT sd_card::open_file_append (const char* path_name)
{
	//////////////////////// NOT TESTED YET //////////////////////////
	// Check the status of mounting the card
	if (!mounted)
	{
		SD_DBG (PMS ("Can't open SD card: Not initialized") << endl);
		return (FR_NOT_READY);
	}

	// Call the file opening function to do the real work
	dir_file_result = f_open (&the_file, path_name, (FA_WRITE | FA_OPEN_ALWAYS));

	// Check the result code
	if (dir_file_result)
	{
		SD_DBG (PMS ("Can't open file ") << path_name << PMS (", code ") 
				<< (uint8_t)dir_file_result << endl);
		return (dir_file_result);
	}

	// Go to the end of the file so we can append data
	dir_file_result = f_lseek (&the_file, f_size (&the_file));
	if (dir_file_result)
	{
		SD_DBG (PMS ("Can't get to end of file ") << path_name << PMS (", code ")
				<< (uint8_t)dir_file_result << endl);
		return (dir_file_result);
	}

	// If we get here, the file was opened successfully
	return (dir_file_result);
	////////////////////////////////////////////////////////////////////
}


//-------------------------------------------------------------------------------------
/** @brief   Close a file, flushing buffer contents to the disk.
 *  @details This method closes a file. If there is any data left in the buffer, it is
 *           written to the card. Then further writing to this file is prevented unless
 *           the file is opened again.
 *  @return  The result code from calling @c f_close() to close the file
 */

FRESULT sd_card::close_file (void)
{
	// If there isn't an open file, we can't really close a file
	if ((!mounted) || (dir_file_result != FR_OK))
	{
		SD_DBG (PMS ("Cannot close file; none open") << endl);
		return (FR_INT_ERR);
	}

	// Make sure all the data in the buffer has been written to the file
	f_sync (&the_file);

	// No more file access is permitted until the file is reopened
	dir_file_result = f_close (&the_file);
	return (dir_file_result);
}


//--------------------------------------------------------------------------------------
/** @brief   Open a new data file with an automatically generated name.
 *  @details This function opens a new data file, in or under the card's root directory,
 *           with a name which hasn't yet been used. Names are made by putting together
 *           a path, base name, number, and extension as in "/data/file_012.txt". The 
 *           base name variable includes the path and the file name together, separated
 *           by forward slashes. 
 *  @param   base_name The base name, including path, such as "/logs/data_" where the
 *           non-path part of the file name should be at most 5 characters long because 
 *           3 digits will be added. The total base name must be no more than 24

 *  @param   extension The 3-character file name extension, such as "txt" or "csv".
 *  @return  The number used to make the new data file's name, or 0xFFFF for failure
 */

uint16_t sd_card::open_new_data_file (char const* base_name, char const* extension)
{
	FILINFO file_info;                      // Holds information about existing files

	// Check the status of mounting the card
	if (!mounted)
	{
		SD_DBG (PMS ("Can't open SD: Not initialized") << endl);
		return (0xFFFF);
	}

	// Allocate a buffer for the file name
	char buffer[32];

	// Try lots of file names until an unused one is found
	for (uint16_t number = 0; number < 1000; number++)
	{
		// Make an extra pointer to the buffer
		char* pbuf = buffer;

		// First put the path and file name into the buffer
		uint8_t path_size = strlen (base_name);
		strncpy (pbuf, base_name, path_size);
		pbuf += path_size;

		// Now add the digits of the number
		uint16_t temp_num = number;
		*pbuf++ = (temp_num / 100) + '0';
		temp_num %= 100;
		*pbuf++ = (temp_num / 10) + '0';
		temp_num %= 10;
		*pbuf++ = temp_num + '0';

		// Add a dot and the extension and a '\0' to terminate the string
		*pbuf++ = '.';
		strncpy (pbuf, extension, 3);
		pbuf += 3;
		*pbuf = '\0';

// 		SD_DBG (PMS ("Checking ") << buffer << "...");

		// Check if the file exists
		dir_file_result = f_stat (buffer, &file_info);

		switch (dir_file_result)
		{
			// If the error is that the file doesn't exist, good! Use that file name
			case FR_NO_FILE:
// 				SD_DBG (PMS ("Opening..."));
				dir_file_result = open_file_overwrite (buffer);
// 				SD_DBG (PMS ("OK") << endl);
				return (number);

			// If there's no error, the file exists, so don't use that name. Loop
			// again and try another name
			case FR_OK:
// 				SD_DBG (PMS ("Found it") << endl);
				break;

			// Any other error means trouble, so exit this function
			default:
				SD_DBG (PMS ("Error ") << (uint8_t)dir_file_result 
						<< PMS (" opening data file") << endl);
				return (0xFFFF);
		}
	}

	// If we get here, no file name was usable (there were 1000 files?!?!)
	return (0xFFFF);
}


//--------------------------------------------------------------------------------------
/** @brief   Method which returns true if at end of file.
 *  @details This method returns true if we're at the end of a file and false if
 *           there is still more data to be read from the file.
 *  @return  @c true if at end of file, @c false if not
 */

bool sd_card::eof (void)
{
	// If there isn't an open file, return true -- there's no more data available
	if ((!mounted) || (dir_file_result != FR_OK))
	{
		SD_DBG (PMS ("No file open; this is the end") << endl);
		return (true);
	}

	// Call the low-level function to check for end of file
	return (f_eof (&the_file));
}

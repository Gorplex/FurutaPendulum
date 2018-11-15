//*************************************************************************************
/** @file    sdio_card.h
 *  @brief   Headers for a class which reads and writes SD cards.
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

/// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _SDIO_CARD_H_
#define _SDIO_CARD_H_

#include <stdlib.h>                         // Standard C functions
#include "emstream.h"                       // Pull in the base class header file
extern "C"
{
	#include "ff.h"                         // Header for DOS filesystem driver
	#include "diskio.h"                     // The ELM-FAT-FS header for disk I/O
	#include "fatfs_sd_sdio.h"              // Header for FATFS on SD card library
}


/** @brief   Number of 512-byte sectors on the SD card to be written at once.
 *  @details Writing multiple sectors at a time can improve SD card data logging 
 *           performance dramatically. This macro specifies how many sectors' worth
 *           of data is to be buffered in memory by default (the code may allow
 *           setting the size of the memory buffer as the program is started up, so
 *           as to optimize speed against not having too much unsaved data when a
 *           card is removed from the socket or power is turned off). @b Limitation:
 *           since a 16-bit counter is used to keep track of the bytes in the buffer,
 *           the number of sectors times the sector size must be @e less than 64K. 
 */
#define SD_SECTORS_TO_WRITE 16

/** @brief   Macro to implement debugging statements through a serial port.
 *  @details We can turn serial port debugging on or off with this define. Defining 
 *           @c SD_DBG as a serial port write makes it active; defining it as nothing 
 *           prevents debugging printouts, saving time and memory. 
 */
// #define SD_DBG(x) if (p_serial) *p_serial << x
#define SD_DBG(x)

/** @brief   Define used to enable the use of an LED to indicate card activity.
 */
#define SD_USE_BLINKY
// #undef SD_USE_BLINKY
#ifdef SD_USE_BLINKY
	extern bool sd_card_LED;
#endif


//-------------------------------------------------------------------------------------
/** @brief   Class which reads and writes SD cards using the SDIO interface. 
 *  @details This class implements an interface to an SD card using the @c emstream 
 *           class hierarchy and its overloaded \c "<<" operator. This allows user 
 *           programs to write data in ASCII text format to the card using simple 
 *           constructions such as the following:
 *           @code
 *           my_card << "Time: " << system_time << " Data: " << some_data << endl;
 *           @endcode
 *
 *           The ELM-FAT-FS library is used for its FAT file system drivers. This 
 *           library can be found at \c http://elm-chan.org/fsw/ff/00index_e.html. 
 * 
 *           @section sd_prep_card Preparing an SD Card for Use
 *           SD cards used by this program should be formatted as MS-DOS style volumes
 *           using the FAT16 or FAT32 formatting style. This is the way most SD cards
 *           are sold, so in most cases there should be no problem using cards out
 *           of the box, except for getting the box open (SD card bubble packs are
 *           frequently very difficult to open, and proper use of a bandsaw is often
 *           the most effective way to access the cards). 
 *
 *           @section sd_card_drv Using the SD Card Driver Software
 *           In order to use the SD card driver, one must have an STM32 board with an
 *           SD card slot, and the board must be configured to operate with Vcc at 
 *           3.3 volts. It is possible that multiple SD cards could be used on the 
 *           same board, but this has not been tested, and it would be rather weird 
 *           to want to do so. Hot swapping SD cards is possible, but it can be
 *           problematic; the circuit requires a fairly large power supply decoupling
 *           capacitor to prevent brownouts due to the large power drain as the SD
 *           card is inserted into its slot. Also, making sure that all your data 
 *           reads and writes are finished before removing the card can be a problem. 
 *
 *           Configuration of the software is done by modifying values in @c sd_card.h,
 *           @c ffconf.h, and @c sdio_defines.h. Recommended settings for @c ffconf.h 
 *          (which configures the filesystem drivers) include the following:
 *    <table border="1" cellspacing="0" cellpadding="2">
 *     <tr><td> _FS_TINY </td><td> 1 </td><td> unless you have > 4K SRAM </td></tr>
 *     <tr><td> _FS_READONLY </td><td> 0 </td><td> to enable writing </td></tr>
 *     <tr><td> _FS_MINIMIZE </td><td> 1 </td><td> to save space </td></tr>
 *     <tr><td> _USE_STRFUNC </td><td> 0 </td><td> strings not needed here </td></tr>
 *     <tr><td> _USE_MKFS </td><td> 0 </td><td> unless you need to format </td></tr>
 *     <tr><td> _USE_FORWARD </td><td> 0 </td><td> unless you need...this </td></tr>
 *     <tr><td> _CODE_PAGE </td><td> 1 </td><td> for ASCII coding </td></tr>
 *     <tr><td> _USE_LFN </td><td> 0 </td><td> to use short (8.3) filenames </td></tr>
 *    </table> 
 *
 *    The processs of reading or writing a file consists of the following steps: 
 *    \li Create the SD card driver. For example: 
 *        @code
 *        sd_card* p_sdcard = new sd_card (p_serial);
 *        @endcode
 *    \li Mount the card using the @c mount() method. This is Unix-talk for getting 
 *        data from the boot record about where the partitions and base directories 
 *        on the card can be found.  For example: 
 *        @code
 *        if (p_sdcard->mount () == FR_OK)
 *        {
 *            // Try opening the file and if that works, reading or writing data
 *        }
 *        else
 *        {
 *            // Error handling
 *        }
 *        @endcode
 *    \li Open a file using a method such as @c open_file_readonly(), 
 *        @c open_file_overwrite(), etc. An example which opens a data logging file
 *        with an automatically generated name:
 *        @code
 *        data_file_number = (uint8_t)(p_sdcard->open_new_data_file ("data_", "txt"));
 *        @endcode
 *    \li Use the overloaded shift operator @c << to write data to the file or use the
 *        more primitive methods @c getchar(), @c putchar(), and @c puts() to read and
 *        write characters from and to the file. 
 *        @code
 *        *p_sdcard << "Time,X,Y" << endl;
 *        for (uint32_t count = 0; count < 1000; count++)
 *        {
 *            *p_sdcard << system_time << ',' << data[0] << ',' << data[1] << endl;
 *        }
 *        @endcode
 *    \li If at all possible, use the @c close_file() method to make sure all the data
 *        is saved to the file. If a file is not closed properly, some data may be  
 *        left in memory buffer and not written to the disk. Alternatively, wait a
 *        while after the last needed data is taken, preferably enough time to take
 *        an extra thousand bytes' readings or so. 
 *
 *  \section sd_tested Functions tested and working
 *    \li Mounting an SD card formatted in the typical way without multiple partitions
 *    \li Automatically creating data files in the root directory and writing to them
 *    \li Opening existing files for reading
 *    \li Reading a plain text configuration file from a card. Software which reads 
 *        and parses configurations is in files config_file.* and logger_config.*
 *
 *  \section sd_bugs Bugs and limitations
 *    \li Only smaller cards of less than or equal to 2GB are supported.
 *    \li When using SD cards formatted with one or more partitions, sometimes the
 *        files written by this driver aren't visible to a computer reading the card.
 *        Workaround: Make sure to use cards which don't have partitions on them. 
 *    \li Writing to the SD card can be very slow, and this code does not attempt to 
 *        prevent blocking of the rest of the system during card writes. 
 *    \li Long file names have not been tested. They're probably not a good idea for
 *        a small embedded system anyway. 
 *
 *  \section sd_int_comp Internal stuff
 *  The DSTATUS flag byte, given in @c diskio.h, holds some status bits for the disk:
 *    \li 0x01: STA_NOINIT
 *    \li 0x02: STA_NODISK
 *    \li 0x04: STA_PROTECT
 *
 *  The FRESULT enumeration, given in @c ff.c, holds the result of a file operation: 
 *    \li 0:  FR_OK
 *    \li 1:  FR_DISK_ERR
 *    \li 2:  FR_INT_ERR
 *    \li 3:  FR_NOT_READY
 *    \li 4:  FR_NO_FILE
 *    \li 5:  FR_NO_PATH
 *    \li 6:  FR_INVALID_NAME
 *    \li 7:  FR_DENIED
 *    \li 8:  FR_EXIST
 *    \li 9:  FR_INVALID_OBJECT
 *    \li 10: FR_WRITE_PROTECTED
 *    \li 11: FR_INVALID_DRIVE
 *    \li 12: FR_NOT_ENABLED
 *    \li 13: FR_NO_FILESYSTEM
 *    \li 14: FR_MKFS_ABORTED
 *    \li 15: FR_TIMEOUT
 *
 */

class sd_card : public emstream
{
	protected:
		/// @brief This is a filesystem data structure (see @c ff.h). 
		FATFS the_fat_fs;

		/// @brief This flag is set when a card has been successfully mounted.
		bool mounted;

		/// @brief This item holds a result code from opening a directory and/or a file.
		FRESULT dir_file_result;

		/// @brief This is a directory data structure (see @c ff.h).
		DIR the_dir;

		/// @brief This data structure stores information about a file on the disk.
		FIL the_file;

		/// @brief Buffer used to hold lots of bytes to improve writing performance.
		char* p_buffer;

		/// @brief The number of characters currently stored in the buffer.
		uint16_t chars_in_buffer;

		/// This pointer points to a serial device used for debugging.
		emstream* p_serial;

	public:
		// The constructor sets up the SD card interface
		sd_card (emstream* = NULL);

		// This method overrides the ready method in emstream to check the card
		bool ready_to_send (void);

		// Method which writes one character to the SD card
		void putchar (char);

		// Write a string to the SD card, probably faster than writing the characters
		void puts (char const*);

		// This method reads a character from the SD card
		char getchar (void);

		// Return the next character from the card but don't read past it
		char peek (void);

		// This method writes a block to the SD card immediately
		void transmit_now (void);

		// This method "mounts" the SD card by reading the partition table
		FRESULT mount (void);

		// Method which unmounts the SD card which has been mounted
		FRESULT unmount (void);

		// Method which returns true if at end of file
		bool eof (void);
		
		// Open a directory so we can read and write files therein
		FRESULT open_directory (const char*);

		// Open a file in read-only mode
		FRESULT open_file_readonly (const char*);

		// Open a file in write mode so that old data will be overwritten
		FRESULT open_file_overwrite (const char*);

		// Open a file in append mode so that data can be written to it
		FRESULT open_file_append (const char*);

		// Close a file by flushing all data from the buffer to the disk
		FRESULT close_file (void);

		// Method to open a data file, automatically making a new name for it
		uint16_t open_new_data_file (char const*, char const*);
};


#endif // _SDIO_CARD_H_

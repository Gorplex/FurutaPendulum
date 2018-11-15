//**************************************************************************************
/** @file    task_sd_card.h
 *  @brief   Headers for a task which saves data to an SD card.
 *  @details This file contains the headers for a task that saves data using the SD 
 *           card on a PolyDAQ board. Commands and data are sent to this task by other 
 *           tasks, and this task handles the saving of data as fast as the SD card can
 *           write it. 
 *
 *  Revisions:
 *    \li ??-??-2013 GDS Original file from the ChibiOS collection of stuff
 *    \li 06-14-2014 JRR Data acquisition task (thread) split off from \c main.c
 *    \li 07-02-2014 JRR User interface spun off from the other tasks
 *    \li 07-25-2014 JRR PC interface cloned from the user interface and modified
 *    \li 10-15-2014 JRR SD card task made from the PC interface task
 *
 *  License:
 *		This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *		Public License, version 2. It intended for educational use only, but its use
 *		is not limited thereto. */
/*		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *		AND	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * 		IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * 		ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * 		LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 * 		TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * 		OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * 		CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * 		OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * 		OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_SD_CARD_H_
#define _TASK_SD_CARD_H_

#include "taskbase.h"                       // Header for the base task class
#include "sdio_card.h"                      // Header for SD card driver using SDIO
#include "logger_config.h"                  // For data logger configuration reader
#include "shares.h"                         // Task queues and shared variables


/** @brief   Time in milliseconds between forced buffer writes to the SD card.
 *  @details This value controls how often a write to the SD card is forced by a call
 *           to the card's @c send_now modifier. Forcing the buffer to be written is 
 *           helpful in that it ensures data will be written to the disk when the card
 *           is removed or power is turned off; however, forcing writes too often can
 *           slow the system down, which may cause trouble when data is being taken at
 *           a high rate. A typical value might be 1000 ms, forcing the buffer to be
 *           written to the SD card once per second. 
 */
const uint16_t SD_TICKS_PER_SYNC = 1000;

/** @brief   Base name of the data file for the logger; a number is added after this 
 *           name.
 *  @details This is the base of the file name used for automatically generated data
 *           files. It should be the first five characters of an eight-character name,
 *           with the last three characters being an automatically generated number.
 */
const char DATA_F_NAME[] = "data_";

/** @brief   Extension of the data file name for the logger.
 *  @details This is the extension of the data file name, which is the part after the
 *           period. The data file name must be at most 8 characters long and the
 *           extension at most 3 characters long in the ancient MS-DOS(tm) style.
 *           Recommended extensions are CSV, for Comma Separated Variable which is a
 *           format easily read by spreadsheet formats, or TXT, which just means that
 *           the data is stored in a text format. 
 */
const char DATA_F_EXT[] = "csv";

/** @brief   Switch to turn SD card task serial debugging messages on or off.
 *  @details We can turn serial port debugging for the SD card task on or off with 
 *           this define. Defining @c SD_TSK_DBG as a serial port write makes it 
 *           active; defining it as nothing blocks debugging printouts from this task.
 */
#define SD_TSK_DBG(x) if (p_serial) *p_serial << x
// #define SD_TSK_DBG(x)


//-------------------------------------------------------------------------------------
/** @brief   Task which saves data to an SD card. 
 *  @details This task saves data to an SD card. The data is transferred to this task
 *           as a stream of bytes in a queue. 
 */

class task_sd_card : public TaskBase
{
public:
	// The constructor allocates memory and sets up the task object
	task_sd_card (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
				  emstream* serpt);

	// The run function contains user code that actually does something
	void run (void);

	// Write a header showing data items to be logged to a serial device
	void write_header (emstream* p_ser_dev);
};


#endif // _TASK_PC_H_

//**************************************************************************************
/** @file task_data_acq.h
 *    This file contains the headers for a demonstration task for an STM32F4 Discovery
 *    board. The task uses the STM32's A/D converter(s) to take some data. 
 *
 *  Revisions:
 *    @li ??-??-2013 Original file from the ChibiOS collection of stuff
 *    @li 06-14-2014 JRR Data acquisition task (thread) split off from @c main.c
 *    @li 08-27-2014 JRR Converted to the latest PolyDAQ 2 version in FreeRTOS
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
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_DATA_ACQ_H_
#define _TASK_DATA_ACQ_H_

#include "polydaq2.h"                       // Drivers for stuff on the PolyDAQ2 board
#include "taskbase.h"                       // This is a task; here's its parent
#include "shares.h"                         // Task queues and shared variables
#include "adc_driver.h"                     // Data acquisition uses an A/D converter


/** @brief   End-of-line character or characters used in data log files.
 *  @details This macro defines the character or characters which will be written to
 *           the end of each line of data which is being saved to the SD card. The
 *           file is typically written in CSV format so it can be easily read by
 *           spreadsheet and mathematical analysis programs. Different operating 
 *           systems use different standards for end-of-line characters; for example,
 *           MS-DOS(tm) and Windows(tm) use @c "\r\n", while Unix(tm), Linux(tm), and
 *           MacOSX(tm) use @c '\\n'.  Many spreadsheet programs seem to work best with
 *           the single carriage return character @c '\\n'. 
 */
// #define DAQ_EOL             "\r\n"
#define DAQ_EOL             '\n'


//-------------------------------------------------------------------------------------
/** @brief   Task which acquires PolyDAQ data at precise time intervals. 
 *  @details This task acquires data. It is run at a high priority so that data can be
 *           taken at precise time intervals. The data is then put into queues so that
 *           other tasks can print the data through a serial port, store the data onto
 *           an SD card, or do whatever else is needed. 
 */

class task_data_acq : public TaskBase
{
private:
	
protected:
	/** @brief   The number of milliseconds per sample taken by this task.
	 */
	portTickType ms_per_sample;

	/** @brief   Pointer to a PolyDAQ driver used by this task.
	 */
	polydaq2* my_poly;

	/** @brief   Data channel which was most recently read, or (-1) for none.
	 */
	char last_data_channel;

	// Check for a character in the command queue and act on it if found
	void run_daq_command (char ch_in);

	// The function which does all the work for the data acquisition task
	void run (void);

public:
	// This constructor creates the data acquisition task
	task_data_acq (const char* name, unsigned portBASE_TYPE prio, size_t stacked, 
				   emstream* serpt, polydaq2* p_polydaq2);
};

#endif // _TASK_DATA_ACQ_H_

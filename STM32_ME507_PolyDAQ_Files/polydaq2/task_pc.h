//**************************************************************************************
/** \file task_pc.h
 *    This file contains the headers for a PC interface task for a PolyDAQ board. The 
 *    task communicates with a Python program running on the PC to create a data
 *    acquisition system. 
 *
 *  Revisions:
 *    \li ??-??-2013 GDS Original file from the ChibiOS collection of stuff
 *    \li 06-14-2014 JRR Data acquisition task (thread) split off from \c main.c
 *    \li 07-02-2014 JRR User interface spun off from the other tasks
 *    \li 07-25-2014 JRR PC interface cloned from the user interface and modified
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
#ifndef _TASK_PC_H_
#define _TASK_PC_H_

#include "rs232.h"
#include "taskbase.h"

#include "adc_driver.h"
#include "shares.h"                         // Task queues and shared variables


//-------------------------------------------------------------------------------------
/** @brief   Task for communication with the user or a PC based user interface. 
 *  @details This task interacts with the user. User input, which may be input from 
 *           another computer, such as a PC running interface software written in
 *           Python, Matlab(tm) or LabView(tm), is usually given as single character 
 *           commands. The user interface responds with whatever the designer wishes 
 *           to send to the user or other computer. The use of states is optional, 
 *           unless this task implements the "vi" editor, in which case it's pretty 
 *           much mandatory.  
 */

class task_pc : public TaskBase
{
public:
	// The constructor allocates memory and sets up the task object
	task_pc (const char* name, unsigned portBASE_TYPE prio, size_t stacked, 
			 emstream* serpt);

	// The run function contains user code that actually does something
	void run (void);
};


#endif // _TASK_PC_H_

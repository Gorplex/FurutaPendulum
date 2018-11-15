//**************************************************************************************
/** \file task_user.h
 *    This file contains the headers for a user interface task for an STM32F4 Discovery
 *    board. It watches for user input and responds accordingly.
 *
 *  Revisions:
 *    \li 07-02-2014 JRR User interface spun off from the other tasks
 *
 *  License:
 *		This file is copyright 2012 by JR Ridgely and released under the Lesser GNU 
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
#ifndef _TASK_USER_H_
#define _TASK_USER_H_


#include "taskbase.h"                       // Header for ME405/FreeRTOS tasks
#include "polydaq2.h"                       // Header for PolyDAQ2 driver


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

class task_user : public TaskBase
{
protected:
	/// @brief   Pointer to the PolyDAQ 2 board driver from which to get data
	polydaq2* p_poly;

public:
	// The constructor creates the user interface but doesn't start it yet
	task_user (const char* name, unsigned portBASE_TYPE prio, size_t stacked, 
			   emstream* serpt, polydaq2* p_poly_driver);

	// The main method is the one that runs until the power is turned off
	void run (void);

	// This method displays a help screen
	void show_help (void);
};


//-------------------------------------------------------------------------------------
/** @brief   Task which prints out characters in the main print queue. 
 *  @details This task prints characters which are in the main print queue which is
 *           pointed to be @c p_main_text_queue. If the queue is empty, this task is
 *           blocked by its @c getchar() method and does nothing.
 */

class task_print : public TaskBase
{
public:
	task_print (const char* name, unsigned portBASE_TYPE prio, size_t stacked, 
			   emstream* serpt);

	// The run method checks for characters and prints those it finds
	void run (void);
};

#endif // _TASK_USER_H_

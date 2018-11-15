//*************************************************************************************
/** \file ch_task.cpp
 *    This file contains a wrapper class which makes the use of ChibiOS tasks a bit
 *    easier. The class incorporates a bunch of enhancements which are specifically
 *    designed to aid in the teaching and learning of embedded software programming in
 *    an RTOS environment. These enhancements include convenient diagnostic printing
 *    and links to the \c emstream class hierarchy which implements output to serial
 *    devices using an overloaded \c << operator in a style similar to that of the C++
 *    \c iostream and related classes. 
 *
 *  Revised:
 *    \li 10-21-2012 JRR Original file
 *    \li 12-23-2013 JRR Ported from FreeRTOS to ChibiOS
 *    \li 07-02-2014 JRR Added lots of fancy and made it run on STM32F4 Discovery
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

#ifdef __AVR
	#include <avr/wdt.h>                    // Watchdog timer, used for rebooting
#endif
#include "string.h"                         // For finding the lengths of strings
#include "ch_task.h"                        // Pull in the class header file


// The base classes from which ChTask is descended are in the ChibiOS namespace.
// We'll just add this stuff to the ChibiOS namespace for convenience
namespace chibios_rt
{


/** @brief   Initialize the pointer to the most recently created task.
 *  @details This bit of weird looking template code initializes one variable which is
 *           shared by all task objects. That variable holds a pointer to the most 
 *           recently created task, such variable being used to make a linked list of
 *           tasks.
 */
ChBaseTask* ChBaseTask::last_created_task_pointer = NULL;


//-------------------------------------------------------------------------------------
/** @brief   Create a task which runs in a ChibiOS thread.
 *  @details This constructor creates a task with the given name. It saves a pointer 
 *           to a serial device to be used for debugging; it also saves a pointer to 
 *           the previously created task (if any) so that tasks can form a linked 
 *           list. Any function, such as diagnostic printouts, that is to be performed
 *           by all tasks can be done by telling the most recently created task to do 
 *           it, then having that most recently created task tell the previously 
 *           created task to do it, and so on. 
 */

ChBaseTask::ChBaseTask (void)
	: BaseThread ()
{
	// The serial port pointer doesn't contain anything yet, so set it to NULL
	p_serial = NULL;

	// Initialize the finite state machine and its transition logger
	state = 0;
	previous_state = 0;

	// Set the previous task pointer to whatever the most recent one used to be; if 
	// this is the first created task, it will be NULL, making this the last task in
	// the linked list of tasks
	previous_task_pointer = last_created_task_pointer;

	// Set the pointer to the most recently created task to point to this one
	last_created_task_pointer = this;
}


//-------------------------------------------------------------------------------------
/** @brief   Start the thread in which the task which owns it will run.
 *  @details This method starts up the thread in which a task will run. It calls the
 *           ChibiOS function \c chThdCreateFromHeap(), which allocates the specified
 *           amount of stack (and other stuff if needed) space for this thread 
 *           dynamically from the default heap and then calls the function 
 *           \c _thread_start(), which runs the \c main() method belonging to the 
 *           user's task. 
 *  @param   p_name       The name which will be given to the task
 *  @param   worka_size   The size of the working area to be dynamically allocated
 *  @param   a_priority   The priority at which task will run unless changed later
 *                        (Default: NORMALPRIO)
 *  @param   p_ser_dev    A pointer to a serial device to be used for debugging
 */

ThreadReference ChTaskDynamic::start (const char* p_name,
									  size_t worka_size, 
									  tprio_t a_priority,
									  emstream* p_ser_dev)
{
	// Create the thread in which this task will run
	thread_ref = chThdCreateFromHeap (NULL, 
									worka_size, 
									a_priority, 
									_thread_start, 
									this);

	// Now that we have a thread, it can save a pointer to the task's name
	thread_ref->p_name = p_name;

	// The working area size can't be determined yet
	working_area_size = worka_size;

	// Set the pointer to a serial port
	p_serial = p_ser_dev;

	// If the serial port is being used, let the user know if the task was created
	// successfully
	if (p_serial != NULL)
	{
		if (thread_ref)
		{
			*p_serial << "Task \"" << get_name () << "\" started at " << this 
					  << ", thread at " << thread_ref << endl;
		}
		else
		{
			// If creating the thread went wrong, complain
			*p_serial << "Problem creating task \"" << p_name << '"' << endl;
		}
	}

	// Return a ThreadReference (a pointer to a structure) for the thread that has 
	// just been created
	return (thread_ref);
}


//-------------------------------------------------------------------------------------
/** This method is called within \c run() to cause a state transition. It changes the 
 *  variable 'state', and if transition logging is enabled, it logs the transition to 
 *  help with debugging.
 *  @param new_state The state to which we will transition
 */

void ChBaseTask::transition_to (uint8_t new_state)
{
	state = new_state;

	// If transition tracing is enabled, print data about the transition
	#ifdef TRANSITION_TRACE
		if (p_serial)
		{
			*p_serial << tick_res_time () << ':' << get_name ()
					  << ':' << previous_state << "->" << state << endl;
		}
	#endif // TRANSITION_TRACE

	previous_state = state;
}


//-------------------------------------------------------------------------------------
/** @brief   Print a table showing the tasks and their status.
 *  @details This C function (not a C++ class method) prints information about how all 
 *           the tasks are doing. It uses the last created task pointer to ask the most
 *           recently created task to print its information; then that task tells the
 *           task created before it to print, and so on until all tasks have printed.
 *  @param   ser_dev A pointer to the serial device on which stuff will be printed
 */

void print_task_list (emstream* ser_dev)
{
	*ser_dev << PMS ("Task Name       State\tPri\tThread\tTicks") << endl;
	(ChBaseTask::last_created_task_pointer)->print_status_in_list (ser_dev);
}


//-------------------------------------------------------------------------------------
/** @brief   Print this task's status on one line of a task status table.
 *  @details This method prints this task's status, then calls the next task in the
 *           linked list of tasks to print its status, unless the next task pointer is
 *           NULL, which means this is the first task created and thus the last task
 *           in the linked list of tasks. 
 */

void ChBaseTask::print_status_in_list (emstream* ser_dev)
{
	// Print the task's name, padding it to 12 characters
	*ser_dev << thread_ref->p_name;
	int8_t phil = 12 - strlen (thread_ref->p_name); 
	while (phil-- > 0)
	{
		ser_dev->putchar (' ');
	}

	// Print the task's state and native priority (ignoring temporary priority 
	// elevation if that has occurred) as well as the thread's state
	*ser_dev << '\t' << state << '\t' << thread_ref->p_realprio 
			 << '\t' << thread_state_strings[thread_ref->p_state];
// 			 << '\t' << (uint32_t)thread_ref->p_ctx.r13;

	// If profiling is turned on, print a rough measurement of time used by the task
	#ifdef CH_DBG_THREADS_PROFILING
		*ser_dev << '\t' << thread_ref->p_time;
	#endif

	// It's the end of the line...so to speak
	*ser_dev << endl;

	// If the previous task pointer isn't NULL, ask that task to print its status
	if (previous_task_pointer != NULL)
	{
		previous_task_pointer->print_status_in_list (ser_dev);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Dump the memory of each task's working area to a terminal.
 *  @details This function (not a C++ class method) has each of the tasks in the task
 *           list do a hex dump of the contents of its working memory area to a serial
 *           device.
 *  @param   p_ser_dev A pointer to the serial device on which to dump the text
 */

void print_task_working_areas (emstream* p_ser_dev)
{
	(ChBaseTask::last_created_task_pointer)->print_working_area_in_list (p_ser_dev);
}


//-------------------------------------------------------------------------------------
/** @brief   Print this task's status on one line of a task status table.
 *  @details This method prints this task's status, then calls the next task in the
 *           linked list of tasks to print its status, unless the next task pointer is
 *           NULL, which means this is the first task created and thus the last task
 *           in the linked list of tasks. 
 *  @param   p_ser_dev A pointer to the serial device on which to dump the text
 */

void ChBaseTask::print_working_area_in_list (emstream* p_ser_dev)
{
	// Print the task's name, then a colon
	*p_ser_dev << thread_ref->p_name << ": " << (uint16_t)working_area_size
			   << " bytes at " << hex << (void*)p_working_area << endl;

	// Dump the task's memory contents
	hex_dump_memory ((uint8_t*)p_working_area,
					 (uint8_t*)(p_working_area) + working_area_size, 
					 p_ser_dev);

	// Put an extra end-line at the end for readability
	*p_ser_dev << endl;

	// If the previous task pointer isn't NULL, ask that task to print its status
	if (previous_task_pointer != NULL)
	{
		previous_task_pointer->print_working_area_in_list (p_ser_dev);
	}
}


// //-------------------------------------------------------------------------------------
// /** This method prints an error message and resets the processor. It should only be 
//  *  used in cases of things going seriously to heck.
//  */
// 
// void ChTask::emergency_reset (void)
// {
// 	*p_serial << PMS ("ERROR in task ") << get_name () << PMS ("! Restarting") << endl;
// 	wdt_enable (WDTO_120MS);
// 	for (;;);
// }


//-------------------------------------------------------------------------------------
/** \brief   Run a task's \c main() method.
 *  \details This C function runs the \c main() method of a ChibiOS task when asked
 *           to by the RTOS. 
 *  @param   p_task A pointer to the task whose \c main() method is to be run
 */

msg_t _got_main (void *p_task)
{
	return ((ChBaseTask*)p_task)->main ();
}


} // namespace chibios_rt

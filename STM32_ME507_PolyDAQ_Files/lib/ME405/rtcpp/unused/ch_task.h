//*************************************************************************************
/** \file ch_task.h
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

// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _CH_TASK_H_
#define _CH_TASK_H_

#include "ch.hpp"                            // Main header for ChibiOS
#include "hal.h"                             // Hardware abstraction layer in ChibiOS
#include "emstream.h"                        // Base formatter class header file


// Check to ensure the thread registry is turned on; we need it for task/thread names
#ifndef CH_USE_REGISTRY
	#error The define CH_USE_REGISTRY must be TRUE in chconf.h to use the ChTask class
#endif


// The base classes from which ChTask is descended are in the ChibiOS namespace.
// We'll just add this stuff to the ChibiOS namespace for convenience
namespace chibios_rt
{

//-------------------------------------------------------------------------------------
/** @brief   Table of printable strings for thread states.
 *  @details This table maps thread states to printable indications of those states so 
 *           that when a task prints its status in a list, its thread's state can be 
 *           a useful part of the printout.
 */

const char thread_state_strings[15][4] =
{
	"Rdy",                                  // Waiting on the ready list
	"Run",                                  // Currently running
	"Sus",                                  // Created in suspended state
	"W:s",                                  // Waiting on a semaphore
	"W:m",                                  // Waiting on a mutex
	"W:v",                                  // Waiting on a condition variable
	"Zzz",                                  // Wait in chThdSleep(), chThdSleepUntil()
	"W:w",                                  // Waiting in chThdWait()
	"W:e",                                  // Waiting for an event
	"W:f",                                  // Waiting for several events
	"Mss",                                  // Sending a message, in queue
	"Msw",                                  // Sent a message, waiting answer
	"Mrw",                                  // Waiting for a message
	"W:q",                                  // Waiting on an I/O queue
	"X-("                                   // Thread terminated
};


/** This is the default, and presumably minimum sensible, stack size for any task. 
 */
const size_t MIN_STACK_SIZE = 140;


// This function calls the user's function which runs in this task's thread
msg_t _thread_start (void* arg);


//-------------------------------------------------------------------------------------
/** @brief A C++ wrapper for the ChibiOS thread functions, plus a state machine. 
 * 
 *  \details Details are forthcoming...but the thing hasn't been written yet. 
 * 
 *  \section Usage
 *      We'll figure out how to use this stuff when it has been written. 
 */

class ChBaseTask : public chibios_rt::BaseThread
{
// These private functions can't be accessed from outside this class. The two
// methods here can't be used to construct anything; this is intentional, as
// it prevents an object of this class from being copied. One shouldn't copy a
// task object; it's unnatural and can lead one to moral instability and ennui
private:
	/** This copy constructor is poisoned by being declared private so that it 
	 *  can't be used. One should not copy a task object.
	 *  @param that_clod A reference to a task which ought not be copied
	 */
	ChBaseTask (const ChBaseTask& that_clod);

	/** This assignment operator is poisoned by being declared private so that it
	 *  can't be used to copy the contents of one task into another. There is no
	 *  legitimate, morally acceptable reason to copy tasks that way. 
	 *  @param that_doofus A reference to a task which ought not be copied
	 */
	ChBaseTask& operator= (const ChBaseTask& that_doofus);

	/** @brief   A pointer to the previously created task.
	 *  @details This pointer holds the address of the task created previously to
	 *           this one. It's used to implement a linked list of tasks so that
	 *           tables of information about all the tasks can be created. Because
	 *           tasks are created from a template class, this must be a void 
	 *           pointer; it will be typecast as needed before it is used. 
	 */
	ChBaseTask* previous_task_pointer;

// This protected data can only be accessed from this class or its descendents
protected:
	/** @brief   A pointer to the thread's working area in memory.
	 *  @details This pointer points to the working area used by the task's OS 
	 *           thread to store the task's local stack and other data. 
	 */
	stkalign_t* p_working_area;

	/** @brief   The size of the working area in bytes.
	 *  @details This variable holds the size of the memory area which is used for the
	 *           task's stack and other task data. 
	 */
	size_t working_area_size;

	/** This pointer can point to a serial output device or port which will be used
	 *  for various diagnostic printouts or logging.
	 */
	emstream* p_serial;

	/** This is the state in which the finite state machine of the task is. This
	 *  variable should be used inside run() to implement the state machine so
	 *  that state transitions can be tracked, if needed, by this parent class.
	 */
	uint8_t state;

	/** This variable keeps track of the previous (before run() runs) value of the
	 *  state so that transitions can be conveniently detected.
	 */
	uint8_t previous_state;

	/** This is the method which runs the user's code. It must be overridden in 
	 *  the user's class.
	 *  @return A messasge when the thread exits (which it generally shouldn't).
	 */
	virtual msg_t main (void) = 0;

// Public methods can be called from anywhere in the program where there is a 
// pointer or reference to an object of this class
public:
	// This constructor creates a ChibiOS task with the given name and serial 
	// port. The serial port is used for debugging and can be left empty. 
	explicit ChBaseTask ();

	// This method is called within main() to cause a state transition
	void transition_to (uint8_t);

	/** This method sets the task's serial device pointer to the given address.
	 *  Changing this serial device pointer means that debugging output will be
	 *  directed to the given device. This can be helpful if task transitions or
	 *  other debugging information should be shown on a serial console as the
	 *  task is being created but logged somewhere else when the task is running.
	 *  @param p_new_dev A pointer to the serial device on which debugging
	 *                   information will be printed in the near future
	 */
	void set_serial_device (emstream* p_new_dev)
	{
		p_serial = p_new_dev;
	}

	/** This method un-sets the task's serial device pointer. Doing so prevents
	 *  serial debugging output from being sent or logged in the future unless 
	 *  \c set_serial_device() is called to set the serial device pointer again.
	 */
	void unset_serial_device (void)
	{
		p_serial = NULL;
	}

	/** This method returns the transition logic state in which the task is at the
	 *  current time. This is the value of the variable 'state' which is 
	 *  manipulated by the user within the run() method to cause state transitions.
	 *  @return The current state
	 */
	uint8_t get_state (void)
	{
		return (state);
	}

	/** This method returns a pointer to the task's name, which resides in a null 
	*  terminated character array belonging to the task. 
	*  @return A pointer to the task's name
	*/
	const char* get_name (void)
	{
		return (thread_ref->p_name);
	}

	// This method prints task status information, then asks the next task in the
	// list to do so
	void print_status_in_list (emstream*);

	/** This method prints a stack dump, then asks the next task in the list 
	 *  to do the same, and so on and so on, so all tasks do it
	 */
	void print_working_area_in_list (emstream*);

// 		// This method causes a processor reset if something has gone seriously wrong
// 		void emergency_reset (void);

	/** @brief   A pointer to the most recently created task.
	 *  @details This pointer holds the address of the most recently created task
	 *           in the list of all tasks in the system. Because it's static, one
	 *           copy is shared by all task objects, so any task can find the last
	 *           task created. Since this is a template class and we don't know
	 *           the stack size of each task before descendent classes are written,
	 *           a void pointer is used to hold the address; it will be typecast
	 *           as needed before it is used. 
	 */
	static ChBaseTask* last_created_task_pointer;

	// The _got_main function is permitted to call main()
	friend msg_t _got_main (void*);
};


//-------------------------------------------------------------------------------------
// This operator writes information about the task's status to the given serial device.
// That information can be used for debugging or perhaps reliability testing
emstream& operator << (emstream&, ChBaseTask&);


//-------------------------------------------------------------------------------------
// This function prints information about how all the tasks are doing. Since it's not
// a template (nor even C++) the function body is in the .cpp file. It's declared as a
// regular C function so that it can be called from pretty much anywhere
void print_task_list (emstream* ser_dev);


//-------------------------------------------------------------------------------------
// This function has all the tasks print their stacks
void print_task_working_areas (emstream* p_ser_dev);


//=====================================================================================
/** @brief   Class which implements a task with static working area allocation.
 *  @details This class adds the ability to statically allocate memory for the working
 *           area to the ChibiOS base task class. Once the task object has been 
 *           created, the functionality of the task is the same as for a dynamically
 *           allocated task. Static allocation is generally preferred for embedded
 *           control work because any problems with memory space will show up at 
 *           compile time rather than when the program is running. 
 *  @param wa_size The working area size for the task class
 */

template <size_t wa_size>
class ChTask : public ChBaseTask
{
protected:
	/** This statement creates a working area (space for the task's stack and 
	 *  associated stuff) of a size given in the task's template parameter. The
	 *  definition of \c WORKING_AREA is given for reference: 
	 *  #define \c WORKING_AREA stkalign_t s[THD_WA_SIZE(n) / sizeof(stkalign_t)]
	 */
	WORKING_AREA (thread_working_area, wa_size);

public:
	//---------------------------------------------------------------------------------
	/**
	 * @brief   Constructor for the static version of a ChibiOS task. 
	 * @details This constructor creates a ChibiOS task, allocating memory statically
	 *          at compile time rather than dynamically from the heap at run time. 
	 *          The task's \c main() method is not started; that will be done by a
	 *          call to the \c start() method later. 
	 */
	ChTask<wa_size> (void)
		: ChBaseTask ()
	{
		// Set the pointer to the base of the working area in memory
		p_working_area = thread_working_area;

		// No serial port has been set up yet
		p_serial = NULL;

		// Set the working area size
		working_area_size = wa_size;   ///////////////////////// sizeof (thread_working_area);
	}

	//---------------------------------------------------------------------------------
	/**
	 *  @brief   Set up and run this thread's \c main() method.
	 *  @details This method calls ChibiOS's function \c chThdCreateStatic() to start
	 *           up the thread in which this task will run.
	 *  @param   p_name The name which will be given to the task
	 *  @param   priority The priority at which this thread will begin running. The
	 *               priority can be changed later if need be
	 *  @param   p_ser_dev A pointer to a serial device which can be used for debugging
	 *               or other communications tasks (default: \c NULL)
	 */
	virtual ThreadReference start (const char* p_name,
								   tprio_t priority = NORMALPRIO,
								   emstream* p_ser_dev = NULL)
	{
		// The function which is called by the RTOS is only used within this method,
		// so we just declare it right here
		msg_t _got_main (void *arg);

		// Set the serial port pointer so this task can communicate
		p_serial = p_ser_dev;

		// Tell the RTOS to start up this task's thread, giving it the information
		// it needs to do so. The last parameter, "this", is a pointer to this task;
		// that parameter will be passed on to the function got_main()
		thread_ref = chThdCreateStatic (thread_working_area, 
										sizeof (thread_working_area), 
										priority, _got_main, this);

		// Now that we have a thread, it can save a pointer to the task's name
		thread_ref->p_name = p_name;

	// If the serial port is being used, let the user know if the task was created
	// successfully
	if (p_serial != NULL)
	{
		if (thread_ref)
		{
			*p_serial << "Task \"" << get_name () << "\" started at " 
					  << this << ", thread at " << thread_ref << endl;
		}
		else
		{
			// If creating the thread went wrong, complain
			*p_serial << "Problem creating task " << get_name () << endl;
		}
	}
		// If the task's main() function ever returns, then (1) something is kind of
		// weird (thread main functions generally shouldn't return) and (2) we will
		// get to this point in the code and return a pointer to this task. The RTOS
		// will then do something undefined until TODO: this behavior has been tested
		return *this;
	}

};


//=====================================================================================
/** @brief   Class which implements a task with dynamic working area allocation.
 *  @details This class adds the ability to dynamically allocate memory for the working
 *           area to the ChibiOS base task class. Once the task object has been 
 *           created, the functionality of the task is the same as for a statically
 *           allocated task. Static allocation is generally preferred for embedded
 *           control work because any problems with memory space will show up at 
 *           compile time rather than when the program is running. 
 *  @param wa_size The working area size for the task class
 */

class ChTaskDynamic : public ChBaseTask
{
protected:

public:
	//---------------------------------------------------------------------------------
	/** @brief   Constructor for the dynamic version of a ChibiOS task. 
	 *  @details This constructor creates a ChibiOS task, allocating memory statically
	 *           at compile time rather than dynamically from the heap at run time. 
	 *           The task's \c main() method is not started; that will be done by a
	 *           call to the \c start() method later. 
	 *  @param   p_name A name for the newly created task
	 */
	ChTaskDynamic ()
		: ChBaseTask ()
	{
		// Set the pointer to the base of the working area in memory
		p_working_area = NULL;

		// The working area size can't be determined yet
		working_area_size = 0;
	}

	// Method which allocates memory dynamically, then starts the thread running
	virtual ThreadReference start (const char* p_name, 
								   size_t a_stack_size, 
								   tprio_t a_priority = NORMALPRIO,
								   emstream* p_ser_dev = NULL);
};


} // namespace chibios_rt

#endif  // _CH_TASK_H_

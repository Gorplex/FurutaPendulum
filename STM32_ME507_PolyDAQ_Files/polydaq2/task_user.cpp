//**************************************************************************************
/** \file task_user.cpp
 *    This file contains source code for a user interface task for an STM32F4 Discovery
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

#include "appconfig.h"
#include "polydaq2.h"
#include "task_user.h"
#include "shares.h"
#include "mma8452q.h"                       // I2C accelerometer driver class


//-------------------------------------------------------------------------------------
/** @brief   Creates a user interface task object.
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a user interface task object. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (quite a bit, actually)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 *  @param   p_poly_driver Pointer to a PolyDAQ 2 driver which runs the PolyDAQ board
 */

task_user::task_user (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
					  emstream* serpt, polydaq2* p_poly_driver)
	: TaskBase (p_name, prio, stacked, serpt)
{
	p_poly = p_poly_driver;
}


//-------------------------------------------------------------------------------------
/** @brief   Show a help menu for the user interface.
 *  @details This method writes a list of commands and their functions so that a user
 *           can see how to make the program do whatever it can do.
 */

void task_user::show_help (void)
{
	*p_serial << "PolyDAQ 2 Help" << endl;
	*p_serial << "Measurement:" << endl;
	*p_serial << "  0-9: Get A/D reading on channel 0-9" << endl;
	*p_serial << "  A-F: Get A/D reading on channel 10-15" << endl;
	*p_serial << "  X-Z: Get X, Y, or Z acceleration (onboard)" << endl;
	*p_serial << "  x-z: Get X, Y, or Z acceleration (external)" << endl;
	*p_serial << "  a:   Acceleration (onboard), all 3 axes" << endl;
	*p_serial << "  r:   Fortran compatible acceleration" << endl;
	*p_serial << "  L,M: Auto-balance strain bridge 1, 2" << endl;
	*p_serial << "  O:   Set oversampling; type an integer 0-99" << endl;
	*p_serial << "System Diagnostics:" << endl;
	*p_serial << "  s:   Show system status" << endl;
	*p_serial << "  d:   Dump tasks' memory areas" << endl;
// 	*p_serial << " ^c:   Reset the processor" << endl;
	*p_serial << "  v:   Show program version" << endl;
	*p_serial << "  @:   Scan I2C bus for devices" << endl;
	*p_serial << "  h,?: Show this help screen" << endl;
	*p_serial << "  err: What's That Function?" << endl;
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the user interface thread. 
 *  @details This method checks for characters typed by the user and responds to any
 *           typed characters. 
 *  @return  This method should never return; if it hypothetically did, it would 
 *           return an OS message of zero. 
 */

void task_user::run (void)
{
	char ch_in;                             // A character sent to us by the user
	uint16_t temp_num;                      // Temporary storage for a number
	uint8_t oversampling;                   // How many times to oversample if > 0
	bool clock_check = false;               // Whether to print one-second time stamps

	// Create a variable that holds the last time a time stamp was printed
	float next_show_time = get_tick_time ();
	float start_time;

	for (;;)
	{
		if (p_serial->check_for_char ())
		{
			ch_in = p_serial->getchar ();

			// Reading A/D channels 2 and 3 isn't allowed because it messes up the
			// serial port by reconfiguring serial I/O lines into A/D lines
			if (ch_in == '2' || ch_in == '3')
			{
				*p_serial << PMS ("Cannot read A/D channel ") << ch_in << endl;
			}
			// The characters from 0 through 9 or A through F mean read A/D channels; 
			// characters 'X', 'Y', and 'Z' mean read accelerations; 'L' and 'M' are
			// commands to auto-balance Wheatstone bridges
			else if ((ch_in >= '0' && ch_in <= '9') || (ch_in >= 'A' && ch_in <= 'F') 
				  || (ch_in >= 'X' && ch_in <= 'Z') || (ch_in >= 'x' && ch_in <= 'z'))
			{
				if (oversampling > 1)
				{
					*p_serial << p_poly->get_data_oversampled (ch_in, oversampling) 
							  << endl;
				}
				else
				{
					*p_serial << p_poly->get_data (ch_in) << endl;
				}
			}

			// An 'L' or an 'M' mean that we should auto-balance a strain bridge
			else if (ch_in == 'L' || ch_in == 'M')
			{
				p_poly->strain_auto_balance (ch_in - 'K');       // Channel is 1 or 2
			}

			// The '#' and '$' are used to set D/A output 1 or 2 respectively
			else if (ch_in == '#' || ch_in == '$')
			{
				*p_serial << "Set D/A " << (uint8_t)(ch_in - '"') << " to: ";
				*p_serial >> temp_num;
				p_serial->getchar ();
				*p_serial << temp_num << endl;
				p_poly->set_DAC ((uint8_t)(ch_in - '"'), temp_num);
			}

			// An uppercase 'O' asks how many times to oversample; 0 turns it off
			else if (ch_in == 'O')
			{
				*p_serial >> oversampling;
				p_serial->getchar ();
				*p_serial << "Oversampling ";
				if (oversampling)
				{
					*p_serial << oversampling << " times";
				}
				else
				{
					*p_serial << " off";
				}
				*p_serial << endl;
			}

			// The 'd' command means dump the task stacks onto the user interface
			else if (ch_in == 'd')
			{
				print_task_stacks (p_serial);
			}

			// Typing 's' requests a task and share status table
			else if (ch_in == 's')
			{
				*p_serial << "Status at " << get_tick_time () << " sec:" << endl;
				print_task_list (p_serial);
				*p_serial << endl;
				p_daq_UI_command_queue->print_all_shares (p_serial);
			}

			// The 'a' character runs an I2C accelerometer test
			else if (ch_in == 'a')
			{
				*p_serial << "Accel: " << p_poly->get_data ('X') << ','
						  << p_poly->get_data ('Y') << ',' 
						  << p_poly->get_data ('Z') << endl;
			}

			// Typing 'r' tests Roman numeral printing because it's SO important
			else if (ch_in == 'r')
			{
				*p_serial << "Acceleratio est: " << fortran 
						  << p_poly->get_data ('X') << ','
						  << p_poly->get_data ('Y') << ',' 
						  << p_poly->get_data ('Z') << dec << endl;
			}

			// Typing uppercase 'I' means get an IR temperature
			else if (ch_in == 'I')
			{
				*p_serial << "IR: " << p_poly->get_IR_temperature () << endl;
			}

			// Typing '@' means scan the I2C bus for sensors
			else if (ch_in == '@')
			{
				*p_serial << "I2C Bus:" << endl;
				p_poly->scan_I2C_bus (p_serial);
			}

			// Typing 'c' toggles the clock check function (show time every second)
			else if (ch_in == 'c')
			{
				clock_check = !clock_check;
				next_show_time = get_tick_time ();
				start_time = next_show_time;
			}

			// Typing 'T' gets the time in seconds since the RTOS started up
			else if (ch_in == 'T')
			{
				*p_serial << get_tick_time () << endl;
			}

			// The 'h' or question mark characters request a help display
			else if (ch_in == 'h' || ch_in == '?')
			{
				show_help ();
			}

			// Return a version string if requested
			else if (ch_in == 'v')
			{
				*p_serial << VERSION_STRING << endl;
			}

	// 		// Character number 3 is a control-C which asks for processor reset
	// 		else if (ch_in == 3)
	// 		{
	// 			*p_serial << "Resetting CPU...Aaarrgh, can't do it" << endl;
	// 			chThdSleepMilliseconds (100);
	// 			NVIC_SystemReset ();
	// 		}

			// If any other character is entered, ask "What's That Function?"
			else
			{
				if (ch_in < ' ')
				{
					*p_serial << '(' << (uint8_t)ch_in << PMS ("): WTF?");
				}
				else
				{
					*p_serial << ch_in << PMS (": WTF?");
				}
				*p_serial << endl;
			}
		} // If character entered by user

		// Check if it's time to print a time (if in clock checking mode)
		if (clock_check)
		{
			if (get_tick_time () > next_show_time)
			{
				*p_serial << get_tick_time () - start_time << endl;
				next_show_time += 1.0;
			}
		}

		// Print any characters found in the main print queue
		if (p_main_text_queue->check_for_char ())
		{
			p_serial->putchar (p_main_text_queue->getchar ());
		}

		delay_ms (1);

		runs++;                             // Track how many runs through the loop
	}                                       // for (;;)
}


//-------------------------------------------------------------------------------------
/** @brief   Creates a printing task object.
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a task object which prints things from the text queue.
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 */

task_print::task_print (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
					  emstream* serpt)
	: TaskBase (p_name, prio, stacked, serpt)
{
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the main queue printing thread. 
 *  @details This method checks for characters in the main print queue whose address
 *           is at @c p_main_text_queue and prints any characters found. 
 *  @return  This method should never return; if it hypothetically did, it would 
 *           return an OS message of zero. 
 */

void task_print::run (void)
{
	// No setup needs to be done

	for (;;)
	{
		// Print any characters found in the main print queue
		p_serial->putchar (p_main_text_queue->getchar ());

		runs++;                             // Track how many runs through the loop
	}
}


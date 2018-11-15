//**************************************************************************************
/** \file task_leds.cpp
 *    This file contains source code for an LED blinking task for an STM32 Discovery
 *    board. This task is mostly used to provide assurance that the unit is working. 
 *
 *  Revisions:
 *    \li 08-26-2014 JRR Original file
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

#include "task_leds.h"


//-------------------------------------------------------------------------------------
/** @brief   Constructor for an LED task object.
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating the two-LED blinking task. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 *  @param   p_polydaq2 A pointer to the PolyDAQ 2 driver used to interface with the
 *                      data acquisition board
 */

task_leds::task_leds (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
					  emstream* serpt, polydaq2* p_polydaq2)
	: TaskBase (p_name, prio, stacked, serpt)
{
	// Save the pointer to the PolyDAQ driver, which has its own A/D and D/A drivers
	my_poly = p_polydaq2;

	brightness_SD_LED = 0;
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the LED task code.  
 *  @details This method causes one LED to pulse on and off and another to get brighter
 *           if the user presses a button.
 */

void task_leds::run (void)
{
	// Create a variable used to enable precise task timing by recording start times
	static TickType_t xLastWakeTime = xTaskGetTickCount ();

// 	// How much should the LED brightness change per run?
// 	delta = 1;

	// Initialize the LED driver on the PolyDAQ 2 card
	my_poly->init_SD_card_LED ();

	for (;;)
	{
// 		// Check for a command from another task; if we get one, respond to it
// 		// Commands:  H = heartbeat; Y = turn on; N = turn off; others ignored
// 		if (p_led_command_queue->not_empty ())
// 		{
// 			uint8_t command = p_led_command_queue->get ();
// 
// 			if (command == 'H')
// 			{
// 				brightness_LED_1 = 0;
// 				my_poly->set_SD_card_LED_brightness (0);
// 				transition_to (0);
// 			}
// 			else if (command == 'M')
// 			{
// 				transition_to (2);
// 			}
// 		}

		// Run the state machine that controls signal type
		switch (state)
		{
			// In states 0 and 1, we smoothly brighten and dim the LED to create a 
			// "heartbeat" display that shows that at least the system is operating
			case (0):
				if (++brightness_SD_LED > SD_CARD_LED_MAX_PWM)
				{
					transition_to (1);
				}
				else
				{
					my_poly->set_SD_card_LED_brightness (brightness_SD_LED);
				}
				if (p_led_command_queue->not_empty ())
				{
					if ((p_led_command_queue->get ()) == 'M')
					{
						transition_to (2);
					}
				}
				break;

			// State 1 is the dimming part of the cycle
			case (1):
				if (--brightness_SD_LED == 0)
				{
					transition_to (0);
				}
				else
				{
					my_poly->set_SD_card_LED_brightness (brightness_SD_LED);
				}
				if (p_led_command_queue->not_empty ())
				{
					if ((p_led_command_queue->get ()) == 'M')
					{
						transition_to (2);
					}
				}
				break;

			// In state 2, the LED is controlled entirely by external commands, so
			// nothing is actually done here -- brightness is left as it has been
			case (2):
				if (sd_card_LED)
				{
					my_poly->set_SD_card_LED_brightness (SD_CARD_LED_MAX_PWM);
				}
				else
				{
					my_poly->set_SD_card_LED_brightness (0);
				}
				if (p_led_command_queue->not_empty ())
				{
					if ((p_led_command_queue->get ()) == 'H')
					{
						brightness_SD_LED = 0;
						transition_to (0);
					}
				}
				break;

			// We should never get here. It's not a fatal error, though, just a
			// blinky light malfunction, so go back to the initial state
			default:
				transition_to (0);
				break;

		}; // switch (state)

		runs++;                             // Track how many runs through the loop

		delay_from_for_ms (xLastWakeTime, 1);
	}
}

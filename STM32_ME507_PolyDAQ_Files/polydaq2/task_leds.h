//**************************************************************************************
/** \file task_leds.h
 *    This file contains the headers for an LED blinking task for an STM32 Discovery
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

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _TASK_LEDS_H_
#define _TASK_LEDS_H_

#include "FreeRTOS.h"
#include "task.h"

#include "rs232.h"
#include "polydaq2.h"                       // Drivers for stuff on the PolyDAQ2 board

#include "taskbase.h"                       // Base class for tasks
#include "shares.h"                         // Lists shares and queues between tasks



//-------------------------------------------------------------------------------------
/** @brief   Task which pulses an LED on and off as another responds to a button.
 *  @details This task controls some LED's on the STM32 Discovery board. The blue
 *           LED uses a PWM to glow brighter and dimmer, and the orange one can be
 *           lit up or turned off by using the blue button. 
 */

class task_leds : public TaskBase
{
protected:
	/// @brief   The current brightness of the LED, as a PWM duty cycle.
	int16_t brightness_SD_LED;

	/// @brief   Pointer to a PolyDAQ driver used by this task.
	polydaq2* my_poly;

	/// @brief   Flag which indicates whether LED is in manual or heartbeat mode.
	bool manual_mode;

public:
	// The constructor sets up the task object and links it to a PolyDAQ 2 driver
	task_leds (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
			   emstream* serpt, polydaq2* p_polydaq2);

	// The run method manages the LED as the program runs
	void run (void);

	/// @brief   Turn off the LED, making sure it's not in an automatic mode.
	void off (void)
	{
		manual_mode = true;
		my_poly->set_SD_card_LED_brightness (0);
	}

	/// @brief   Turn on the LED, making sure it's not in an automatic mode.
	void on (void)
	{
		manual_mode = true;
		my_poly->set_SD_card_LED_brightness (SD_CARD_LED_MAX_PWM);
	}

	/// @brief   Put the LED in automatic "heartbeat" display mode.
	void heartbeat (void)
	{
		manual_mode = false;
	}
};


#endif // _TASK_LEDS_H_

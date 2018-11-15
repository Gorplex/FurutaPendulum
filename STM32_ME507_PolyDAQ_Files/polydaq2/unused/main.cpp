/*
    ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string.h>
#include <math.h>
#include <stdlib.h>                         // Need this for rand()

// #include "stm32f4xx.h"

#include "ch.hpp"                           // ChibiOS base header
#include "hal.h"                            // ChibiOS hardware abstration layer

#include "rs232.h"
#include "ch_task.h"
#include "ch_queue.h"
#include "dac_driver.h"

#include "task_data_acq.h"                  // Headers for all the task classes
#include "task_save_data.h"
#include "task_accel.h"
#include "task_pc.h"
#include "task_user.h"
#include "shares.h"


// In order to use the C++ wrappers for ChibiOS, we need the namespace
using namespace chibios_rt;


//-------------------------------------------------------------------------------------
/** This task runs the accelerometer and controls the brightnesses of the four user 
 *  LED's to indicate measured acceleration.
 */
static AccelTask the_accel_task;

/** This task takes data from the A/D converter and puts it into a queue. 
 */
static task_data_acq the_data_acq_task;

/** This task saves data which has been taken and queued by the A/D task.
 */
static task_save_data the_save_data_task;

/** This task communicates with the PC, responding to its commands by changing data
 *  acquisition parameters and sending data.
 */
static task_pc the_pc_task;

/** This is the one and only instance of the user interface task; it looks for 
 *  characters typed by the user and responds to them.
 */
static task_user the_user_task;


//=====================================================================================
/** \brief   This function runs when the application is started up.
 *  \details The \c main() function initializes the RTOS system, starts the threads 
 *           running, and then enters an infinite loop which constitutes a low priority
 *           task. 
 */

int main (void) 
{
	/* System initializations:
	 * - HAL initialization, this also initializes the configured device drivers
	 *   and performs the board-specific initializations.
	 * - Kernel initialization, the main() function becomes a thread and the
	 *   RTOS is active.
	 */
	halInit();
	System::init();

	// Create a serial port for debugging; say hello. Common baud rates are: 
	// 9600, 14400, 19200, 38400, 57600, 115200, 230400, 460800, 921600
	rs232 dbg_port (460800, &SD2);
	dbg_port << clrscr << "PolyDAQ 2 compiled " << __DATE__ << ": debugging port" 
			 << endl;
	#ifdef CH_QUEUE_DEBUG
		daq_command_queue.set_serial_dev (&dbg_port);
		daq_data_queue.set_serial_dev (&dbg_port);
	#endif

	// This serial port is used to communicate with a PC for data collection
// 	rs232 ser_port (460800, &SD3);
// 	ser_port << clrscr << "PolyDAQ 2 compiled " << __DATE__ << endl;

	// Initializes the PWM driver 4, routes the TIM4 outputs to the board LEDs.
	pwmStart (&PWMD4, &pwmcfg);
	palSetPadMode (GPIOD, GPIOD_LED4, PAL_MODE_ALTERNATE(2));      /* Green.   */
	palSetPadMode (GPIOD, GPIOD_LED3, PAL_MODE_ALTERNATE(2));      /* Orange.  */
	palSetPadMode (GPIOD, GPIOD_LED5, PAL_MODE_ALTERNATE(2));      /* Red.     */
	palSetPadMode (GPIOD, GPIOD_LED6, PAL_MODE_ALTERNATE(2));      /* Blue.    */

	// Start the accelerometer-LED-blinky task
	the_accel_task.start ("Accel", NORMALPRIO + 10, &dbg_port);

	// Start up the A/D task
	the_data_acq_task.start ("Data Acq", NORMALPRIO + 20, &dbg_port);

	// Start up the task which saves data from the A/D task
	the_save_data_task.start ("Save Data", NORMALPRIO + 15, &dbg_port);

	// Start the task which communicates with the PC
	the_pc_task.start ("PC Int", NORMALPRIO + 5, &dbg_port);

	// Start the user interface task
	the_user_task.start ("Luser", NORMALPRIO + 5, &dbg_port);

	// Set up the DAC. The first number is a channel bitmask
	simple_DAC my_dac (0x03, &dbg_port);
	dacsample_t dac_out = 0;

	// Codes to make a silly spinner
	const uint8_t spin_codes[] = ".oO@* ";
	uint8_t* p_spin = (uint8_t*)spin_codes;
	uint16_t slow_count = 0;

	for (;;)
	{
		// Send a sawtooth wave to the DAC
		my_dac.set (1, dac_out);
		my_dac.set (2, (4095 - dac_out));
		if (dac_out++ > 4095)
		{
			dac_out = 0;
		}

		// Print a silly spinner
		if (slow_count++ >= 100)
		{
			slow_count = 0;
			if (*p_spin)
			{
				dbg_port << '\r' << (char)(*p_spin++) << ' ';
			}
			else
			{
				p_spin = (uint8_t*)spin_codes;
			}
		}

		// Control that timing
		chThdSleepMilliseconds (2);
	}

	return (0);
}

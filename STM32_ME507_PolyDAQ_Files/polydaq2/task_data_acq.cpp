//**************************************************************************************
/** \file task_data_acq.cpp
 *    This file contains the source for a demonstration task for an STM32F4 Discovery
 *    board. The task uses the STM32's A/D converter(s) to take some data. 
 *
 *  Revisions:
 *    \li ??-??-2013 Original file from the ChibiOS collection of stuff
 *    \li 06-14-2014 JRR Data acquisition task (thread) split off from \c main.c
 *
 *  License:
 *    The original file was released under the Apache License and has the following
 *    license statement:
 * 
 *        ChibiOS/RT - Copyright (C) 2006-2013 Giovanni Di Sirio
 *    
 *        Licensed under the Apache License, Version 2.0 (the "License");
 *        you may not use this file except in compliance with the License.
 *        You may obtain a copy of the License at
 *
 *            http://www.apache.org/licenses/LICENSE-2.0
 *
 *        Unless required by applicable law or agreed to in writing, software
 *        distributed under the License is distributed on an "AS IS" BASIS,
 *        WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *        See the License for the specific language governing permissions and
 *        limitations under the License.
 *
 *    Further modifications to this file are copyright 2014 by JR Ridgely and released
 *    under the same Apache license as the original file.  
 */
//**************************************************************************************

// #include "stdint.h"
#include "emstream.h"
#include "task_data_acq.h"


//-------------------------------------------------------------------------------------
/** @brief   This constructor creates a data acquisition task. 
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a data acquisition task. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 *  @param   p_polydaq2 A pointer to the PolyDAQ 2 driver used to interface with the
 *                      data acquisition board
 */

task_data_acq::task_data_acq (const char* p_name, unsigned portBASE_TYPE prio, 
							  size_t stacked, emstream* serpt, polydaq2* p_polydaq2)
	: TaskBase (p_name, prio, stacked, serpt)
{
	// Save the pointer to the PolyDAQ driver, which has its own A/D and D/A drivers
	my_poly = p_polydaq2;

	// By default, take samples once per second if in continuous sampling mode
	ms_per_sample = 100;

	// No A/D channel has been read yet
	last_data_channel = (char)0xFF;
}


//-------------------------------------------------------------------------------------
/** @brief   Run a command from the DAQ command queue.
 *  @details This method runs a command which has been found in the DAQ command queue.
 *           The command will have been sent from the UI task, @c task_user.
 *  @param   ch_in A character containing the command to be run
 */

void task_data_acq::run_daq_command (char ch_in)
{
	// If the command is from 0 through 15, take an A/D conversion, unless it's
	// using a channel which isn't safe to read
	if (ch_in == '2' || ch_in == '3')
	{
		*p_serial << "Not reading A/D ch. " << ch_in 
					<< "; it kills a serial port" << endl;
	}
	else if (ch_in >= '0' && ch_in <= '9')
	{
		last_data_channel = ch_in;
		*p_serial << my_poly->get_data (ch_in) << endl;
	}
	else if (ch_in >= 'A' && ch_in <= 'F')
	{
		last_data_channel = ch_in;
		*p_serial << my_poly->get_data (ch_in) << endl;
	}

	// The characters 'X', 'Y', and 'Z' read accelerations 
	else if (ch_in >= 'X' && ch_in <= 'Z')
	{
// 			my_poly->set_accel_range (MMA_RANGE_8g);  /////////////////////////////////////////////

		*p_serial << my_poly->get_accel (ch_in - 'X') << endl;
	}

	// If we get an 'L' or 'M' character, try to balance a bridge
	else if (ch_in == 'L')
	{
		my_poly->strain_auto_balance (1, 2047);
	}
	else if (ch_in == 'M')
	{
		my_poly->strain_auto_balance (2, 2047);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the data acquisition task code. 
 *  @details This method sets up the A/D converter, then waits for a signal to take 
 *           data. Then it takes data and fills up a buffer with that data. 
 */

void task_data_acq::run (void)
{
	char ch_in;                             // Stores a character from command queue
// 	logger_config* p_the_config;            // Data logger configuration object
// 	uint16_t ms_counter = 0;                // Counts ms per sample

	// This counter is used to run through the for (;;) loop at precise intervals
// 	TickType_t LastWakeTime = xTaskGetTickCount ();

	// In the main loop, wait for a command through the command queue; if a command
	// is received, do what the commander asked
	for (;;)
	{
// 		if (p_daq_UI_command_queue->not_empty ())     // If a command is in the queue,
// 		{                                             // call function to execute it
			ch_in = p_daq_UI_command_queue->get ();
			run_daq_command (ch_in);
// 		}

		runs++;                             // Track how many runs through the loop

// 		delay_from_for_ms (LastWakeTime, 1);    // Always a 1 ms delay for this task
	}
}

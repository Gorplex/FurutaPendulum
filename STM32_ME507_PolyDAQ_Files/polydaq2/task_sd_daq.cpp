//*************************************************************************************
/** \file task_sd_daq.cpp
 *    This file contains the source code for a task which uses a STM32's A/D converter
 *    to take some data that will be written to the SD card. 
 *
 *  Revisions:
 *    @li ??-??-2013 Original file from the ChibiOS collection of stuff
 *    @li 06-14-2014 JRR Data acquisition task (thread) split off from @c main.c
 *    @li 08-27-2014 JRR Converted to the latest PolyDAQ 2 version in FreeRTOS
 *    @li 03-20-2015 JRR Cloned and modded from the other DAQ task, the UI one
 *
 *  License:
 *    This file is copyright 2015 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 3. It intended for educational use only, but its use
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
//**************************************************************************************

#include "emstream.h"
#include "task_sd_daq.h"


//-------------------------------------------------------------------------------------
/** @brief   This constructor creates a SD card data acquisition task. 
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a data acquisition task. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 *  @param   p_polydaq2 A pointer to the PolyDAQ 2 driver used to interface with the
 *                      data acquisition board
 */

task_sd_daq::task_sd_daq (const char* p_name, unsigned portBASE_TYPE prio, 
							  size_t stacked, emstream* serpt, polydaq2* p_polydaq2)
	: TaskBase (p_name, prio, stacked, serpt)
{
	// Save the pointer to the PolyDAQ driver, which has its own A/D and D/A drivers
	my_poly = p_polydaq2;

	// By default, take samples pretty slowly if in continuous sampling mode
	ticks_per_sample = 1000;
}


//-------------------------------------------------------------------------------------
/** @brief   Take one row of data for the SD card.
 *  @details This method acquires one row of data for the SD card and places the text
 *           representing the data into the SD card's text queue. The SD card task will
 *           write the data to the card when it can.
 *  @param   p_log_config Pointer to the data logger configuration object to be used
 */

void task_sd_daq::acquire_sd_data (logger_config* p_log_config)
{
	logger_col_cfg* p_col_cfg;              // Stores each data column's configuration
	int16_t data;                           // Data which is read from the PolyDAQ

	// Turn the LED on -- the card is busy
	p_led_command_queue->put ('Y');

	// Write the current RTOS time down to the millisecond
	*p_sd_card_text_queue << get_tick_time ();

	// For each configured data item, get the data and write it
	p_col_cfg = p_log_config->get_first_channel ();
	while (p_col_cfg != NULL)
	{
		data = my_poly->get_data (p_col_cfg->command);
		*p_sd_card_text_queue << ',' 
			<< (float)data * p_col_cfg->slope + p_col_cfg->offset;
// 			<< '"' << p_col_cfg->command << '"';
		p_col_cfg = p_log_config->get_next_channel ();
	}
	*p_sd_card_text_queue << DAQ_EOL;

	// Turn the LED off -- the card is done for now
	p_led_command_queue->put ('N');
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the data acquisition task code. 
 *  @details This method sets up the A/D converter, then waits for a signal to take 
 *           data. Then it takes data and fills up a buffer with that data. 
 */

void task_sd_daq::run (void)
{
	logger_config* p_the_config;            // Data logger configuration object

	// This counter is used to run through the for (;;) loop at precise intervals
	TickType_t LastWakeTime = xTaskGetTickCount ();

	////////////////////// TESTING FOR MLX90614 PIR THERMOMETER ///////////////////////
	// Drop the SCL line for a couple of milliseconds to put thermo in SMBus (I2C) mode
	delay_ms (2);
	///////////////////////////////////////////////////////////////////////////////////

	// Do initializations which must be done with the RTOS already up and running
	my_poly->initialize ();

	// In the main loop, wait for a command through the command queue; if a command
	// is received, do what the commander asked
	for (;;)
	{
		// If the logger configuration isn't NULL, there's an SD card present
		if ((p_the_config = p_logger_config->get ()) != NULL)
		{
			acquire_sd_data (p_the_config);
			delay_from_for (LastWakeTime, p_the_config->get_ticks_per_sample ());
		}
		else
		{
			delay_from_for_ms (LastWakeTime, 100);  // No card?  Wait a bit longer
		}

		runs++;                                     // Count runs through the loop
	}
}

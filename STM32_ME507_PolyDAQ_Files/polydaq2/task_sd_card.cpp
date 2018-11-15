//**************************************************************************************
/** \file task_sd_card.cpp
 *  @brief   Source code for a task which saves data to an SD card.
 *  @details This file contains the source code for a task that saves data using the SD 
 *           card on a PolyDAQ board. Commands and data are sent to this task by other 
 *           tasks, and this task handles the saving of data as fast as the SD card can
 *           write it. 
 *
 *  Revisions:
 *    \li ??-??-2013 GDS Original file from the ChibiOS collection of stuff
 *    \li 06-14-2014 JRR Data acquisition task (thread) split off from \c main.c
 *    \li 07-02-2014 JRR User interface spun off from the other tasks
 *    \li 07-25-2014 JRR PC interface cloned from the user interface and modified
 *    \li 10-15-2014 JRR SD card task made from the PC interface task
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

#include "task_sd_card.h"


//-------------------------------------------------------------------------------------
/** @brief   Creates a SD card interface task object.
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a task object. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 */

task_sd_card::task_sd_card (const char* p_name, unsigned portBASE_TYPE prio, 
							size_t stacked, emstream* serpt)
	: TaskBase (p_name, prio, stacked, serpt)
{
	// The parent constructor does all the work
}


//-------------------------------------------------------------------------------------
/** @brief   The run method that saves data to an SD card. 
 *  @details This run method waits for an SD card to be inserted in the socket. When a
 *           card is found in the socket, it opens a data file using an automatically
 *           generated, sequenced name. Then data which arrives in the data queue is
 *           saved to the card as soon as the data arrives. 
 * 
 *  @dot
 *    digraph state_diagram_task_sd_card_run 
 *    {
 *      node [shape=ellipse, fontname=Sans, fontsize=12];
 *      s0 [ label="0: No Card" ];
 *      s1 [ label="1: Mount Card" ];
 *      s2 [ label="2: Read Config" ];
 *      s3 [ label="3: Open Data File" ];
 *      s4 [ label="4: Saving Data" ];
 *      edge [fontname="Sans", fontsize=11];
 *      s0 -> s1 [ label="Card found"];
 *      s1 -> s0 [ label="No card || Can't open file" ];
 *      s1 -> s2 [ label="File open" ];
 *      s2 -> s0 [ label="No card" ];
 *      s2 -> s3 [ label="Config file read || No config file" ];
 *      s3 -> s0 [ label="No card || Data file error" ];
 *      s3 -> s4 [ label="Data file opened" ];
 *      s4 -> s0 [ label="No card" ];
 *    }
 *  @enddot
 */

void task_sd_card::run (void)
{
// 	TickType_t ticks_last_save = 0;         // Saves last time data saved to SD card
// 	TickType_t ticks_now;                   // Temporary storage for current time

	// Create an SD card object
	sd_card* p_sdcard = new sd_card (p_serial);

	// We also need a configuration file reading object
	logger_config* p_logconf = new logger_config (p_sdcard, p_serial);

	// The logging file will need a number to make automatic names work
	uint16_t fnum;

	// Run the finite state machine in this infinite loop
	for (;;)
	{
		switch (state)
		{
			//------------------- State 0: Wait for a card ----------------------------
			case (0):
				// Check if a card can be detected
				if (SD_Detect () == SD_PRESENT)
				{
					SD_TSK_DBG (PMS ("SD card detected") << endl);
					transition_to (1);
				}
				else
				{
					// Put the LED in heartbeat mode
					p_led_command_queue->put ('H');

					// Wait a second before looking again
					delay_ms (1000);
				}
				break;

			//------------------- State 1: Mount and open -----------------------------
			case (1):
				// If there's no card detected, go back to state 0
				if (SD_Detect () != SD_PRESENT)
				{
					SD_TSK_DBG ("SD card removed" << endl);
					transition_to (0);
				}
				else // There's a card; mount it, open a file, and go to state 2
				{
					SD_TSK_DBG (PMS ("Mounting..."));
					if (p_sdcard->mount () == FR_OK)
					{
						transition_to (2);
					}
					else  // Oops, problems...wait 5 seconds and try again
					{
						SD_TSK_DBG (PMS ("Oh Noes! SD card mount problem") << endl);
						transition_to (0);
						delay_ms (5000);
					}
					SD_TSK_DBG (endl);
				}
				break;

			//------------------- State 2: Read configuration file --------------------
			case (2):
				// There's a card; read the configuration file and go to state 3
				if (SD_Detect () == SD_PRESENT)
				{
					// Turn the LED on -- the card is busy
					p_led_command_queue->put ('M');
					sd_card_LED = true;

					// Try to open the configuration file
					SD_TSK_DBG (PMS ("Reading configuration..."));
					p_logconf->read ("polydaq2.cfg");
					SD_TSK_DBG (PMS ("done") << endl);

					// Done reading the SD card for a little while
					sd_card_LED = false;

					// If the configuration file's data is valid, use it
					if (p_logconf->is_valid ())
					{
						p_logger_config->put (p_logconf);
						p_ticks_per_sd_data->put (p_logconf->get_ticks_per_sample ());
						SD_TSK_DBG (*p_logconf << endl);
						transition_to (3);
					}
					else      // If the configuration is bad, don't save any data
					{
						SD_TSK_DBG (PMS ("SD card removed") << endl);
						p_sdcard->unmount ();
						p_logger_config->put (NULL);
						p_led_command_queue->put ('H');
						transition_to (9);
					}
				}
				else // If there's no card detected, go back to state 0
				{
					SD_TSK_DBG ("No can has SD card." << endl);
					p_logger_config->put (NULL);
					p_led_command_queue->put ('H');
					transition_to (0);
				}
				break;

			//------------------- State 3: Open data logging file ---------------------
			case (3):
				// If there's no card detected, go back to state 0
				if (SD_Detect () != SD_PRESENT)
				{
					SD_TSK_DBG ("No can has SD card." << endl);
					p_logger_config->put (NULL);
					p_led_command_queue->put ('H');
					transition_to (0);
				}
				else // There's a card; open the logging file
				{
					// Try to open the data logging file
					fnum = p_sdcard->open_new_data_file (DATA_F_NAME, DATA_F_EXT);
					if (fnum == 0xFFFF)
					{
						SD_TSK_DBG (PMS ("Oh Noes! Can't open data file") << endl);
						p_sdcard->unmount ();
						transition_to (0);
						delay_ms (5000);
					}
					else
					{
						*p_sdcard << PMS ("PolyDAQ 2 data file, samples at ") 
								  << (p_logger_config->get ())->get_ms_per_sample ()
								  << PMS (" ms") << endl;
// 						ticks_last_save = get_tick_count ();

						write_header (p_sdcard);
						transition_to (4);
					}
				}
				break;

			//------------------- State 4: Wait for and save data ---------------------
			case (4):
				// Watch for data which needs to be written
				while (p_sd_card_text_queue->check_for_char ())
				{
					p_sdcard->putchar (p_sd_card_text_queue->getchar ());
				}

// 				// Check if it's time to flush the text buffer to the SD card
// 				ticks_now = get_tick_count ();
// 				if ((ticks_now - ticks_last_save) > SD_TICKS_PER_SYNC)
// 				{
// 					ticks_last_save = ticks_now;
// 					*p_sdcard << send_now;
// 				}

				// Check for the card, then let other tasks run
				if (SD_Detect () != SD_PRESENT)
				{
					SD_TSK_DBG (PMS ("SD card removed") << endl);
					p_sdcard->unmount ();
					p_logger_config->put (NULL);
					p_led_command_queue->put ('H');
					transition_to (0);
				}
				delay_ms (1);   ///////////////////////////////////////////////////////////
// 				yield ();
				break;

			//------------------- State 9: Bad configuration file ---------------------
			case (9):
				// Turn the LED off and wait until the card has been removed
				// Check for the card, then let other tasks run
				if (SD_Detect () != SD_PRESENT)
				{
					transition_to (0);
				}
				break;

			//------------------- Other States: Should never happen -------------------
			default:
				transition_to (0);
				break;
		}; // switch

// 		ch_in = p_serial->getchar ();       // This should block for empty buffer

		runs++;                             // Track how many runs through the loop
	} // for (;;)
}


//-------------------------------------------------------------------------------------
/** @brief   Write a header showing data items to be logged to a serial device. 
 *  @details This method writes a header line which shows the name of each of the
 *           items to be logged. Usually the logging is done to an SD card, and this
 *           line is a header for the logged data. The logger configuration object,
 *           which is pointed to by the shared variable @c p_logger_config, is used
 *           to get the data headers. 
 *  @param   p_ser_dev A pointer to the serial device to which to write the header
 */

void task_sd_card::write_header (emstream* p_ser_dev)
{
	logger_config* p_the_config;            // Data logger configuration object
	logger_col_cfg* p_col_cfg;              // Stores each data column's configuration

	// If the logger configuration isn't NULL, there's an SD card present
	if ((p_the_config = p_logger_config->get ()) != NULL)
	{
		// Write the current RTOS time down to the millisecond
		*p_ser_dev << PMS ("Time");

		// For each configured data item, get the header and write it
		p_col_cfg = p_the_config->get_first_channel ();
		while (p_col_cfg != NULL)
		{
			*p_ser_dev << ',' << '"' << p_col_cfg->p_label << '"';
			p_col_cfg = p_the_config->get_next_channel ();
		}
		*p_ser_dev << '\n';
	}
}


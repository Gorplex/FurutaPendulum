//**************************************************************************************
/** \file task_pc.cpp
 *    This file contains source code for a PC interface task for a PolyDAQ board. This 
 *    task communicates with a Python program running on the PC to create a data
 *    acquisition system. 
 *
 *  Revisions:
 *    \li ??-??-2013 GDS Original file from the ChibiOS collection of stuff
 *    \li 06-14-2014 JRR Data acquisition task (thread) split off from \c main.c
 *    \li 07-02-2014 JRR User interface spun off from the other tasks
 *    \li 07-25-2014 JRR PC interface cloned from the user interface and modified
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

#include "task_pc.h"


//-------------------------------------------------------------------------------------
/** @brief   Creates a PC interface task object.
 *  @details This constructor calls its parent class's constructor to actually do the 
 *           work of creating a PC interface task object. 
 *  @param   p_name A name for this task
 *  @param   prio The priority at which this task will run (should be low)
 *  @param   stacked The stack space to be used by the task (not much)
 *  @param   serpt A pointer to a serial device on which debugging messages are shown
 */

task_pc::task_pc (const char* p_name, unsigned portBASE_TYPE prio, size_t stacked, 
				  emstream* serpt)
	: TaskBase (p_name, prio, stacked, serpt)
{
	// The parent constructor does all the work
}


//-------------------------------------------------------------------------------------
/**
 * 
 *  @section cmd_codes "Command Codes"
 *           The following structure is used for commands sent from the PC to the 
 *           microcontroller:
 *           | 1 byte   | 0-4 bytes | 1 byte   |
 *           |----------|-----------|----------|
 *           | command  |   data    | checksum |
 *           Commands:
 *           \li 0 - F : No data : Send A/D reading from one channel, 0 through 15
 *           \li More to come...
 * 
 *  @section data_packets "Data Packets"
 *           The following structure is used for data sent by the microcontroller to
 *           the PC:
 *           \li Packet header: Begin with a byte which is 0xAA
 *           \li Time: 32-bit number containing milliseconds since program startup
 *           \li Time per point: 32-bit number containing microseconds per reading set
 *           \li Active channels: 16-bit bitmask, active channels being 1
 *           \li A/D data: 16-bit number; 4 MSB's 0000; other 12 bits have the data
 *           \li CRC16 for packet
 *           \li Others...?
 */


//-------------------------------------------------------------------------------------
/** @brief   The run method that runs the thread which talks to a PC. 
 *  @details This method sets up the A/D converter, then waits for a signal to take 
 *           data. Then it takes data and fills up a buffer with that data. 
 */

void task_pc::run (void)
{
	char ch_in = '\0';                      // A character sent to us by the PC
// 	uint8_t ad_channel;                     // Number of A/D channel to be sampled

// 	adc_driver* p_adc = new adc_driver (p_serial);             // Create an A/D driver
// 	p_adc->on ();

	for (;;)
	{
		if (p_serial->check_for_char ())
		{
			ch_in = p_serial->getchar ();       // This should block for empty buffer
			*p_main_text_queue << '[' << ch_in << ']';

			// Return a version string if requested
			if (ch_in == 'v')
			{
				*p_serial << "PolyDAQ 2 compiled " << __DATE__ << endl;
			}

	// 		// Characters from 0 through 9 and A through F mean read the A/D right now
	// 		else if ((ch_in >= '0' && ch_in <= '9') || (ch_in >= 'A' && ch_in <= 'F'))
	// 		{
	// 			ad_channel = (ch_in <= '9') ? ch_in - '0' : ch_in - ('A' - 10);
	// 			*p_serial << "Put: " << (uint8_t)ad_channel << endl;  ///////////////////////////
	// 			p_daq_command_queue->put (ad_channel);
	// 			*p_serial << p_adc->read_once (ad_channel) << endl;
	// 		}

			// If any other character is entered, ask "What's That Function?"
			else
			{
				*p_serial << ch_in << ": WTF?" << endl;
			}
		} // Done checking for characters

		// Every now and then, send a ping
		if (runs % 1000 == 0)
		{
			*p_serial << "PolyDAQ2" << endl;
// 			*p_main_text_queue << "PD2";
		}

		runs++;                             // Track how many runs through the loop

		delay (10);
	} // for (;;)
}

//*************************************************************************************
/** @file    main.cpp
 *  @brief   The main file for the PolyDAQ 2 firmware.
 *  @details This is the main file for the PolyDAQ2 firmware, FreeRTOS version. It
 *           contains the declarations to globally accessible pointers to the shared
 *           data items and queues which are used to exchange information between
 *           tasks; code which creates the pointers to the shares and queues; and the
 *           function @c main(), which is the first function to run when the program 
 *           is started up. Inside @c main(), code does initial setup, creates the 
 *           task objects which are used in the program, and starts the RTOS scheduler.
 *
 *  Revisions:
 *    \li 08-26-2014 JRR Original file, based on earlier PolyDAQ2 project files
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

#include <math.h>                               ////////////////////////////////////////////
// extern "C" 
// {
// 	int __errno;                           // WTF
// }

#include "FreeRTOS.h"
#include "task.h"

#include "rs232.h"                          // Header for serial port class
#include "taskbase.h"                       // The base class for all tasks
#include "taskqueue.h"                      // Queues transmit data between tasks
#include "textqueue.h"                      // Queues that only carry text

#include "appconfig.h"                      // Application configuration header
#include "task_leds.h"
#include "task_user.h"                      // Headers for each of the tasks
// #include "task_pc.h"
#include "task_data_acq.h"
#include "task_sd_card.h"
#include "task_sd_daq.h"


//-------------------------------------------------------------------------------------
// The pointers in the following section are for shares and queues that transfer data,
// commands, and other information between tasks. 

/** @brief   Pointer to a queue for text to be displayed by the UI.
 *  @details This queue can be used by all the tasks to print messages without having
 *           to wait for the comparatively slow serial port.
 */
TextQueue* p_main_text_queue;

/** @brief   Queue for commands given to the data acquisition task.
 *  @details This pointer points to a queue that transfers commands from the PC 
 *           interface task and/or user interface task to the data acquisition task.
 */
TaskQueue<char>* p_daq_UI_command_queue;

/** @brief   Queue for text to be written to the SD card.
 *  @details This pointer points to a queue which carries text that will be written 
 *           to an SD card if a card is in the socket. Of no card is in the socket,
 *           some data will just be lost. 
 */
TextQueue* p_sd_card_text_queue;

/** @brief   Pointer to the SD card data logger configuration if one exists.
 *  @details This shared pointer contains the memory address of the SD card data 
 *           logger configuration if one exists.  If there is no SD card in the slot 
 *           with a valid logger configuration file, this pointer is NULL to indicate
 *           that no SD card data logging is to be done. 
 */
TaskShare<logger_config*>* p_logger_config;

/** @brief   Pointer to a share holding milliseconds per SD card data row.
 *  @details This item holds the number of milliseconds between data rows collected
 *           for storage on the SD card. It is set by the logger configuration reader
 *           in the SD card task and used by the data acquisition task.
 */
TaskShare<uint16_t>* p_ticks_per_sd_data;

/** @brief   Pointer to a queue for commands to the indicator LED control task.
 *  @details The queue pointed to by this pointer holds commands for the task which
 *           controls the orange LED on the PolyDAQ board. A command byte consists of
 *           a 3-bit command in the most significant bits and a 5-bit number in the
 *           least significant bits. Commands are: 
 * 
 *           Number | Command
 *           :----: | :------
 *            000   | Turn off and stay off until another command arrives
 *            001   | Turn on and stay on until another command arrives
 *            010   | Go to "heartbeat" mode, smooth on-and-off blinking
 *            011   | Blink the number of times in the 5-bit number
 *            1XX   | Other commands are ignored for now
 */
TaskQueue<uint8_t>* p_led_command_queue;

/** @brief   Flag which causes LED task to turn the SD card activity LED on and off.
 *  @details This flag allows the SD card driver to control the SD card activity LED.
 *           Although scoped globally, it's not a task share because access to a bool
 *           is atomic, so the data can't be corrupted by a context switch. 
 */
bool sd_card_LED;


//-------------------------------------------------------------------------------------
/** @brief   Test function to set RTS and CTS of RN-42 bluetooth module.
 */

void set_RTS_CTS (void)
{
	// Create the initialization structure
	GPIO_InitTypeDef GP_Init_Struct;
	GPIO_StructInit (&GP_Init_Struct);

	// Set up pin used for transmission for alternate function, push-pull
	GP_Init_Struct.GPIO_Pin = (1 << 13);
	GP_Init_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	GP_Init_Struct.GPIO_Mode = GPIO_Mode_IN;
	GP_Init_Struct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init (GPIOB, &GP_Init_Struct);

	// Receiver pin is set up as an alternate function input
	GP_Init_Struct.GPIO_Pin = (1 << 14);
	GP_Init_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	GP_Init_Struct.GPIO_Mode = GPIO_Mode_IN;
	GP_Init_Struct.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init (GPIOB, &GP_Init_Struct);

	// Connect output and input pins to alternate function of this U(S)ART
// 	GPIO_PinAFConfig (p_port, tx_pin, GPIO_AF_USART2);
// 	GPIO_PinAFConfig (p_port, rx_pin, GPIO_AF_USART2);
}


//-------------------------------------------------------------------------------------
/** @brief   This function runs when the application is started up.
 *  @details The @c main() function instantiates shared variables and queues, sets up 
 *           serial ports and other drivers (if needed), creates the task objects 
 *           which will run @a ad @a infinitum, and starts the RTOS scheduler. 
 *  @return  Although the scheduler never exits and this function never returns, it's
 *           C/C++ tradition for @c main() to return an integer, so we (don't) return
 *           zero; if the return statement is missing the compiler issues a warning.
 */

int main (void)
{
	// Create the serial port which will be used for debugging
	RS232* usart_2 = new RS232 (USART2, 115200);
	*usart_2 << endl << clrscr << "FreeRTOS Test Program on STM32" << endl;

// 	// Create a serial port which will be used to talk to the PC. It can be a different
// 	// port than the one used for debugging the system
// 	RS232* usart_3 = new RS232 (USART3, 115200);
// 	*usart_3 << endl << clrscr << "FreeRTOS Test Program on STM32" << endl;
// 
// 	// See just above main()
// 	set_RTS_CTS ();

	//------------------------------- Queues and Shares -------------------------------

	/*  This queue prints out debugging and other messages.
	 */
	p_main_text_queue = new TextQueue (400, "DBG Text", 0, 10);

	/*  This queue carries text to the task that writes data to the SD card. The 
	 *  parameters are buffer size, name, and wait time if queue is full
	 */
	p_sd_card_text_queue = new TextQueue (1000, "SD Text", 0);

	/*  This queue carries commands from the user interface task to the data 
	 *  acquisition task.
	 */
	p_daq_UI_command_queue = new TaskQueue<char> (10, "DAQ Cmds");

	/*  Pointer to a share which points to the SD card data logger configuration if it
	 *  exists, or otherwise is NULL to indicate no SD card data logging is to be done
	 */
	p_logger_config = new TaskShare<logger_config*> ("Log Conf");
	p_logger_config->put (NULL);

	/*  Create a queue for commands to the indicator LED control task.
	 */
	p_led_command_queue = new TaskQueue<uint8_t> (10, "LED Cmds");

	/*  Pointer to a share which contains the number of milliseconds per SD card data row
	 */
	p_ticks_per_sd_data = new TaskShare<uint16_t> ("SD Time");

	//--------------------------------- Device Drivers --------------------------------

	// Create a PolyDAQ driver, which has its own A/D and D/A drivers
	polydaq2* my_poly_driver = new polydaq2 (usart_2);
	*usart_2 << "PolyDAQ2" << endl;

	//*********************************************************************************

// 	float moose = 0.5;
// 	*usart_2 << "Sqrt of " << moose << " is ";
// 	moose = sqrt (moose);
// 	*usart_2 << moose << endl;
// 
// 	moose = -0.0;
// 	*usart_2 << "Sqrt of " << moose << " is ";
// 	moose = sqrt (moose);
// 	*usart_2 << moose << endl;

	//*********************************************************************************
	
	//------------------------------------- Tasks -------------------------------------

	// Create a task that blinks an LED
	new task_leds ("LED's", 1, 240, usart_2, my_poly_driver);

	// And a task that checks for characters sent through the user interface
	new task_user ("Luser", 1, 620, usart_2, my_poly_driver);

	// This task acquires data to be sent out a serial port
	new task_data_acq ("Data Acq", 1, 400, usart_2, my_poly_driver);

	// This task acquires data to be saved to an SD card
	new task_sd_daq ("SD DAQ", 3, 400, usart_2, my_poly_driver);

	// This task is the one which actually writes to an SD card
	new task_sd_card ("SD Card", 2, 800, usart_2);

// 	// This task prints characters from the main print queue
// 	new task_print ("PrintQ", 1, 240, usart_2);

// 	// This task interfaces through the Bluetooth interface (hopefully)
// 	new task_pc ("PC_Com", 1, 320, usart_3);

	// Start the FreeRTOS scheduler
	vTaskStartScheduler ();

	return (0);
}

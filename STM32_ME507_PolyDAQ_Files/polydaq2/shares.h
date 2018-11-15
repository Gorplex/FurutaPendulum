//*************************************************************************************
/** @file    shares.h
 *  @brief   Declarations for inter-task data communication items (shares and queues).
 *  @details This file contains @c extern declarations for queues and other inter-task 
 *           data communication objects used in a ME405/507/PolyDAQ FreeRTOS project. 
 *
 *  Revisions:
 *    \li 09-30-2012 JRR Original file was a one-file demonstration with two tasks
 *    \li 10-05-2012 JRR Split into multiple files, one for each task plus a main one
 *    \li 10-29-2012 JRR Reorganized with global queue and shared data references
 *    \li 06-18-2014 JRR Modified into ChibiOS/STM32F4 test version
 *
 *  License:
 *    This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _SHARES_H_
#define _SHARES_H_

#include "emstream.h"                       // Base class for byte stream classes
#include "adc_driver.h"                     // Has ADC queue size
#include "taskshare.h"                      // Include definitions of shared variable
#include "taskqueue.h"                      // and queue classes
#include "textqueue.h"                      // Queues that only carry text
#include "logger_config.h"                  // Data logger configuration parser


//-------------------------------------------------------------------------------------
// Externs:  In this section, we declare variables and functions that are used in all
// (or at least two) of the files in the project. Each of these items will also be 
// declared exactly once, without the keyword 'extern', in one .cpp file as well as 
// being declared extern here. 

/*  Pointer to a queue for text to be displayed by the UI
 */
extern TextQueue* p_main_text_queue;

/*  Pointer to a queue that transfers commands from the PC interface task to the
 *  data acquisition task
 */
extern TaskQueue<char>* p_daq_UI_command_queue;

/*  Pointer to a share which points to the SD card data logger configuration if one
 *  exists, or otherwise is NULL to indicate no SD card data logging is to be done
 */
extern TaskShare<logger_config*>* p_logger_config;

/*  Pointer to a share which contains the number of milliseconds per SD card data row
 */
extern TaskShare<uint16_t>* p_ticks_per_sd_data;

/*  Pointer to a queue which carries text that will be written to an SD card if a card
 *  is in the socket
 */
extern TextQueue* p_sd_card_text_queue;

/*  Pointer to a queue which carries commands to an indicator LED control task
 */
extern TaskQueue<uint8_t>* p_led_command_queue;

/*  Flag which causes LED task to turn the SD card activity LED on and off. It's not a
 *  task share because reads and writes to a bool are atomic.
 */
extern bool sd_card_LED;


// /*  Pointer to the A/D converter driver which is used in the program
//  */
// extern adc_driver* p_a2d;
// 
// /*  Pointer to the MMA8452Q driver for the PolyDAQ 2's onboard accelerometer
//  */
// extern mma8452q p_onboard_accel;


#endif // _SHARES_H_

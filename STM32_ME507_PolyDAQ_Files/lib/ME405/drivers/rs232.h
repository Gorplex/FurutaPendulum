//*************************************************************************************
/** @file    rs232.h
 *  @brief   Headers for a class that drives an RS-232 style asynchronous serial port.
 *  @details This file contains a class which allows the use of a serial port on a 
 *           microcontroller. The port is usually used in "text mode"; that is, the 
 *           information which is sent and received is expected to be plain ASCII text,
 *           and the set of overloaded left-shift operators @c << in @c emstream.* can
 *           be used to conveniently send all sorts of data to the serial port in a 
 *           manner similar to iostreams such as @c cout in regular C++. There is also
 *           a "raw" mode which can be activated to send items as streams of binary 
 *           bytes. 
 *
 *  Revised:
 *    \li 04-03-2006 JRR For updated version of compiler
 *    \li 06-10-2006 JRR Ported from C++ to C for use with some C-only projects; also
 *        serial_avr.h now has defines for compatibility among lots of AVR variants
 *    \li 08-11-2006 JRR Some bug fixes
 *    \li 03-02-2007 JRR Ported back to C++. I've had it with the limitations of C.
 *    \li 04-16-2007 JO  Added write (unsigned long)
 *    \li 07-19-2007 JRR Changed some character return values to bool, added m324p
 *    \li 01-12-2008 JRR Added code for the ATmega128 using USART number 1 only
 *    \li 02-14-2008 JRR Split between emstream and rs232 files
 *    \li 05-31-2008 JRR Changed baud calculations to use CPU_FREQ_MHz from Makefile
 *    \li 06-01-2008 JRR Added getch_tout() because it's needed by 9Xstream modems
 *    \li 07-05-2008 JRR Changed from 1 to 2 stop bits to placate finicky receivers
 *    \li 12-22-2008 JRR Split off stuff in base232.h for efficiency
 *    \li 06-30-2009 JRR Received data interrupt and buffer added
 *    \li 12-21-2013 JRR Reworked so that it works under ChibiOS
 *    \li 07-08-2014 JRR Made to work with STM32F4; port numbers now 1, 2, ...
 *    \li 07-27-2014 JRR Cleaned up and streamlined with extra error checking
 *    \li 08-08-2014 JRR New FreeRTOS version with STM32 code
 *    \li 10-11-2014 JRR Fixed a bug in USART3 for the PolyDAQ2 (I/O port for pins)
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
//*************************************************************************************

/// This define prevents this .h file from being included more than once in a .cpp file
#ifndef _RS232_H_
#define _RS232_H_

#include <stdlib.h>                         // Standard C/C++ library stuff

#if defined __AVR
	#include <avr/io.h>                     // If using AVR, talk to ports directly
#elif defined __ARMEL__
	#include "stm32f4xx.h"                  // If using STM32, include its driver .h's
	#include "stm32f4xx_gpio.h"
	#include "stm32f4xx_usart.h"
	#include "misc.h"                       // Needed for NVIC_InitTypeDef, etc. 
#endif

#include "FreeRTOS.h"                       // Header for FreeRTOS
#include "queue.h"                          // FreeRTOS queues header

#include "emstream.h"                       // Pull in the base class header file

// This file, in user's directory, tells which serial ports are to be activated
#include "appconfig.h"


/** @brief   Turn on or off the use of a transmitter queue for serial devices.
 *  @details This flag is used to activate the use of a transmitter queue with 
 *           transmitter empty interrupts. If it is zero, no queue will be used; the
 *           serial port driver will wait until the transmitter's shift register 
 *           preload register is empty before sending a character. If it is one, the
 *           driver will keep a buffer for transmitted characters, allowing tasks to
 *           send short bursts of characters more quickly without waiting for the
 *           serial line to send them.
 */
#define UART_USE_TX_BUFFERS   0


/** @brief   Factor by which to adjust baud rate for bug in program.
 *  @details For some unknown reason, the baud rate divisor being calculated in the
 *           STM32 library is off by a factor of 3 as of this writing. The value given
 *           here is used as a shameless hack to get the UART working.
 *           NOTE: The problem is probably due to not acccounting for one of the 
 *           peripheral clock dividers. 
 */
const uint8_t UART_FIX_BAUD_HACK = 3;

/** @brief   Number of ticks to wait if @c getchar() doesn't have a character ready.
 *  @details If a serial port's @c getchar() method is called and there isn't a 
 *           character available in the queue, the calling task will be blocked for 
 *           this number of ticks before the function call times out and '\0' is
 *           returned. The usual value is @c portMAX_DELAY, which means that 
 *           @c RS232::getchar() will block indefinitely until a character arrives.
 */
const TickType_t UART_GETCHAR_DELAY = portMAX_DELAY;

/** @brief   The size of the UART receiver buffers.
 *  @details Each UART uses a receiver buffer to hold characters in case characters
 *           arrive in the serial port more quickly than the task which reads the
 *           port can deal with them. This value sets the size of each receiver 
 *           buffer; all UARTs use the same size buffers. A larger buffer may allow 
 *           faster communication rates to be used without loss of data, but it eats
 *           more memory than a smaller one. Valid values are 4 to 255. 
 */
const uint8_t UART_RX_BUF_SZ = 32;

/** @brief   The size of the UART transmitter buffers.
 *  @details Each UART uses a transmitter buffer to hold characters in case the 
 *           sending task needs to write characters more quickly than the serial port
 *           can transmit them. This value sets the size of each transmitter buffer; 
 *           all UARTs use the same size buffers. A larger buffer may allow faster 
 *           communication rates to be used without forcing the transmitting task to
 *           wait for the serial port, but it eats more memory than a smaller one. 
 *           Valid values are 4 to 255. 
 */
const uint8_t UART_TX_BUF_SZ = 16;


//=====================================================================================
/** @brief   Class which controls a U(S)ART connected to a serial port.
 *  @details This class controls a U(S)ART, a Universal (Synchronous or) Asynchronous 
 *           Receiver Transmitter, a common asynchronous serial interface used in 
 *           microcontrollers, and allows the use of an overloaded \c << operator to 
 *           print things in \c cout style. It talks to old-style RS232 serial ports 
 *           (through a voltage converter chip such as a MAX232) or through a USB to 
 *           serial converter such as a FT232RL chip. The U(S)ART can also be used to 
 *           communicate with some sensors, wireless modems, or other microcontrollers.
 * 
 *  @section Usage
 *      In order to use the RS232 class on an STM32 microcontroller, one first needs 
 *      to enable the correct U(S)ART base driver in the file @c appconfig.h, which is
 *      generally kept in the user's project directory (not a library directory). For
 *      example, to enable U(S)ART 1, the following must be present in @c appconfig.h:
 *      @code
 *      #define USART_1_ENABLE      1
 *      @endcode
 *      Next, one must create an object of the RS232 class, using one of the U(S)ART
 *      definitions @c USART1, @c USART2, @c USART3, @c UART4, @c UART5, or @c USART6
 *      which are predefined in the library. Note that @c UART4 and @c UART5 aren't
 *      typos; those two ports lack clock pins so the "S" doesn't apply to them. A
 *      code example which sets up port #2 to communicate at 115200 baud: 
 *      @code
 *      RS232* usart_2 = new RS232 (USART2, 115200);
 *      @endcode
 *      Having created a U(S)ART driver, one can write to it using the full set of
 *      @c emstream shift operator conversions. For example, 
 *      @code
 *      *usart_2 << "The moose has traveled " << distance << " miles" << endl;
 *      @endcode
 *      Note that the variable @c usart_2 is a @e pointer, so it must be dereferenced,
 *      and that's why there is an asterisk before it. 
 *
 *      \section rs232pins Pin Connections
 *      Some pin choices from chips that have been used are in this section for 
 *      convenience. All pinouts must be checked against manufacturers' data sheets;
 *      YMMV; @e etc. 
 * 
 *      \b STM32F4xx:
 *      \li USART1_TX = PA9, USART1_RX = PA10, 
 *          USART1_CTS = PA11, USART1_RTS = PA12. @b UNTESTED.
 *      \li USART2_TX = PA2, USART2_RX = PA3, 
 *          USART2_CTS = PA0, USART2_RTS = PA1. Tested with no flow control.
 *      \li USART3_TX = PB10/PC10/PD8, USART3_RX = PB11/PC11/PD9, 
 *          USART3_CTS = PB13/PD11, USART3_RTS = PB14/PD12. Tested on PD8
 *          and PD9 with no flow control. 
 *      \li UART4_TX = PA0/PC10, UART4_RX = PA1/PC11. @b UNTESTED on PA0 and PA1.
 *      \li UART5_TX = PC12, UART5_RX = PD2. @b UNTESTED.
 *      \li USART6_TX = PC6/PG14, USART6_RX = PC7/PG9,
 *          USART6_CTS = PG13/PG15, USART6_RTS = PG8/PG12. @b UNTESTED on PC6 and PC7.
 * 
 *      \b STM32 \b gotcha: USART1 and USART6 use the PCLK2 clock; the others use
 *      the PCLK1 clock. 
 *
 */

class RS232 : public emstream
{
protected:
	#if defined __AVR

	#elif defined __ARMEL__
		/** @brief   A pointer to the U(S)ART data structure in the STM32 library.
		 *  @details This pointer is used to access the U(S)ART data structure that is
		 *           used to control the U(S)ART with the STM32 standard peripheral 
		 *           library code. 
		 */
		USART_TypeDef* p_USART;

		// This function sets up the interrupts used by a U(S)ART
		void init_interrupts (IRQn_Type which_int);

		// A function to configure the I/O pins used by the U(S)ART
		void init_pins (GPIO_TypeDef* p_port, uint16_t tx_pin, uint16_t rx_pin);
	#endif

	/** @brief   A queue that holds incoming characters from the receiver.
	 *  @details This queue holds characters that are received by the serial port.
	 *           The receipt of each character causes an interrupt, and the interrupt
	 *           service routine puts the character in the queue.
	 */
	QueueHandle_t receiver_queue;

	#if UART_USE_TX_BUFFERS == 1
		/** @brief   A queue that holds characters to be sent by the transmitter.
		 *  @details This queue holds characters to be transmitted by the serial port.
		 */
		QueueHandle_t transmitter_queue;

		/** @brief   Mask for a bit indicating if the transmitter empty interrupt is on.
		 *  @details This flag is used to indicate that the transmitter empty interrupt
		 *           will need to be activated within @c putchar() when a character is
		 *           queued for transmission. 
		 */
		uint8_t tx_irq_on_mask;
	#endif

	/** @brief   Baud rate currently being used by this UART.
	 */
	uint32_t baud_rate;

public:
	// The constructor sets up and initializes the U(S)ART driver object
	RS232 (USART_TypeDef* p_usart, uint32_t a_baud_rate);

	// Method which initializes the serial port, activates its interrupts, and so on
	virtual void start (void);

	// Method which turns off a serial port, thus saving power
	virtual void stop (void);

	// Send one character through the serial port
	void putchar (char chout);

	// Get a character from the serial port
	char getchar (void);

	// Check if a character is available in the serial port
	bool check_for_char (void);

	// Return the next character in the receiver queue without removing it
	char peek (void);
};


#endif  // _RS232_H_

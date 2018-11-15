//*************************************************************************************
/** @file    rs232.cpp
 *  @brief   Source for a class that drives an RS-232 style asynchronous serial port.
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

#include "rs232.h"                          // Header for this class


/** @brief   Flags indicating if the transmitter empty interrupt is on.
 *  @details This set of flags, whose scope is global to this @c .cpp file so the 
 *           interrupt service routines can use it, indicates that the transmitter 
 *           empty interrupt for each U(S)ART will need to be activated within 
 *           @c putchar() when a character is queued for transmission. 
 */
uint8_t tx_irq_on;

#if USART_0_ENABLE == 1
	/** @brief   Driver object for USART (serial port) number 0. */
	RS232 usart_0;

	#ifdef __ARMEL__
		#error UART 0 is undefined for STM32, which supports UARTs 1 through 6
	#endif
#endif

#if USART_1_ENABLE == 1
	/** @brief   A queue that holds incoming characters from the receiver.
	 *  @details This queue holds characters that are received by the serial port.
	 *           The receipt of each character causes an interrupt, and the 
	 *           interrupt service routine puts the character in the queue.
	 */
	QueueHandle_t receiver_queue_1;

	#if UART_USE_TX_BUFFERS == 1
		/** @brief   A queue that holds characters to be sent by the transmitter.
		 *  @details This queue holds characters to be transmitted by the serial port.
		 */
		QueueHandle_t transmitter_queue_1;
	#endif

	#if defined __AVR

	#elif defined __ARMEL__
		//-----------------------------------------------------------------------------
		/** @brief   Interrupt handler for USART 1.
		 *  @details This interrupt handler runs when a character has been received by 
		 *           the UART @b or when the transmitter data register is empty. If the
		 *           interrupt occurred because a character was received, this handler 
		 *           puts the character into the receiver queue; if it occurred because 
		 *           the transmitter is empty, it checks for another character to be 
		 *           transmitted and sends that character if there is one available. 
		 */
		extern "C" void USART1_IRQHandler (void)
		{
			BaseType_t dummy;               // Receives unused stuff from queue call
			uint8_t data;                   // A character transmitted or received

			// If a character has been received, put it into the queue
			if (USART_GetITStatus (USART1, USART_IT_RXNE) != RESET)
			{
				data = USART_ReceiveData (USART1);                        // & 0xFF;
				xQueueSendToBack (receiver_queue_1, &data, 0);
			}

			// If the transmitter buffer is empty, check for a character to send
			if (USART_GetITStatus (USART1, USART_IT_TXE) != RESET)
			{
				#if UART_USE_TX_BUFFERS == 1
					if (xQueueReceive (transmitter_queue_1, &data, 0) == pdTRUE)
					{
						USART_SendData (USART1, data);
					}
					else        // Nothing to send; disable transmitter empty interrupt
					{
						USART_ITConfig (USART1, USART_IT_TXE, DISABLE);
						tx_irq_on &= ~(1 << 1);
					}
				#endif
			}
		}
	#endif
#endif // USART_1_ENABLE

#if USART_2_ENABLE == 1
	/** @brief   A queue that holds incoming characters from the receiver.
	 *  @details This queue holds characters that are received by the serial port.
	 *           The receipt of each character causes an interrupt, and the 
	 *           interrupt service routine puts the character in the queue.
	 */
	QueueHandle_t receiver_queue_2;

	#if UART_USE_TX_BUFFERS == 1
		/** @brief   A queue that holds characters to be sent by the transmitter.
		 *  @details This queue holds characters to be transmitted by the serial port. 
		 */
		QueueHandle_t transmitter_queue_2;
	#endif

	#if defined __AVR

	#elif defined __ARMEL__
		//-----------------------------------------------------------------------------
		/** @brief   Interrupt handler for USART 2.
		 *  @details This interrupt handler runs when a character has been received by 
		 *           the UART @b or when the transmitter data register is empty. If the
		 *           interrupt occurred because a character was received, this handler 
		 *           puts the character into the receiver queue; if it occurred because 
		 *           the transmitter is empty, it checks for another character to be 
		 *           transmitted and sends that character if there is one available. 
		 */
		extern "C" void USART2_IRQHandler (void)
		{
			uint8_t data;                   // A character transmitted or received

			// If a character has been received, put it into the queue
			if (USART_GetITStatus (USART2, USART_IT_RXNE) != RESET)
			{
				data = USART_ReceiveData (USART2);
				xQueueSendToBack (receiver_queue_2, &data, 0);
			}

			// If the transmitter buffer is empty, check for a character to send
			if (USART_GetITStatus (USART2, USART_IT_TXE) != RESET)
			{
				#if UART_USE_TX_BUFFERS == 1
					if (xQueueReceive (transmitter_queue_2, &data, 0) == pdTRUE)
					{
						USART_SendData (USART2, data);
					}
					else        // Nothing to send; disable transmitter empty interrupt
					{
						USART_ITConfig (USART2, USART_IT_TXE, DISABLE);
						tx_irq_on &= ~(1 << 2);
					}
				#endif
			}
		}
	#endif
#endif // USART_2_ENABLE

#if USART_3_ENABLE == 1
	/** @brief   A queue that holds incoming characters from the receiver.
	 *  @details This queue holds characters that are received by the serial port.
	 *           The receipt of each character causes an interrupt, and the 
	 *           interrupt service routine puts the character in the queue.
	 */
	QueueHandle_t receiver_queue_3;

	#if UART_USE_TX_BUFFERS == 1
		/** @brief   A queue that holds characters to be sent by the transmitter.
		 *  @details This queue holds characters to be transmitted by the serial port. 
		 */
		QueueHandle_t transmitter_queue_3;
	#endif

	#if defined __AVR

	#elif defined __ARMEL__
		//-----------------------------------------------------------------------------
		/** @brief   Interrupt handler for USART 3.
		 *  @details This interrupt handler runs when a character has been received by 
		 *           the UART @b or when the transmitter data register is empty. If the
		 *           interrupt occurred because a character was received, this handler 
		 *           puts the character into the receiver queue; if it occurred because 
		 *           the transmitter is empty, it checks for another character to be 
		 *           transmitted and sends that character if there is one available. 
		 * 
		 *           @b Feature: This code crashes if @c xQueueSendToBackFromISR() is
		 *           used, but it works fine if @c xQueueSendToBack() is used.  This 
		 *           makes no sense to the author because we're in an interrupt 
		 *           handler here. But it works this way, so let's keep it. 
		 */
		extern "C" void USART3_IRQHandler (void)
		{
			uint8_t data;                   // A character transmitted or received

			if (USART_GetITStatus (USART3, USART_IT_RXNE) != RESET)
			{
				data = USART_ReceiveData (USART3);
				xQueueSendToBack (receiver_queue_3, &data, 0);
			}
			if (USART_GetITStatus (USART3, USART_IT_TXE) != RESET)
			{
				#if UART_USE_TX_BUFFERS == 1
					if (xQueueReceive (transmitter_queue_3, &data, 0) == pdTRUE)
					{
						USART_SendData (USART3, data);
					}
					else
					{
						USART_ITConfig (USART3, USART_IT_TXE, DISABLE);
						tx_irq_on &= ~(1 << 3);
					}
				#endif
			}
		}
	#endif
#endif // USART_3_ENABLE

#if UART_4_ENABLE == 1
#endif // UART_4_ENABLE

#if UART_5_ENABLE == 1
#endif // UART_5_ENABLE

#if USART_6_ENABLE == 1
#endif // USART_6_ENABLE


#if defined __ARMEL__ || defined __DOXYGEN__
	//---------------------------------------------------------------------------------
	/** @brief   Constructor which sets up a U(S)ART as a \c cout style stream.
	 *  @details This constructor sets up a U(S)SART for communications. It calls the 
	 *           @c emstream constructor, preparing to convert things to text strings.
	 *           It allocates memory for the transmitter and receiver queues and does
	 *           all the setup (clocks, baud rate, etc.) to make the U(S)ART run. 
	 *  @param   p_usart A pointer to the U(S)ART structure from the STM32 standard
	 *           peripheral library which we're using, for example @c USART1
	 *  @param   a_baud_rate The baud rate at which the U(S)ART will initially run
	 */

	RS232::RS232 (USART_TypeDef* p_usart, uint32_t a_baud_rate) 
		: emstream ()
	{
		// Save the pointer to the U(S)ART structure with which we associate
		p_USART = p_usart;

		// Save the baud rate
		baud_rate = a_baud_rate;

		// Create the queues which will be used to send and receive characters
		receiver_queue = xQueueCreate (UART_RX_BUF_SZ, sizeof (char));
		#if UART_USE_TX_BUFFERS == 1
			transmitter_queue = xQueueCreate (UART_TX_BUF_SZ, sizeof (char));
		#endif

		// Each U(S)ART has a somewhat different setup; choose the correct one
		#if USART_1_ENABLE == 1
			if (p_USART == USART1)
			{
				// Copy the pointers to the queues for incoming and outgoing characters
				// to file-scope, global queue handles which are needed because the 
				// interrupt handler needs to be able to access the queues. Also set
				// the mask used to find the tx_irq_on bit for this U(S)ART
				receiver_queue_1 = receiver_queue;
				#if UART_USE_TX_BUFFERS == 1
					transmitter_queue_1 = transmitter_queue;
					tx_irq_on_mask = (1 << 1);
				#endif

				// Enable clocks to the USART and to the GPIO port used by the pins
				RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);

				// Set up the GPIO pins; each U(S)ART uses different pins of course
				init_pins (GPIOA, 9, 10);
			}
		#endif
		#if USART_2_ENABLE == 1
			if (p_USART == USART2)
			{
				receiver_queue_2 = receiver_queue;
				#if UART_USE_TX_BUFFERS == 1
					transmitter_queue_2 = transmitter_queue;
					tx_irq_on_mask = (1 << 2);
				#endif
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART2, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				init_pins (GPIOA, 2, 3);
				init_interrupts (USART2_IRQn);
			}
		#endif
		#if USART_3_ENABLE == 1
			if (p_USART == USART3)
			{
				receiver_queue_3 = receiver_queue;
				#if UART_USE_TX_BUFFERS == 1
					transmitter_queue_3 = transmitter_queue;
					tx_irq_on_mask = (1 << 3);
				#endif
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART3, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
				init_pins (GPIOB, 10, 11);
				init_interrupts (USART3_IRQn);
// 				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);
// 				init_pins (GPIOD, 8, 9);
			}
		#endif
		#if UART_4_ENABLE == 1
			if (p_USART == UART4)
			{
				receiver_queue_4 = receiver_queue;
				#if UART_USE_TX_BUFFERS == 1
					transmitter_queue_4 = transmitter_queue;
					tx_irq_on_mask = (1 << 4);
				#endif
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_UART4, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				init_pins (GPIOA, 0, 1);
			}
		#endif
		#if UART_5_ENABLE == 1
			if (p_USART == UART5)  // Uses pins on two GPIO ports, what a pain...
			{
			}
		#endif
		#if USART_6_ENABLE == 1
			if (p_USART == USART6)
			{
				receiver_queue_6 = receiver_queue;
				#if UART_USE_TX_BUFFERS == 1
					transmitter_queue_6 = transmitter_queue;
					tx_irq_on_mask = (1 << 6);
				#endif
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART6, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				init_pins (GPIOC, 6, 7);
			}
		#endif

		// This data structure is used by the STM32 standard peripheral library
		USART_InitTypeDef USART_InitStruct;
		USART_StructInit (&USART_InitStruct);

		// Fill in the data structure and have the standard library do the setup
		USART_InitStruct.USART_BaudRate = baud_rate * UART_FIX_BAUD_HACK;
		USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_InitStruct.USART_WordLength = USART_WordLength_8b;
		USART_InitStruct.USART_StopBits = USART_StopBits_1;
		USART_InitStruct.USART_Parity = USART_Parity_No;
		USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_Init (p_USART, &USART_InitStruct);

		// Turn on receiver interrupts and turn on the U(S)ART itself
		USART_ITConfig (p_USART, USART_IT_RXNE, ENABLE);     // Receive interrupts on       /////////////
		USART_Cmd (p_USART, ENABLE);                         // Turn on the U(S)ART
	}


	//---------------------------------------------------------------------------------
	/** @brief   Set up the interrupts to be used by a given U(S)ART.
	 *  @details TODO
	 *  @param   which_int The interrupt which is to be set up, for example 
	 *                     @c USART1_IRQn (defined in @c stm32f4xx.h)
	 */

	void RS232::init_interrupts (IRQn_Type which_int)
	{
		// Structure used to set up interrupt for the given U(S)ART
		NVIC_InitTypeDef NVIC_InitStruct;

		// Fill up the structure, then have NVIC_Init() do the work
		NVIC_InitStruct.NVIC_IRQChannel = which_int;
		NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
		NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init (&NVIC_InitStruct);
	}


	//---------------------------------------------------------------------------------
	/** @brief   Set up the GPIO pins used by an STM32's U(S)ART.
	 *  @details This function configures the I/O pins used by the U(S)ART of an STM32
	 *           as an output with push-pull amplifier for the transmitter and an
	 *           input for the receiver. The GPIO's clock must have been enabled 
	 *           separately. 
	 *  @param   p_port A pointer to the I/O port's GPIO structure, @c GPIOx
	 *  @param   tx_pin The pin number of the TXD pin. Do @b not use the macro 
	 *                  GPIO_Pin_x, it won't work; use a number between 0 and 15
	 *  @param   rx_pin The pin number of the RXD pin
	 */

	void RS232::init_pins (GPIO_TypeDef* p_port, uint16_t tx_pin, uint16_t rx_pin)
	{
		// Create the initialization structure
		GPIO_InitTypeDef GP_Init_Struct;
		GPIO_StructInit (&GP_Init_Struct);

		// Set up pin used for transmission for alternate function, push-pull
		GP_Init_Struct.GPIO_Pin = (1 << tx_pin);
		GP_Init_Struct.GPIO_Speed = GPIO_Speed_50MHz;
		GP_Init_Struct.GPIO_Mode = GPIO_Mode_AF;
		GP_Init_Struct.GPIO_OType = GPIO_OType_PP;
		GPIO_Init (p_port, &GP_Init_Struct);

		// Receiver pin is set up as an alternate function input
		GP_Init_Struct.GPIO_Pin = (1 << rx_pin);
		GP_Init_Struct.GPIO_Speed = GPIO_Speed_50MHz;
		GP_Init_Struct.GPIO_Mode = GPIO_Mode_AF;
		GPIO_Init (p_port, &GP_Init_Struct);

		// Connect output and input pins to alternate function of this U(S)ART
		GPIO_PinAFConfig (p_port, tx_pin, GPIO_AF_USART2);
		GPIO_PinAFConfig (p_port, rx_pin, GPIO_AF_USART2);
	}
#endif // __ARMEL__


//-------------------------------------------------------------------------------------
/** @brief   Activate the serial port.
 *  @details This method activates the serial port, turning on the clock and U(S)ART
 *           hardware which may have been turned off by @c stop(). 
 */

void RS232::start (void)
{
	#if defined __AVR

	#elif defined __ARMEL__
		#if USART_1_ENABLE == 1
			if (p_USART == USART1)
			{
				// Enable clocks to the USART and to the GPIO port used by the pins
				RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART1, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				USART_ITConfig (USART1, USART_IT_RXNE, ENABLE);
				return;
			}
		#endif
		#if USART_2_ENABLE == 1
			if (p_USART == USART2)
			{
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART2, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				USART_ITConfig (USART2, USART_IT_RXNE, ENABLE);
				return;
			}
		#endif
		#if USART_3_ENABLE == 1              // Using TX = PB10 and RX = PB11
			if (p_USART == USART3)
			{
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART3, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
				USART_ITConfig (USART3, USART_IT_RXNE, ENABLE);
				return;
			}
		#endif
		#if UART_4_ENABLE == 1
			if (p_USART == UART4)
			{
				RCC_APB1PeriphClockCmd (RCC_APB1Periph_USART4, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
				USART_ITConfig (USART4, USART_IT_RXNE, ENABLE);
				return;
			}
		#endif
		#if UART_5_ENABLE == 1
			if (p_USART == UART5)
			{
				// TODO: Need to figure out the two-GPIO-ports problem
				return;
			}
		#endif
		#if USART_6_ENABLE == 1
			if (p_USART == USART6)
			{
				RCC_APB2PeriphClockCmd (RCC_APB2Periph_USART6, ENABLE);
				RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);
				USART_ITConfig (USART6, USART_IT_RXNE, ENABLE);
				return;
			}
		#endif
	#endif
}


//-------------------------------------------------------------------------------------
/** @brief   Turn off a serial port to save power.
 *  @details This method turns off a serial port, thus saving power, if the processor
 *           supports deactivating the port. The port can be restarted using its
 *           @c start() method. NOTE: This method hasn't been tested yet. 
 */

void RS232::stop (void)
{
	// For Atmel 8-bit AVR processors
	#if defined __AVR

	// For the STM32F4
	#elif defined __ARMEL__
		// Turn off the clock to the USART; turn off the USART; disable interrupts
		#if USART_1_ENABLE == 1
			if (p_USART == USART1)
			{
			}
		#endif
		#if USART_2_ENABLE == 1
			if (p_USART == USART2)
			{
				RCC_APB2PeriphClockCmd (RCC_APB1Periph_USART2, DISABLE);
				USART_Cmd (USART2, DISABLE);
				USART_ITConfig (USART2, USART_IT_RXNE, DISABLE);
			}
		#endif
		#if USART_3_ENABLE == 1
			if (p_USART == USART3)
			{
				RCC_APB2PeriphClockCmd (RCC_APB1Periph_USART3, DISABLE);
				USART_Cmd (USART3, DISABLE);
				USART_ITConfig (USART3, USART_IT_RXNE, DISABLE);
			}
		#endif
		#if USART_4_ENABLE == 1
			if (p_USART == UART4)
			{
			}
		#endif
		#if USART_5_ENABLE == 1
			if (p_USART == UART5)
			{
			}
		#endif
		#if USART_6_ENABLE == 1
			if (p_USART == USART6)
			{
			}
		#endif
	#endif
}


//-------------------------------------------------------------------------------------
/** @brief   Send one character through the serial port.
 *  @details This method sends one character to the UART. If a transmitter queue and
 *           interrupts are being used, the character will be sent to the serial port
 *           output queue and the port's interrupt handler will send a queued character
 *           when the transmitter empty interrupt goes off. If the queue is not being
 *           used (@c UART_USE_TX_BUFFERS isn't one), this method waits until the 
 *           transmitter has transmitted a previous character, then puts the character
 *           to be transmitted into the transmitter buffer. 
 *  @param   chout The character to be sent out
 */

void RS232::putchar (char chout)
{
	#if UART_USE_TX_BUFFERS == 1
		// Stuff the character in the transmitter queue. If the transmitter interrupt has
		// been turned off, turn it back on; this should trigger an interrupt
		xQueueSendToBack (transmitter_queue, &chout, portMAX_DELAY);
		if (!(tx_irq_on & tx_irq_on_mask))
		{
			tx_irq_on |= tx_irq_on_mask;
			USART_ITConfig (p_USART, USART_IT_TXE, ENABLE);
		}
	#else
		// Backup version with no transmitter queue
		while (!(p_USART->SR & USART_FLAG_TXE))
		{
		}
		p_USART->DR = chout;
	#endif
}


//-------------------------------------------------------------------------------------
/** @brief   Get a character from the serial port.
 *  @details This method gets one character from the serial port, if one is there.
 *           If not, it waits until there is a character available. This can block
 *           the receiving task, which is a good thing if that task is designed to
 *           wait without wasting processor cycles until a character is available. 
 *           If a non-blocking implementation is desired, call @c check_for_char() 
 *           to make sure a character is available before calling this method. 
 *  @return  The character which was found in the serial port receive buffer
 */

char RS232::getchar (void)
{
	char got_this = '\0';

	xQueueReceive (receiver_queue, &got_this, UART_GETCHAR_DELAY);

	return (got_this);
}


//-------------------------------------------------------------------------------------
/** @brief   Check if a character is available in the serial port.
 *  @details This method checks if there is a character in the serial port's 
 *           receiver queue. The queue will have been filled if a character came 
 *           in through the serial port. 
 *  @return  True for character available, false for no character available
 */

bool RS232::check_for_char (void)
{
	return (uxQueueMessagesWaiting (receiver_queue) > 0);
}


//-------------------------------------------------------------------------------------
/** @brief   Look at a character in the serial port without removing it. 
 *  @details This method returns the next character which has been received by the 
 *           serial port but leaves it in the port buffer to be read by a future call
 *           to @c getchar().  This can block the task calling this function, which 
 *           may be a good thing if that task is designed to wait without wasting 
 *           processor cycles until a character is available. 
 *           If a non-blocking implementation is desired, call @c check_for_char() 
 *           to make sure a character is available before calling this method. 
 *  @return  The character which is sitting in the serial port receive buffer
 */

char RS232::peek (void)
{
	char got_this = '\0';

	xQueuePeek (receiver_queue, &got_this, UART_GETCHAR_DELAY);

	return (got_this);
}


//**************************************************************************************
/** @file    irq_timer.h
 *  @brief   Header for a class that runs a pulse frequency couter on the STMpunk board.
 *  @details This file contains headers for a class that sets up and runs a 
 *           timer-counter in the STM32F4 to measure the rate at which pulses arrive at
 *           an external interrupt pin. It's not as accurate as a timer which uses an
 *           input capture pin, but it's simpler and more flexible. 
 *
 *  Revisions:
 *    @li 12-17-2014 JRR Original file
 *    @li 01-10-2015 JRR Morphed into an interrupt based pulse timer class
 *
 *  License:
 *      This file is copyright 2015 by JR Ridgely and released under the Lesser GNU 
 *      Public License, version 3. It intended for educational use only, but its use
 *      is not limited thereto. */
/*      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *      AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *      IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *      ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *      LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *      TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *      OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *      CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *      OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//**************************************************************************************

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _IRQ_TIMER_H_
#define _IRQ_TIMER_H_

#include "FreeRTOS.h"                       // Header for FreeRTOS critical sections

#include "stm32f4xx.h"                      // Main header for STM32 peripheral library
#include "stm32f4xx_gpio.h"                 // Header for GPIO ports
#include "stm32f4xx_rcc.h"                  // Header for the real-time clock
#include "stm32f4xx_tim.h"                  // Header for STM32 counter/timers
#include "stm32f4xx_syscfg.h"               // Header for system configuration thing??
#include "stm32f4xx_exti.h"                 // Header for external interrupts
#include "misc.h"                           // Header for interrupt controller is here


/** @brief   Flag which causes compiling of code for the Pin 10 frequency timer.
 */
#define IRQT_PIN10

/** @brief   Flag which causes compiling of code for the Pin 11 frequency timer.
 */
#define IRQT_PIN11

/** @brief   Flag which causes compiling of code for the Pin 12 frequency timer.
 */
#undef IRQT_PIN12

/** @brief   Flag which causes compiling of code for the Pin 13 frequency timer.
 */
#undef IRQT_PIN13

/** @brief   Flag which causes compiling of code for the Pin 14 frequency timer.
 */
#undef IRQT_PIN14

/** @brief   Flag which causes compiling of code for the Pin 15 frequency timer.
 */
#undef IRQT_PIN15


/** @brief   The clock rate in Hertz for the timer/counter used by the interrupt based 
 *           timer. 
 *  @details We prefer to use a 32 bit timer running at 1 MHz so that there is
 *           good precision and the timer doesn't overflow for a while (about one hour,
 *           eleven and a half minutes). A megahertz is represented by a value of 
 *           @c 1000000L. 
 */
const uint32_t IRQ_TIMER_CK_RATE = 1000000L;

/** @brief   The timer/counter used for the interrupt based pulse timer.
 *  @details It is recommended to use either @c TIM5 or @c TIM2 because those are the
 *           32-bit timers. Because @c TIM2 is connected to useful external pins on
 *           the STM32F405VG, the best timer for the interrupt based pulse timer, which
 *           doesn't use hardware to directly influence the timer, is usually @c TIM5,
 *           unless the frequency range being measured is fairly narrow, in which case
 *           a 16-bit timer such as @c TIM14 is a good choice. In any case, the type
 *           specified in @c IRQT_CTR_TYPE @b must match the bit width of the timer. 
 */
#define IRQ_TIMER_TIMER     TIM5

/** @brief   The clock signal used by the interrupt based timer's timer/counter.
 *  @details Some timers use the @c RCC_APB1 clock system and other use the @c RCC_APB2
 *           clock system; make sure to check which clock system is used by each 
 *           particular timer you might be using.
 */
#define IRQ_TIMER_TMRCLK    RCC_APB1Periph_TIM5

/** @brief   The command to enable the timer/counter's clock.
 */
#define IRQ_TIMER_TMCLKCMD  RCC_APB1PeriphClockCmd


//-------------------------------------------------------------------------------------
/** @brief   The type of integer to be used for the interrupt timer.
 *  @details This definition sets the type of integer which is used to perform interval
 *           computations with the timer/counter's data. It must match the bit width of
 *           the timer/counter - either 32 bits for @c TIM5 (recommended) or @c TIM2,
 *           or 16 bits for any other timer which might be used. 
 */
#define IRQT_CTR_TYPE       int32_t


//-------------------------------------------------------------------------------------
/** @brief   Preprocessor variable which is defined to enable diagnostic printouts.
 */
#define IRQT_SER_DBG

/** @brief   Macro function which prints debugging messages if @c IRQT_SER_DBG has 
 *           been defined. 
 */
#ifdef IRQT_SER_DBG
	#include "emstream.h"
	#define IRQT_DBG(x) if (p_serial) *p_serial << x
#else
	#define IRQT_DBG(x)
#endif


//-------------------------------------------------------------------------------------
/** @brief   Uses an external interrupt on the STM32 to measure pulse frequency. 
 *  @details This class sets up a timer/counter on the STM32 as a time base and an
 *           external interrupt pin as an input. The period at which edges
 *           occur on the input is measured. The most recently measured period and 
 *           frequency can be queried at any time by other code in a program.
 *           Some helpful information came from 
 *        @c http://stm32f4-discovery.com/2014/08/stm32f4-external-interrupts-tutorial/
 */

class irq_timer
{
protected:
	/** @brief   This flag records when the timer/counter has been set up.
	 */
	static bool timer_set_up;

	/** @brief   Pointer to the variable which holds the period between pulse edges.
	 */
	IRQT_CTR_TYPE* p_period;

	/** @brief   The pin number on the GPIO port used for the input pin.
	 *  @details This number selects which pin of the GPIO port is used to read the
	 *           signal being measured. It also controls which external interrupt 
	 *           source is used and where the measured period data is stored.
	 */
	uint8_t pin_num;

	/** @brief   The most recently measured period between interrupts.
	 *  @details This variable keeps track of the time, as measured by the timer
	 *           associated with this object, between the most recent two edges
	 *           detected in the input signal. It can be used to calculate the
	 *           period (generally given in microseconds) or frequency (in Hertz)
	 *           of the signal. 
	 */
	IRQT_CTR_TYPE period;

	/// Previous value read from the hardware timer/counter. 
	IRQT_CTR_TYPE previous_time;

	#ifdef IRQT_SER_DBG
		/// Pointer to a serial device which displays debugging messages.
		emstream* p_serial;
	#endif

public:
	#ifdef IRQT_PIN10
		/** @brief   Holds period in timer ticks between two signal edges on pin 10.
		 *  @details This public static member variable is accessible to the interrupt
		 *           handler which responds to pulses on pin 10, computing the period
		 *           between the previous pulse and the most recent pulse and storing
		 *           the result in this variable. 
		 */
		static IRQT_CTR_TYPE period_10;
	#endif
	#ifdef IRQT_PIN11
		/** @brief   Holds period in timer ticks between two signal edges on pin 11. */
		static IRQT_CTR_TYPE period_11;
	#endif
	#ifdef IRQT_PIN12
		/** @brief   Holds period in timer ticks between two signal edges on pin 12. */
		static IRQT_CTR_TYPE period_12;
	#endif
	#ifdef IRQT_PIN13
		/** @brief   Holds period in timer ticks between two signal edges on pin 13. */
		static IRQT_CTR_TYPE period_13;
	#endif
	#ifdef IRQT_PIN14
		/** @brief   Holds period in timer ticks between two signal edges on pin 14. */
		static IRQT_CTR_TYPE period_14;
	#endif
	#ifdef IRQT_PIN15
		/** @brief   Holds period in timer ticks between two signal edges on pin 15. */
		static IRQT_CTR_TYPE period_15;
	#endif

	// This constructor initializes the timer/counter and GPIO ports
	irq_timer (GPIO_TypeDef* port, uint8_t pin_number
				  #ifdef IRQT_SER_DBG
					  , emstream* p_ser_dev = NULL
				  #endif
				 );

	// Get the current count from the quadrature encoder
	IRQT_CTR_TYPE get_period (void);

	// Update the count, checking for overflows; this must be done periodically
	float get_frequency (void);

	/** @brief   Get a pointer to the timer/counter used by this interrupt timer.
	 *  @details This method returns a pointer to the timer/counter used by this
	 *           timing measurement object. The pointer is returned as @c const, so
	 *           it is not able to be altered by the calling function.
	 */
	const TIM_TypeDef* get_p_timer (void)
	{
		return (IRQ_TIMER_TIMER);
	}
/*
	// Declare interrupt handlers which need to access protected variables as friends
	friend void ::EXTI15_10_IRQHandler (void);*/
};


#endif // _IRQ_TIMER_H_

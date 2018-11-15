//**************************************************************************************
/** @file    quad_counter.h
 *  @brief   Headers for a class that runs a quadrature encoder on the STMpunk board.
 *  @details This file contains headers for a class that sets up and runs a 
 *           timer-counter in the STM32F4 in quadrature reading mode. It maintains a 
 *           count of current position, assuming that there's an incremental encoder
 *           supplying the pulses. 
 *
 *  Revisions:
 *    @li 12-17-2014 JRR Original file
 *
 *  License:
 *      This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
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
#ifndef _QUAD_COUNTER_H_
#define _QUAD_COUNTER_H_

#include "FreeRTOS.h"                       // Header for FreeRTOS critical sections

#include "stm32f4xx.h"                      // Main header for STM32 peripheral library
#include "stm32f4xx_gpio.h"                 // Header for GPIO ports
#include "stm32f4xx_rcc.h"                  // Header for the real-time clock
#include "stm32f4xx_tim.h"                  // Header for STM32 counter/timers
#include "misc.h"                           // Header for interrupt controller is here


//-------------------------------------------------------------------------------------
/** @brief   The type of integer to be used for a software quadrature counter.
 *  @details This definition sets the type of integer which is used as the software
 *           counter that holds the position of a quadrature encoder. It needs to be
 *           large enough to hold the largest position (a number of bits) to which the 
 *           encoder may travel.  An @c int32_t is usually sufficient, as it can count
 *           to about 2.3 billion; in some cases, an @c int64_t may be needed. The 
 *           type should be signed, as the encoder might travel in a positive or 
 *           negative direction; if negative can travel never occur, quadrature isn't
 *           needed. 
 */
#define QUAD_CTR_TYPE       int32_t


//-------------------------------------------------------------------------------------
/** @brief   Preprocessor variable which is defined to enable diagnostic printouts.
 */
#define QUAD_SER_DBG

/** @brief   Macro function which prints debugging messages if @c QUAD_SER_DBG has 
 *           been defined. 
 */
#ifdef QUAD_SER_DBG
	#include "emstream.h"
	#define QUAD_DBG(x) if (p_serial) *p_serial << x
#else
	#define QUAD_DBG(x)
#endif


//-------------------------------------------------------------------------------------
/** @brief   Type for an entry into a table of GPIO ports and pins for timer/counters.
 */
class quad_timer_counter_set
{
public:
	/** @brief   The timer/counter which is used.
	 */
	TIM_TypeDef* p_timer;

	/** @brief   The GPIO port for the pins used as quadrature inputs.
	 */
	GPIO_TypeDef* p_port;

	/** @brief   An array of pin numbers, one for each pin used by a given channel.
	 *  @details In this array of pin numbers, the timer/counter channel to which the
	 *           pin number applied is between 1 and 4, while the array indices are
	 *           between 0 and 3.
	 */
	uint8_t pin_number[4];

	/** @brief   The alternate function code for the given timer/counter. 
	 */
	uint8_t alt_function;

	/** @brief   The clock signal for the GPIO port, such as @c RCC_AHB1Periph_GPIOA.
	 */
	uint32_t gpio_clock;

	/** @brief   The clock signal for the timer, such as @c RCC_APB2Periph_TIM1.
	 */
	uint32_t timer_clock;
};


//-------------------------------------------------------------------------------------
/** @brief   Table of GPIO ports, clocks, and pins matching timer/counter channels. 
 *  @details This table holds a set of GPIO ports and pins and peripheral clocks so
 *           that a given timer/counter can easily be set up as a quadrature counter.
 *           The ports and pins chosen are convenient sets taken from the alternate 
 *           [pin] function mapping in the STM32F40x reference; some of these pin
 *           mappings can be changed if needed to work with different board designs. 
 *           WARNING: Some pins may overlap between timers; for example, pins PC6 
 *           through PC9 can be used by Timer 3 or Timer 8, but it would not be useful
 *           for both timers to use the same pins at the same time. 
 */
const quad_timer_counter_set TC_SET[6] = 
{
	// Port, pins {#1, #2, #3, #4}, alt. function code, GPIO clock, timer clock
	{TIM1, GPIOA, { 8,  9, 10, 11}, GPIO_AF_TIM1, RCC_AHB1Periph_GPIOA, RCC_APB2Periph_TIM1},
	{TIM2, GPIOB, { 0,  3, 10, 11}, GPIO_AF_TIM2, RCC_AHB1Periph_GPIOB, RCC_APB1Periph_TIM2},
	{TIM3, GPIOB, { 4,  5,  0,  1}, GPIO_AF_TIM3, RCC_AHB1Periph_GPIOB, RCC_APB1Periph_TIM3},
	{TIM4, GPIOB, { 6,  7,  8,  9}, GPIO_AF_TIM4, RCC_AHB1Periph_GPIOB, RCC_APB1Periph_TIM4},
	{TIM5, GPIOA, { 0,  1,  2,  3}, GPIO_AF_TIM5, RCC_AHB1Periph_GPIOA, RCC_APB1Periph_TIM5},
	{TIM8, GPIOC, { 6,  7,  8,  9}, GPIO_AF_TIM8, RCC_AHB1Periph_GPIOC, RCC_APB2Periph_TIM8}
};


//-------------------------------------------------------------------------------------
/** @brief   Uses a timer/counter on the STM32 as a quadrature counter. 
 *  @details This class sets up a timer/counter on the STM32 as a quadrature counter,
 *           adding a software counter which has more bits than the hardware 
 *           timer/counter so as to prevent overflows. This class was created using
 *           inspiration from 
 *  @c http://www.micromouseonline.com/2013/02/16/quadrature-encoders-with-the-stm32f4/
 */

class quad_counter
{
protected:
	/// Pointer to the timer/counter being used by this object. 
	TIM_TypeDef* p_timer;

	/** @brief   The most recently measured position of the quadrature encoder.
	 *  @details This variable keeps track of the current position of the quadrature
	 *           encoder, being updated each time the @c update() method is called. 
	 *           It functions as a sort of extended version of the hardware timer,
	 *           being updated with the difference between current and previous 
	 *           readings of the hardware counter. The type @c QUAD_CTR_TYPE, defined
	 *           in @c quad_counter.h, specifies the type (typically @c uint32_t) for
	 *           this position variable. 
	 */
	QUAD_CTR_TYPE count;

	/// Previous value read from the hardware timer/counter. 
	int16_t previous_reading;

	#ifdef QUAD_SER_DBG
		/// Pointer to a serial device which displays debugging messages.
		emstream* p_serial;
	#endif

public:
	// This constructor initializes the timer/counter and GPIO ports
	quad_counter (uint8_t timer, uint16_t A_channel, uint16_t B_channel
				  #ifdef QUAD_SER_DBG
					, emstream* p_ser_dev = NULL
				  #endif
				 );

	// Get the current count from the quadrature encoder
	QUAD_CTR_TYPE get (void);

	// Update the count, checking for overflows; this must be done periodically
	void update (void);

	// Zero the encoder counter at the current location
	void zero (void);

	// Set the count at the current location to the given value
	void set (QUAD_CTR_TYPE);

	/** @brief   Get a pointer to the timer/counter used by this decoder.
	 *  @details This method returns a pointer to the timer/counter used by this
	 *           encoder decoder object. The pointer is returned as @c const, so
	 *           it is not able to be altered by the calling function.
	 */
	const TIM_TypeDef* get_p_timer (void)
	{
		return (p_timer);
	}
};


#endif // _QUAD_COUNTER_H_

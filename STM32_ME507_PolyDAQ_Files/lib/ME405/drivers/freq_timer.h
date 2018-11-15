//**************************************************************************************
/** @file    freq_timer.h
 *  @brief   Headers for a class that does input capture timing on the STMpunk board.
 *  @details This file contains headers for a class that sets up and runs an input 
 *           capture unit on the STM32F4. It maintains a reading of the frequency at 
 *           which pulses are arriving at the given input. 
 *
 *  Revisions:
 *    @li 11-29-2014 JRR Original file
 *
 *  License:
 *      This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *      Public License, version 3. It intended for educational use only, but its use
 *      is not limited thereto. */
/*      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *      AND	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
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
#ifndef _FREQ_TIMER_H_
#define _FREQ_TIMER_H_

#include "FreeRTOS.h"                       // Header for FreeRTOS critical sections

/*
 * CRAZYNESS - to define one vaible in makefile to include mutiple headers
 * PPCAT SECOND ARG CANNOT START WITH . (ex .h is invalid)
 */
/*
 * Concatenate preprocessor tokens A and B without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define PPCAT_NX(A, B) A ## B
/*
 * Concatenate preprocessor tokens A and B after macro-expanding them.
 */
#define PPCAT(A, B) PPCAT_NX(A, B)
/*
 * Turn A into a string literal without expanding macro definitions
 * (however, if invoked from a macro, macro arguments are expanded).
 */
#define STRINGIZE_NX(A) #A
/*
 * Turn A into a string literal after macro-expanding it.
 */
#define STRINGIZE(A) STRINGIZE_NX(A)

/*
 * STM_GENERIC_HEADER concatinated with  x.h and other postfxes(like: x_gpio.h)
 * now defined in makefile -DSTM_GENERIC_HEADER=stm32f10
 * #define STM_GENERIC_HEADER stm32f1
 */
#ifndef STM_GENERIC_HEADER
#error "STM_GENERIC_HEADER not defined (define in BOARDFLAGS = -DSTM_GENERIC_HEADER=stm32f10)"
#endif


#include STRINGIZE(PPCAT(STM_GENERIC_HEADER,x.h))       // Main header for STM32 peripheral library
//#include "stm32f4xx.h"                                // Main header for STM32 peripheral library
#include STRINGIZE(PPCAT(STM_GENERIC_HEADER,x_gpio.h))  // Header for GPIO ports
//#include "stm32f4xx_gpio.h"                           // Header for GPIO ports
#include STRINGIZE(PPCAT(STM_GENERIC_HEADER,x_rcc.h))   // Header for the real-time clock
//#include "stm32f4xx_rcc.h"                            // Header for the real-time clock
#include STRINGIZE(PPCAT(STM_GENERIC_HEADER,x_rcc.h))   // Header for STM32 counter/timers
//#include "stm32f4xx_tim.h"                            // Header for STM32 counter/timers
#include "misc.h"                                       // Header for interrupt controller is here
//END CRAZYNESS


//-------------------------------------------------------------------------------------

/// This macro is defined if the user is using Timer 1 as a frequency counter. 
#define FREQ_CTR_USE_TIM1

#undef FREQ_CTR_USE_TIM2
#undef FREQ_CTR_USE_TIM3                    // Used for motor drivers on STMpunk board
#undef FREQ_CTR_USE_TIM4                    // Used for orange LED on STMpunk board
#undef FREQ_CTR_USE_TIM5
#undef FREQ_CTR_USE_TIM6
#undef FREQ_CTR_USE_TIM7
#undef FREQ_CTR_USE_TIM8


//-------------------------------------------------------------------------------------
/** @brief   Control a timer/counter to do input capture based high-precision timing. 
 *  @details This class controls a timer/counter to do frequency measurement using the
 *           input caputure 
 */

class freq_timer
{
protected:
	/// Pointer to the timer/counter being used by this object. 
	TIM_TypeDef* p_timer;

public:
	// The constructor initializes the timer/counter and GPIO ports
	freq_timer (TIM_TypeDef* timer, uint32_t timer_clock, uint16_t channel,
				uint32_t clock_rate);

	// Read a frequency which has been measured on one of the timer's channels
	float get_freq (uint8_t channel);
};


#endif // _FREQ_TIMER_H_

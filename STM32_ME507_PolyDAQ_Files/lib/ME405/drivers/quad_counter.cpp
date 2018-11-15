//**************************************************************************************
/** @file    quad_counter.cpp
 *  @brief   Class that implements a quadrature encoder counter on the STMpunk board.
 *  @details This file contains source code for a class that sets up and runs a 
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

#include "quad_counter.h"                   // Header for this class is here


//-------------------------------------------------------------------------------------
/** @brief   Constructor for a quadrature encoder reading object. 
 *  @details The constructor initializes the timer/counter and GPIO ports necessary to
 *           read a quadrature encoder. Given which timer/counter to use and which 
 *           channels of that timer/counter to use for the A and B encoder inputs,
 *           this constructor calls code which is mostly supplied by macros to 
 *           initialize the timer/counter, software counter, and GPIO pins. 
 *  @param   timer The STM32 timer/counter used, from @c TIM1 to @c TIM8
 *  @param   A_channel The timer/counter channel to use for encoder signal A
 *  @param   B_channel The timer/counter channel to use for encoder signal B
 *  @param   p_ser_dev A pointer to a serial device which is used to print debugging
 *                     messages (only defined if @c QUAD_SER_DBG has been defined)
 */

quad_counter::quad_counter (uint8_t timer, uint16_t A_channel, uint16_t B_channel
							#ifdef QUAD_SER_DBG
							    , emstream* p_ser_dev
							#endif
						   )
{
	quad_timer_counter_set tc_set;          // Holds a set of initialization constants

	// Save the pointer to a serial device which may be used for debugging
	#ifdef QUAD_SER_DBG
		p_serial = p_ser_dev;
	#endif
	QUAD_DBG ("Quad Counter Constructor...");

	// Make sure the timer number is valid and the channel numbers are valid; if not,
	// the timer pointer will be NULL and this object won't do anything
	p_timer = NULL;
	if (A_channel <= 15 && B_channel <= 15)
	{
		if (timer >= 1 && timer <= 5)
		{
			tc_set = TC_SET[timer - 1];
		}
		else if (timer == 8)
		{
			tc_set = TC_SET[5];
		}
		else
		{
			QUAD_DBG ("ERROR: Invalid quadrature decoder configuration" << endl);
			return;
		}
	}

// 	// Save a pointer to the timer/counter being used for this object
// 	p_timer = tc_set.p_timer;
// 
// 	// Enable the clock to the GPIO port which is used for the pins
// 	RCC_AHB1PeriphClockCmd (tc_set.gpio_clock, ENABLE);
// 
// 	// Set up the pins used as channel A and B as inputs with pullup resitors on
// 	QUAD_DBG ("Channels " << A_channel << " and " << B_channel << endl);
// 	GPIO_InitTypeDef GPIO_InitStruct;
// 	GPIO_StructInit (&GPIO_InitStruct);
// 	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
// 	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
// 	GPIO_InitStruct.GPIO_Pin = 1 << (tc_set.pin_number[A_channel-1]);
// 	GPIO_Init (tc_set.p_port, &GPIO_InitStruct);
// 	GPIO_InitStruct.GPIO_Pin = 1 << (tc_set.pin_number[B_channel-1]);
// 	GPIO_Init (tc_set.p_port, &GPIO_InitStruct);
// 
// 	QUAD_DBG ("Pins: " << tc_set.pin_number[A_channel-1]
// 		<< " and " << tc_set.pin_number[B_channel-1] << endl);
// 	QUAD_DBG ("Pin mask: " << bin 
// 		<< (uint16_t)((1 << (tc_set.pin_number[A_channel-1]))
// 			| (1 << (tc_set.pin_number[B_channel-1]))) << dec << endl);
//  
// 	// Connect the pins to the alternate functions of encoder A/B sources
// 	GPIO_PinAFConfig (tc_set.p_port, tc_set.pin_number[A_channel-1], 
// 					  tc_set.alt_function);
// 	GPIO_PinAFConfig (tc_set.p_port, tc_set.pin_number[B_channel-1], 
// 					  tc_set.alt_function);
// 
// 	// Enable the timer/counter's clock signal
// 	RCC_APB1PeriphClockCmd (tc_set.timer_clock, ENABLE);
// 
// 	// Configure the encoder interface
// 	TIM_EncoderInterfaceConfig (p_timer, TIM_EncoderMode_TI12, 
// 								TIM_ICPolarity_Rising, 
// 								TIM_ICPolarity_Rising);
// 	TIM_SetAutoreload (p_timer, 0xFFFF);
// 
// 	// Turn on the timer/counter being used
// 	TIM_Cmd (p_timer, ENABLE);
//

	// Save a pointer to the timer/counter being used for this object
	p_timer = TIM4;

	// Enable the clock to the GPIO port which is used for the pins
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);

	// Set up the pins used as channel A and B as inputs with pullup resitors on
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit (&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init (GPIOB, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init (GPIOB, &GPIO_InitStruct);

	// Enable the timer/counter's clock signal
	RCC_APB1PeriphClockCmd (RCC_APB1Periph_TIM4, ENABLE);

	// Configure the encoder interface
	TIM_EncoderInterfaceConfig (TIM4, TIM_EncoderMode_TI12, 
								TIM_ICPolarity_Rising, 
								TIM_ICPolarity_Rising);
	TIM_SetAutoreload (TIM4, 0xFFFF);

	// Connect the pins to the alternate functions of encoder A/B sources
	GPIO_PinAFConfig (GPIOB, 6, GPIO_AF_TIM4);
	GPIO_PinAFConfig (GPIOB, 7, GPIO_AF_TIM4);
	
	// Turn on the timer/counter being used
	TIM_Cmd (TIM4, ENABLE);

	// Might as well begin with the software encoder counter set to zero
	zero ();

	QUAD_DBG ("done." << endl);
}


//-------------------------------------------------------------------------------------
/** @brief   Update and get the current count from the quadrature encoder.
 *  @details This method gets a reading of the quadrature encoder's position. First it
 *           calls @c update(), which adds the number of encoder ticks traveled since
 *           the previous update to the current position count. Then it returns the
 *           newly updated count. The return type of @c QUAD_CTR_TYPE corresponds to a
 *           large enough integer to hold any position count; it's specified in 
 *           @c quad_counter.h. 
 *  @return  The current position of the encoder
 */

QUAD_CTR_TYPE quad_counter::get (void)
{
// 	QUAD_DBG ("Quad: " << p_timer << " CNT:" << p_timer->CNT << hex << " CR1:" << p_timer->CR1
// 		<< " CR2:" << p_timer->CR2 << " SMCR:" << p_timer->SMCR << " SR:" << p_timer->SR
// 		<< " OR:" << p_timer->OR << " ARR:" << p_timer->ARR << " CCER:" << p_timer->CCER
// 		<< dec << endl);
// 	QUAD_DBG ("GPIOB: " << bin << GPIOB->IDR << dec << endl);

	update ();
	return (count);
}


//-------------------------------------------------------------------------------------
/** @brief   Update the current position count from change in hardware count. 
 *  @details This method calculates how far the encoder moved since the last time
 *           @c update() was called and adds that distance to the current position
 *           reading. Such a process allows a software count with more bits than the
 *           hardware counter has to be kept. 
 */

void quad_counter::update (void)
{
	if (p_timer)
	{
		int16_t reading = (int16_t)(p_timer->CNT);
		count += reading - previous_reading;
		previous_reading = reading;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Set the current measurement of encoder position to zero. 
 *  @details This method sets the current position measurement to zero. It sets the 
 *           position count to zero, and it also sets the variable which holds a 
 *           previous reading of the hardware counter's value equal to the current 
 *           value in the hardware counter. This will cause changes in position to be
 *           computed from the current position.
 */

void quad_counter::zero (void)
{
	if (p_timer)
	{
		previous_reading = p_timer->CNT;
		count = 0;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Set the count at the current location to the given value
 *  @details This method sets the count of the encoder's current position to the given
 *           value. It also sets the variable which holds a previous reading of the 
 *           hardware counter's value equal to the current value in the hardware 
 *           counter. This will cause changes in position to be computed from the 
 *           current position.
 */

void quad_counter::set (QUAD_CTR_TYPE new_count)
{
	if (p_timer)
	{
		previous_reading = p_timer->CNT;
		count = new_count;
	}
}


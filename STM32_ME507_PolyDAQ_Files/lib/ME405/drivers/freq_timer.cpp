//**************************************************************************************
/** @file    freq_timer.cpp
 *  @brief   Source code for a class to do input capture timing on the STMpunk board.
 *  @details This file contains source code for a class that sets up and runs an input 
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

#include "freq_timer.h"                     // Header for A/D converter driver class


#if (defined FREQ_CTR_USE_TIM1 || defined __DOXYGEN__)
	/** @brief   Array of frequencies measured by Timer 1.
	 *  @details This file-scope static variable holds the frequencies measured by 
	 *           Timer 1 on each of its four channels. Elements 0 through 3 in this
	 *           array hold measurements taken from channels 1 through 4 of the timer.
	 */
	float timer1_freq[4] = {0.0, 0.0, 0.0, 0.0};

	/// Interrupt-accessible storage for Timer 1's clock rate in Hertz.
	uint32_t timer1_clock_rate = 1000000L;

	////////////////////////////////////////////////////////////////
	uint32_t t1_irq_runs = 0;
	////////////////////////////////////////////////////////////////
#endif // FREQ_CTR_USE_TIM1


//--------------------------------------------------------------------------------------
/** @brief   Constructor for a pulse timing class.
 *  @details This constructor configures a timer/counter in the STM32 for use as a 
 *           frequency counter. The timer is turned on, its prescaler is set, and it is
 *           set to count up. The input capture unit is set to detect rising edges. 
 *           The timer's clock rate is configurable to a frequency between the timer
 *           internal clock rate and 1/65535 of the internal clock rate. 
 * 
 *           The base clock rate is controlled in @c system_stm32f4xx.c where the 
 *           main system clock multiplier and peripheral clock dividers are configured.
 *           The standard setup which this file matches is the setup used in the STM32
 *           standard peripheral library for the STM32 Discovery board, with the main
 *           clock at 168 MHz, the AHB prescaler at 1, and the APB2 peripheral clock 
 *           divider at 2, giving a base clock to the timer/counters of 84 MHz. This 
 *           gives a fastest timer update rate of 84 MHz and a slowest rate of about
 *           1.28 KHz. 
 *  @param   timer Pointer to the timer/counter to use, such as @c TIM1
 *  @param   timer_clock The clock signal to activate to use the timer, such as 
 *                       @c RCC_APB2Periph_TIM1
 *  @param   channel The channel for that timer, from 1 to 4 inclusive. This must match
 *                   the pin on the microcontroller to which the signal is conneted
 *  @param   clock_rate The rate at which the timer will run. 
 *                      Default: @c 1000000L for 1 microsecond clock ticks
 */

freq_timer::freq_timer (TIM_TypeDef* timer, uint32_t timer_clock, uint16_t channel,
						uint32_t clock_rate = 1000000L)
{
	// Save pointers to objects given as parameters
	p_timer = timer;

	// Calculate the prescaler value and saturate it so we don't get rollover
	uint32_t prescaler_value = ((uint32_t)(SystemCoreClock / 2) / clock_rate) - 1;
	if (prescaler_value > 0x0000FFFF)
	{
		prescaler_value = 0x0000FFFF;
	}

	// Save the timer's clock rate so the interrupt handler can use it
// 	#ifdef FREQ_CTR_USE_TIM1
// 		if (timer == TIM1)
// 		{
	timer1_clock_rate = clock_rate;
// 		}
// 	#endif

	(void)timer_clock;
	(void)channel;

	// Enable the clock for Timer/Counter 1
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_TIM1, ENABLE);

	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin   = GPIO_Pin_8;
	GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init (GPIOA, &GPIO_InitStruct);

	GPIO_PinAFConfig (GPIOA, GPIO_PinSource8, GPIO_AF_TIM1);

	// Timer/counter initialization. Control with dead zone
	// Timer clock = sysclock / (TIM_Prescaler + 1)
	TIM_TimeBaseInitTypeDef TBI_Struct;
	TBI_Struct.TIM_ClockDivision = TIM_CKD_DIV1; 
	TBI_Struct.TIM_CounterMode = TIM_CounterMode_Up;
	TBI_Struct.TIM_Prescaler = (uint16_t)prescaler_value;
	TBI_Struct.TIM_RepetitionCounter = 0;
	TBI_Struct.TIM_Period = 0xFFFF;
	TIM_TimeBaseInit (TIM1, &TBI_Struct);

	// Initialize the input capture unit
	//Period = (TIM counter clock / TIM output clock) - 1 = 40Hz 
	TIM_ICInitTypeDef IC_InitStruct;
	IC_InitStruct.TIM_Channel = 1;         // channel
	IC_InitStruct.TIM_ICFilter = 0;
	IC_InitStruct.TIM_ICPolarity = TIM_ICPolarity_Rising;
	IC_InitStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	IC_InitStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
	TIM_ICInit (TIM1, &IC_InitStruct);

	// Now that the timer/counter is configured, enable it
	TIM_Cmd (TIM1, ENABLE);

	NVIC_InitTypeDef NVIC_InitStruct;
	NVIC_InitStruct.NVIC_IRQChannel                   = TIM1_CC_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority        = 1;
	NVIC_InitStruct.NVIC_IRQChannelCmd                = ENABLE;
	NVIC_Init (&NVIC_InitStruct);

// 	TIM_ITConfig (TIM1, TIM_IT_CC1, ENABLE);             /////////////////////////////////
}


////////////////////////// Method add_channel()? ///////////////////////////////////////


//--------------------------------------------------------------------------------------
/** @brief   Read a frequency measurement from the given channel of the timer.
 *  @details This method returns the most recently computed measurement of frequency
 *           from the given channel of the timer. 
 *  @param   chan The channel (from 1 to 4) of the timer whose frequency is to be read
 *  @return  The most recently computed frequency, or 0.0 if no measurement has been 
 *           made or the frequency is too slow to measure
 */

float freq_timer::get_freq (uint8_t channel)
{
	// If some luser is trying to read from a nonexistent channel, don't comply
	if (channel >= 4)
	{
		return (0.0);
	}

	// For an extant channel, get the data in a thread-safe fashion because it has been
	// set by an interrupt handler
	float temp_freq;
	portENTER_CRITICAL ();
// 	temp_freq = timer1_freq[channel];
	temp_freq = (float)t1_irq_runs;
	portEXIT_CRITICAL ();
	return (temp_freq);
}


//--------------------------------------------------------------------------------------


#if (defined FREQ_CTR_USE_TIM1 || defined __DOXYGEN__)
//--------------------------------------------------------------------------------------
/** @brief   Timer 1 compare match interrupt handler for frequency counter.
 *  @details This interrupt handler runs when an input capture has been made by Timer 1.
 *           It computes the interval between the current capture and a previous one and
 *           inverts the time difference to find the frequency of the signal. 
 *           The code was found at `https://my.st.com/public/STe2ecommunities/mcu/Lists/
 *           STM32Discovery/Flat.aspx?RootFolder=https%3a%2f%2fmy.st.com%2fpublic%2f
 *           STe2ecommunities%2fmcu%2fLists%2fSTM32Discovery%2fTimer%202%20of%20
 *           STM32F4%20Discovery%20don%27t%20work%20!`
 */

extern "C" void TIM1_CC_IRQHandler (void)
{
	// Previous input capture values for each channel are stored persistently here
// 	static uint16_t prev_captures[4] = {0, 0, 0, 0};

	// Current capture reading
// 	uint16_t current_capture;

	///////////////////////////////////////////////////////////////////////////////////
	t1_irq_runs++;

	if (TIM_GetITStatus (TIM1, TIM_IT_CC1) != RESET)
	{
		TIM_ClearITPendingBit (TIM1, TIM_IT_CC1);
	}
	if (TIM_GetITStatus (TIM1, TIM_IT_CC2) != RESET)
	{
		TIM_ClearITPendingBit (TIM1, TIM_IT_CC2);
	}
	if (TIM_GetITStatus (TIM1, TIM_IT_CC3) != RESET)
	{
		TIM_ClearITPendingBit (TIM1, TIM_IT_CC3);
	}
	if (TIM_GetITStatus (TIM1, TIM_IT_CC4) != RESET)
	{
		TIM_ClearITPendingBit (TIM1, TIM_IT_CC4);
	}
	///////////////////////////////////////////////////////////////////////////////////

	// Check each of the four interrupt sources, which are each triggered by an event
	// on one of the four channels belonging to this timer
// 	for (uint8_t chan = 0; chan < 4; chan++)
// 	{
// 		// Check each channel of the timer; if the interrupt was on that channel, 
// 		// measure the time associated with it
// 		if (TIM_GetITStatus (TIM1, (0x0002 << chan)) == SET)
// 		{
// 			TIM_ClearITPendingBit (TIM1, (0x0002 << chan));
// 			current_capture = *(&(TIM1->CCR1) + chan);
// 			timer1_freq[chan] 
// 					= (float)SystemCoreClock / (current_capture - prev_captures[chan]);
// 			prev_captures[chan] = current_capture;
// 		}
// 	}
}
#endif // FREQ_CTR_USE_TIM1


//**************************************************************************************
/** @file    irq_timer.cpp
 *  @brief   Class that implements an interrupt driven pulse timer on an STMpunk board.
 *  @details This file contains source code for a class that sets up and runs a 
 *           timer/counter in the STM32F4 to measure the rate at which pulses arrive at
 *           an external interrupt pin. It's not as accurate as a timer which uses an
 *           input capture pin, but it's simpler and more flexible, and with the (by
 *           current standards) whopping processing power of the STM32F4 running at 
 *           full speed, the timing is sufficiently accurate for most mechanical
 *           measurements. 
 *
 *  Revisions:
 *    @li 12-17-2014 JRR Original file made for input capture based measurements
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

#include "irq_timer.h"                      // Header for this class is here


// Here we initialize the interrupt based timer's flag which will be set true when the
// timer/counter has been initialized, or perhaps initialised if done in Britain
bool irq_timer::timer_set_up = false;

// Create and initialize static member variables which hold the measured pulse period
#ifdef IRQT_PIN10
	IRQT_CTR_TYPE irq_timer::period_10 = 0;
#endif
#ifdef IRQT_PIN11
	IRQT_CTR_TYPE irq_timer::period_11 = 0;
#endif


//-------------------------------------------------------------------------------------
/** @brief   Constructor for a quadrature encoder reading object. 
 *  @details The constructor initializes the timer/counter and GPIO ports necessary to
 *           read a quadrature encoder. Given which timer/counter to use and which 
 *           channels of that timer/counter to use for the A and B encoder inputs,
 *           this constructor calls code which is mostly supplied by macros to 
 *           initialize the timer/counter, software counter, and GPIO pins. 
 *  @param   port The GPIO port whose pin is used as the input, for example @c GPIOA
 *  @param   pin_number The pin number of the GPIO port used, from 0 to 15
 *  @param   p_ser_dev A pointer to a serial device which is used to print debugging
 *                     messages (only defined if @c IRQT_SER_DBG has been defined)
 */

irq_timer::irq_timer (GPIO_TypeDef* port, uint8_t pin_number
					  #ifdef IRQT_SER_DBG
						  , emstream* p_ser_dev
					  #endif
					 )
{
	// Save the pointer to a serial device which may be used for debugging
	#ifdef IRQT_SER_DBG
		p_serial = p_ser_dev;
	#endif
	IRQT_DBG ("IRQ Timer Constructor...");

	// Save the pin number
	pin_num = pin_number;

	// Save a pointer to the variable which holds the period measured in the interrupt
	// handler. Each pin has its own variable which is shared with the handler
	p_period = NULL;
	switch (pin_num)
	{
		#ifdef IRQT_PIN10
			case (10): p_period = &period_10; break;
		#endif
		#ifdef IRQT_PIN11
			case (11): p_period = &period_11; break;
		#endif
		#ifdef IRQT_PIN12
			case (12): p_period = &period_12; break;
		#endif
		#ifdef IRQT_PIN13
			case (13): p_period = &period_13; break;
		#endif
		#ifdef IRQT_PIN14
			case (14): p_period = &period_14; break;
		#endif
		#ifdef IRQT_PIN15
			case (15): p_period = &period_15; break;
		#endif
		default:
			IRQT_DBG ("Error: No setup defined for pin " << pin_num << ' ');
			break;
	}

	// Set up the timer/counter if it hasn't been set up already
	if (timer_set_up == false)
	{
		timer_set_up = true;

		IRQT_DBG ("Setting up timer at " << IRQ_TIMER_CK_RATE << " Hz...");

		// Enable the timer/counter's clock signal
		IRQ_TIMER_TMCLKCMD (IRQ_TIMER_TMRCLK, ENABLE);

		// Calculate the prescaler value and saturate it so we don't get rollover
		uint32_t prescale = ((uint32_t)(SystemCoreClock / 4) / IRQ_TIMER_CK_RATE) - 1;
		if (prescale > 0x0000FFFF)
		{
			prescale = 0x0000FFFF;
		}
		// Timer clock = sysclock / (TIM_Prescaler + 1)
		TIM_TimeBaseInitTypeDef TBI_Struct;
		TIM_TimeBaseStructInit (&TBI_Struct);
		TBI_Struct.TIM_ClockDivision = TIM_CKD_DIV1; 
		TBI_Struct.TIM_CounterMode = TIM_CounterMode_Up;
		TBI_Struct.TIM_Prescaler = (uint16_t)prescale;
		TBI_Struct.TIM_Period = 0xFFFF;
		TIM_TimeBaseInit (IRQ_TIMER_TIMER, &TBI_Struct);

		// Now that the timer/counter is configured, enable it
		TIM_Cmd (IRQ_TIMER_TIMER, ENABLE);
	}

	// Enable the clock for the GPIO port. Kind of a kludgey way of choosing the
	// port designator, but I'm sick of too many function parameters
	if (port == GPIOA) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	if (port == GPIOB) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
	if (port == GPIOC) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);
	if (port == GPIOD) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);
	if (port == GPIOE) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOE, ENABLE);

	// Enable clock for SYSCFG
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_SYSCFG, ENABLE);

	// Set the GPIO pin as an input
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_Pin = (1 << pin_number);
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init (port, &GPIO_InitStruct);

	// Tell system that we will use the given pin for EXTI_Line N. Sorry, more kludges
	if (port == GPIOA) SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOA, pin_number);
	if (port == GPIOB) SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOB, pin_number);
	if (port == GPIOC) SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOC, pin_number);
	if (port == GPIOD) SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOD, pin_number);
	if (port == GPIOE) SYSCFG_EXTILineConfig (EXTI_PortSourceGPIOE, pin_number);

	// Configure external interrupts, triggering on falling edge only
	EXTI_InitTypeDef EXTI_InitStruct;
	EXTI_InitStruct.EXTI_Line = (1 << pin_number);
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init (&EXTI_InitStruct);

	// Add the IRQ vector to the NVIC.  Lower numbered interrupts have their own
	// vectors; higher numbered ones share an interrupt vector.  The priority is
	// set to something which should be compatible with FreeRTOS
	NVIC_InitTypeDef NVIC_InitStruct;
	if (pin_number == 0)      NVIC_InitStruct.NVIC_IRQChannel = EXTI0_IRQn;
	else if (pin_number == 1) NVIC_InitStruct.NVIC_IRQChannel = EXTI1_IRQn;
	else if (pin_number == 2) NVIC_InitStruct.NVIC_IRQChannel = EXTI2_IRQn;
	else if (pin_number == 3) NVIC_InitStruct.NVIC_IRQChannel = EXTI3_IRQn;
	else if (pin_number == 4) NVIC_InitStruct.NVIC_IRQChannel = EXTI4_IRQn;
	else if (pin_number < 10) NVIC_InitStruct.NVIC_IRQChannel = EXTI9_5_IRQn;
	else                      NVIC_InitStruct.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 0xE0;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0x11;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init (&NVIC_InitStruct);

	// Set the period counter to zero
	period = 0;

	IRQT_DBG ("done." << endl);
}


//-------------------------------------------------------------------------------------
/** @brief   Gets the most recently measured period from the interrupt timer.
 *  @details This method returns the time period, as measured by the timer/counter, 
 *           between the most recent edge detected on the input line and the edge
 *           before that one. 
 *  @return  The time difference between edges of pulses
 */

IRQT_CTR_TYPE irq_timer::get_period (void)
{
	IRQT_CTR_TYPE ret_per;
	portENTER_CRITICAL ();
	ret_per = *p_period;
	portEXIT_CRITICAL ();

	return (ret_per);
}


//-------------------------------------------------------------------------------------
/** @brief   Computes and returns the measured frequency of pulses. 
 *  @details This method calculates and returns the frequency at which pulses are 
 *           arriving at the input pin from the most recently measured time difference
 *           between the arrival of pulse edges. 
 *  @return  The most recent measurement of pulse frequency in Hertz
 */

float irq_timer::get_frequency (void)
{
	IRQT_CTR_TYPE ret_per;                  // Get the period from the global variable
	portENTER_CRITICAL ();
	ret_per = *p_period;
	portEXIT_CRITICAL ();

	return ((float)IRQ_TIMER_CK_RATE / (float)ret_per);
}


//-------------------------------------------------------------------------------------
/** @brief   Interrupt handler for external interrupts on pins 10 - 15 of some port.
 */

extern "C" void EXTI15_10_IRQHandler (void)
{
	// Temporary value used to hold a count taken directly from the timer/counter
	IRQT_CTR_TYPE temp_cnt;

	#ifdef IRQT_PIN10
		static IRQT_CTR_TYPE prev10;
		if (EXTI_GetITStatus (EXTI_Line10) != RESET)    // If pin 10 caused interrupt,
		{
			temp_cnt = IRQ_TIMER_TIMER->CNT;            // Record when it happened
			irq_timer::period_10 = temp_cnt - prev10;   // Compute and save interval
			prev10 = temp_cnt;

			EXTI_ClearITPendingBit (EXTI_Line10);       // Clear interrupt flag
		}
	#endif
	#ifdef IRQT_PIN11
		static IRQT_CTR_TYPE prev11;
		if (EXTI_GetITStatus (EXTI_Line11) != RESET)
		{
			temp_cnt = IRQ_TIMER_TIMER->CNT;
			irq_timer::period_11 = temp_cnt - prev11;
			prev11 = temp_cnt;

			EXTI_ClearITPendingBit (EXTI_Line11);
		}
	#endif
	#ifdef IRQT_PIN12
		if (EXTI_GetITStatus (EXTI_Line12) != RESET)
		{
			EXTI_ClearITPendingBit (EXTI_Line12);
		}
	#endif
	#ifdef IRQT_PIN13
		if (EXTI_GetITStatus (EXTI_Line13) != RESET)
		{
			EXTI_ClearITPendingBit (EXTI_Line13);
		}
	#endif
	#ifdef IRQT_PIN14
		if (EXTI_GetITStatus (EXTI_Line14) != RESET)
		{
			EXTI_ClearITPendingBit (EXTI_Line14);
		}
	#endif
	#ifdef IRQT_PIN15
		if (EXTI_GetITStatus (EXTI_Line15) != RESET)
		{
			EXTI_ClearITPendingBit (EXTI_Line15);
		}
	#endif
}

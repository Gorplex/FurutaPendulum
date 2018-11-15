//**************************************************************************************
/** @file    hw_pwm.cpp
 *  @brief   Source for a class that controls a hardware PWM generator on the STM32.
 *  @details This file contains the source code for a class that sets up and controls a 
 *           timer/counter in PWM mode. 
 *
 *  Revisions:
 *    @li 09-03-2014 JRR Original file for PolyDAQ2 (meow)
 *    @li 11-26-2014 JRR Generic hardware PWM driver made from PolyDAQ's LED driver
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

#include "hw_pwm.h"                         // Header file for this very class


//-------------------------------------------------------------------------------------
/** @brief   Constructor for the STM32 PWM driver.
 *  @details This constructor sets up a driver for a PWM on an STM32 microcontroller.
 *           The specified timer/counter is enabled and set up in PWM mode with the
 *           given prescaler settings and maximum count value. Pins are @b not 
 *           configured in this constructor; the @c setup_pin() function must be used
 *           to configure pins for use as PWM outputs. 
 *  @param   timer_number The number of the timer/counter to use, such as 1 for @c TIM1
 *  @param   frequency The frequency in Hz at which the PWM wave should run
 *  @param   resolution The maximum number to which the PWM timer/counter will count,
 *                      which determines the number of levels the PWM output can have
 */

hw_pwm::hw_pwm (uint8_t timer_number,
				uint32_t frequency,
				uint16_t resolution
			   )
{
	// Save the timer number
	num_timer = timer_number;

	// Save pointers needed by this object
	p_timer = PWM_TMR_SET[num_timer-1].p_timer;

	// Set the maximum allowable PWM value
	max_count = resolution - 1;

    // Enable the clock to the timer which will be used by the PWM
	PWM_TMR_SET[num_timer-1].clock_enable_function 
										(PWM_TMR_SET[num_timer-1].timer_clock, ENABLE);

    // Compute the timer prescaler value. If using the APB1 clock, stick with the 
	// APB1 prescaler of 4; if using the APB2 clock, we need double the prescaling,
	// because the APB2 prescaler itself is only set to 2 in the standard config-
	// uration for which this formula is set up
    uint32_t prescale;
    prescale = (uint16_t)((SystemCoreClock / 4) / (frequency * resolution)) - 1;
	if (timer_number == 1 || (timer_number >= 8 && timer_number <= 11))
	{
		prescale <<= 1;
	}

	// Set up the time base for the timer
    TIM_TimeBaseInitTypeDef TimeBaseStruct;
    TimeBaseStruct.TIM_Period = resolution;
    TimeBaseStruct.TIM_Prescaler = prescale;
    TimeBaseStruct.TIM_ClockDivision = 0;
    TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit (p_timer, &TimeBaseStruct);

	// Enable the timer; this really gets things going
	TIM_Cmd (p_timer, ENABLE);
}


//-------------------------------------------------------------------------------------
/** @brief   Activate a PWM output pin. 
 *  @details This method configures a PWM output pin, using the @c stm32_pwm object as
 *           its signal source. Calls to the pin's @c set_duty_cycle() method will then
 *           control the duty cycle from the pin. The pin must be chosen from the pins
 *           associated with each timer's channels; see the @c Alternate @c function
 *           @c mapping table in the STM32F40X reference manual (Table 9 for Rev 4 of 
 *           the manual). 
 *  @param   port The GPIO port used for the PWM pin, such as @c GPIOB
 *  @param   pin_number The pin designation of the PWM pin, such as @c 8
 *  @param   tim_init_fn Function used to initialize timer/counter's output compare 
 *                       unit, such as @c TIM_OC3Init
 *  @param   tim_preload_fn Function to enable peripheral preload register, such as
 *                          @c TIM_OC3PreloadConfig
 */

void hw_pwm::activate_pin (GPIO_TypeDef* port,
						   uint16_t pin_number,
						   void (*tim_init_fn)(TIM_TypeDef*, TIM_OCInitTypeDef*),
						   void (*tim_preload_fn)(TIM_TypeDef*, uint16_t)
						  )
{
	// Enable the clock to the GPIO port. Use a wacky equation to find the clock
	// number which corresponds to RCC_AHB1Periph_GPIOA, RCC_AHB1Periph_GPIOB, etc.
// 	uint32_t clock_for_port = 
// 		1 << (((uint32_t)(port - GPIOA) / (uint32_t)(GPIOB - GPIOA)) + 1);
// 	RCC_AHB1PeriphClockCmd (clock_for_port, ENABLE);
	// OK, this is an inelegant hack, but it works for now...
	if (port == GPIOA)      RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	else if (port == GPIOB) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
	else if (port == GPIOC) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);
	else if (port == GPIOD) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);

	// Initialize the I/O port pin being used
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = (1 << pin_number);
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init (port, &GPIO_InitStruct);

	// Connect the port pin to its alternate source, which is the PWM
	GPIO_PinAFConfig (port, pin_number, PWM_TMR_SET[num_timer-1].alt_function);

	// Configure the output compare used by the PWM. TIM_Pulse sets inital duty cycle
	TIM_OCInitTypeDef OCInitStruct;
    OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    OCInitStruct.TIM_Pulse = 0;
    OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    tim_init_fn (p_timer, &OCInitStruct);
    tim_preload_fn (p_timer, TIM_OCPreload_Enable);
}


//-------------------------------------------------------------------------------------
/** @brief   Set the duty cycle for the PWM output on one PWM pin.
 *  @details This method sets the duty cycle for a GPIO pin's PWM output. The duty 
 *           cycle must be a number between 0 and the maximum count of the 
 *           timer/counter used by the PWM generator, which was set in the constructor
 *           of the @c stm32_pwm object associated with this pin. 
 *  @param   channel The channel of the timer/counter being used for this PWM output
 *  @param   new_duty_cycle The new duty cycle number to set
 */

void hw_pwm::set_duty_cycle (uint8_t channel, uint16_t new_duty_cycle)
{
	// Make sure the duty cycle isn't out of bounds
	if (new_duty_cycle > max_count)
	{
		new_duty_cycle = max_count;
	}

	// Put the new duty cycle number in the output compare register. Use a bit of a
	// kludge to locate the correct output compare register in the timer's SFR memory
	// (It's still a little more elegant than a bunch of if-then's or a switch/case)
	*(&(p_timer->CCR1) + (channel - 1)) = new_duty_cycle;
}

//**************************************************************************************
/** @file    hw_pwm.h
 *  @brief   Headers for a class that controls a hardware PWM generator on the STM32.
 *  @details This file contains the headers for a class that sets up and controls a 
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

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _HW_PWM_H_
#define _HW_PWM_H_


#include "adc_driver.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"


/** @brief   Holds a timer pointer, its clock source, and clock enable command.
 */
class pwm_timer_set
{
public:
	/** @brief   Pointer to the timer, such as @c TIM1. 
	 */
	TIM_TypeDef* p_timer;

	/** @brief   The clock source for the timer, such as @c RCC_APB2Periph_TIM1.
	 */
	uint32_t timer_clock;

	/** @brief   Pointer to the function used to enable the timer.
	 */
	void (*clock_enable_function)(uint32_t, FunctionalState);

	/** @brief   Alternate function code to enable GPIO pins in PWM mode.
	 */
	uint8_t alt_function;

	/** @brief   Nothing...?
	 */
};


/** @brief   Table of timer numbers, enabling clock sources, and enabling commands.
 *  @details This table holds information needed to set up a timer/counter in PWM
 *           mode and enable PWM pins. Each line is an item of type @c pwm_timer_set.
 *           Data for @c TIM6 and @c TIM7 is zero because those two timers cannot be
 *           used for PWM. 
 */
const pwm_timer_set PWM_TMR_SET[14] =
{
	{TIM1,  RCC_APB2Periph_TIM1,  RCC_APB2PeriphClockCmd, GPIO_AF_TIM1},
	{TIM2,  RCC_APB1Periph_TIM2,  RCC_APB1PeriphClockCmd, GPIO_AF_TIM2},
	{TIM3,  RCC_APB1Periph_TIM3,  RCC_APB1PeriphClockCmd, GPIO_AF_TIM3},
	{TIM4,  RCC_APB1Periph_TIM4,  RCC_APB1PeriphClockCmd, GPIO_AF_TIM4},
	{TIM5,  RCC_APB1Periph_TIM5,  RCC_APB1PeriphClockCmd, GPIO_AF_TIM5},
	{TIM6,                    0,                       0,            0},
	{TIM7,                    0,                       0,            0},
	{TIM8,  RCC_APB2Periph_TIM8,  RCC_APB2PeriphClockCmd, GPIO_AF_TIM8},
	{TIM9,  RCC_APB2Periph_TIM9,  RCC_APB2PeriphClockCmd, GPIO_AF_TIM9},
	{TIM10, RCC_APB2Periph_TIM10, RCC_APB2PeriphClockCmd, GPIO_AF_TIM10},
	{TIM11, RCC_APB2Periph_TIM11, RCC_APB2PeriphClockCmd, GPIO_AF_TIM11},
	{TIM12, RCC_APB1Periph_TIM12, RCC_APB1PeriphClockCmd, GPIO_AF_TIM12},
	{TIM13, RCC_APB1Periph_TIM13, RCC_APB1PeriphClockCmd, GPIO_AF_TIM13},
	{TIM14, RCC_APB1Periph_TIM14, RCC_APB1PeriphClockCmd, GPIO_AF_TIM14}
};


//=====================================================================================
/** @brief   Class which runs a PWM on an STM32 microcontroller. 
 *  @details This class initializes and operates a PWM on an STM32 microcontroller. It
 *           sets up a timer/counter in PWM mode and, when asked, configures a pin to
 *           operate as one of the PWM outputs from that timer/counter. 
 */
class hw_pwm
{
protected:
	/// The number of the timer, from 1 to 14, used by the PWM generator. 
	uint8_t num_timer;

	/// A pointer to the timer/counter used by this PWM generator.
	TIM_TypeDef* p_timer;

	/** @brief   The maximum value to which the timer/counter will count.
	 *  @details This value is the maximum number to which the timer/counter for this
	 *           PWM generator will count before rolling over to zero. It sets the 
	 *           resolution of the PWM generator. 
	 */
	uint16_t max_count;

public:
	// Constructor sets up timer N at frequency F and resolution Q
	hw_pwm (uint8_t timer_number, uint32_t frequency, uint16_t resolution);

	// Method which sets up a GPIO pin for use as a PWM pin
	void activate_pin (GPIO_TypeDef* port, 
					   uint16_t pin,
					   void (*tim_init_fn)(TIM_TypeDef*, TIM_OCInitTypeDef*),
					   void (*tim_preload_fn)(TIM_TypeDef*, uint16_t)
					  );

	// Method which sets the PWM duty cycle for one PWM channel
	void set_duty_cycle (uint8_t channel, uint16_t new_duty_cycle);
};


#endif // _HW_PWM_H_

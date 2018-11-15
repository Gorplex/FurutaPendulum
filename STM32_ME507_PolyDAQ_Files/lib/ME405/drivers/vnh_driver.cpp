//**************************************************************************************
/** @file    vnh_driver.cpp
 *  @brief   Headers for a class that drives motor drivers on the STMpunk board.
 *  @details This file contains the headers for a class that drives the VNH5019 motor
 *           driver chips on the STMpunk board. The drivers are controlled with the PWM
 *           of one of the STM32's timer/counters plus three mode control bits. The
 *           current sense output of the chips can be read by using an A/D converter
 *           driver. 
 *
 *  Revisions:
 *    @li 11-25-2014 JRR Original file
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

#include "hw_pwm.h"                         // Header for hardware PWM used by driver
#include "vnh_driver.h"                     // Header for the vnh_driver class


//-------------------------------------------------------------------------------------
/** @brief   Constructor for a VNH motor driver object.
 *  @details This constructor sets up a VNH motor driver object in a STM32 
 *           microcontroller. The GPIO ports used by pins connected to the driver chip
 *           are turned on and their clocks are enabled; the timer/counter used to
 *           generate PWM signals is enabled and set to PWM mode, and the duty cycle 
 *           is set to zero. @b Note: A PWM driver must have been created and set up 
 *           before this VNH motor driver is constructed, with the PWM driver's output 
 *           channel having been configured with its @c activate_pin() method. 
 *  @param   port_for_INA The GPIO port used for the @c INA pin, such as @c GPIOC
 *  @param   pin_for_INA The number of the pin to which @c INA is connected (0 to 15)
 *  @param   port_for_INB The GPIO port used for the @c INB pin, such as @c GPIOC
 *  @param   pin_for_INB The number of the pin to which @c INB is connected (0 to 15)
 *  @param   port_for_ENDIAG The GPIO port used for the @c EN/DIAG pin
 *  @param   pin_for_ENDIAG The pin number for the @c EN/DIAG pin (from 0 to 15)
 *  @param   PWM_driver A driver which controls the PWM timer and channel
 *  @param   PWM_channel Which channel of the PWM (from 1 - 4) is used
 */

vnh_driver::vnh_driver (GPIO_TypeDef* port_for_INA, uint16_t pin_for_INA, 
						GPIO_TypeDef* port_for_INB, uint16_t pin_for_INB, 
						GPIO_TypeDef* port_for_ENDIAG, uint16_t pin_for_ENDIAG, 
						hw_pwm* PWM_driver, uint8_t PWM_channel
					   )
{
	// Save pointers to GPIO ports to class member data so that the ports can be
	// accessed from any member functions belonging to this class
	INA_port = port_for_INA;
	INB_port = port_for_INB;
	EN_DIAG_port = port_for_ENDIAG;

	// Save bitmasks used to access specific pins on the GPIO ports
	INA_pin_mask = ((uint16_t)1 << pin_for_INA);
	INB_pin_mask = ((uint16_t)1 << pin_for_INB);
	EN_DIAG_pin_mask = ((uint16_t)1 << pin_for_ENDIAG);

	// Save a pointer to the PWM driver and the PWM channel number used
	p_pwm = PWM_driver;
	pwm_channel = PWM_channel;

	// Turn on the clocks for the GPIO ports; we almost always need GPIOA, B, and C
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);

	// Configure the INA and INB pins as push-pull outputs
	// Initialize the I/O port pin being used
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_StructInit (&GPIO_InitStruct);               // Begin with default values
	GPIO_InitStruct.GPIO_Pin = INA_pin_mask;          // Choose pin used by INA
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;        // Set pin as an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;    // Set GPIO port clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;       // Set output type to push-pull
	GPIO_Init (INA_port, &GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = INB_pin_mask;          // For INB, same as INA
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_Init (INB_port, &GPIO_InitStruct);

	// Configure the EN/DIAG pin as an input with a pullup resistor turned on
	GPIO_StructInit (&GPIO_InitStruct);
	GPIO_InitStruct.GPIO_Pin = EN_DIAG_pin_mask;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init (EN_DIAG_port, &GPIO_InitStruct);
}


//-------------------------------------------------------------------------------------
/** @brief   Method to apply torque to the motor at a given PWM level and direction.
 *  @details This method causes the motor driver to be put into clockwise or 
 *           counterclockwise mode and the PWM duty cycle set according to the 
 *           absolute value of the given level. 
 *  @param   pwm_level The absolute value of this number controls PWM duty cycle and
 *                     the sign controls which way the motor torque will be applied
 */

void vnh_driver::actuate (int16_t pwm_level)
{
	// If the PWM level is less than zero, cause 'clockwise' torque
	if (pwm_level < 0)
	{
		INA_port->ODR |= INA_pin_mask;
		INB_port->ODR &= ~INB_pin_mask;
	}
	else
	{
		INA_port->ODR &= ~INA_pin_mask;
		INB_port->ODR |= INB_pin_mask;
	}

	// Set the duty cycle according to the magnitude of the PWM level
	if (pwm_level < 0)
	{
		pwm_level = -pwm_level;
	}
	p_pwm->set_duty_cycle (pwm_channel, pwm_level);
}


//-------------------------------------------------------------------------------------
/** @brief   Method to put the motor in braking mode.
 *  @details This method puts the motor into braking mode, with both its leads shorted
 *           to ground by the motor driver chip when the PWM signal is high. The given
 *           level controls the fraction of the time during which the PWM signal is 
 *           high and hence how strongly the motor will brake. 
 *  @param   pwm_level The duty cycle of the PWM, controlling how strong the braking is
 */

void vnh_driver::brake (uint16_t pwm_level)
{
	INA_port->ODR &= ~INA_pin_mask;
	INB_port->ODR &= ~INB_pin_mask;

	p_pwm->set_duty_cycle (pwm_channel, pwm_level);
}



//**************************************************************************************
/** @file    vnh_driver.h
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

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _VNH_DRIVER_H_
#define _VNH_DRIVER_H_


#include "adc_driver.h"                     // Header for A/D converter driver class
#include "stm32f4xx_gpio.h"                 // Header for GPIO ports
#include "stm32f4xx_tim.h"                  // Header for STM32 counter/timers


//-------------------------------------------------------------------------------------
/** @brief   Class which controls one VNH5019 motor driver chip using an STM32F4. 
 *  @details This is a driver class which controls a VNH5019 motor driver chip such as
 *           one of the motor drivers on an STMpunk board. If multiple VNH5019 chips
 *           are on the board, they work best together if they all use the same 
 *           timer/counter. Using different output compare registers allows different
 *           duty cycles for each PWM, albeit at the same frequency for all. 
 * 
 *           The connections between the microcontroller and the VNH5019 driver on the 
 *           STMpunk board are as follows:
 * 
 *           | STM32F4 Pin | VNH5019 Pin |
 *           | :---------: | :---------: |
 *           | PC5         | ENA/ENB 1   |
 *           | PB2         | INA 1       |
 *           | PC4         | INB 1       |
 *           | PA6/T3Ch1   | PWM 1       |
 *           | PC7         | ENA/ENB 2   |
 *           | PC9         | INA 2       |
 *           | PB12        | INB 2       |
 *           | PA7/T3Ch2   | PWM 2       |
 * 
 *           The name @c T3Ch1 refers to Timer/Counter 3, Channel 1 which is used to
 *           create a PWM signal for the motor driver. 
 */

class vnh_driver
{
protected:
	/// The GPIO port used for the motor driver's INA pin.
	GPIO_TypeDef* INA_port;

	/// Bitmask for the pin used for the motor driver's INA signal.
	uint16_t INA_pin_mask;

	/// The GPIO port used for the motor driver's INB pin.
	GPIO_TypeDef* INB_port;

	/// Bitmask for the pin used for the motor driver's INB signal.
	uint16_t INB_pin_mask;

	/// The GPIO port used for the motor driver's EN/DIAG pins.
	GPIO_TypeDef* EN_DIAG_port;

	/// Bitmask for the pin used for the motor driver's EN/DIAG signal.
	uint16_t EN_DIAG_pin_mask;

	/// A pointer to the PWM driver used for the PWM signal.
	hw_pwm* p_pwm;

	/// The PWM channel on the PWM timer which is used for the PWM signal.
	uint8_t pwm_channel;

public:
	/// The constructor initializes the timer/counter and GPIO ports
	vnh_driver (GPIO_TypeDef* port_for_INA, uint16_t pin_for_INA, 
				GPIO_TypeDef* port_for_INB, uint16_t pin_for_INB, 
				GPIO_TypeDef* port_for_ENDIAG, uint16_t pin_for_ENDIAG, 
				hw_pwm* PWM_driver, uint8_t PWM_channel
			   );

	// Method to apply torque to the motor -- positive one way, negative the other
	void actuate (int16_t pwm_level);

	// Method to put the motor in braking mode
	void brake (uint16_t pwm_level);
};


#endif // _VNH_DRIVER_H_

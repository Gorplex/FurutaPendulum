//**************************************************************************************
/** @file    dac_driver.cpp
 *  @brief   Source code for a very simple D/A converter driver for the STM32.
 *  @details This file contains the source code for a very simple, low performance D/A 
 *           converter driver for the STM32 (at least) microcontroller. This driver is
 *           used for simple problems in which we just need to set the voltage on a
 *           D/A pin to a certain value and leave it that way for a while. For more
 *           complex jobs involving playing audio and the like, one should use drivers
 *           from the Standard Peripheral Library supplied by ST Microelectronics. 
 *
 *  Revisions:
 *    \li 07-24-2014 JRR Original file
 *    \li 08-26-2014 JRR Version made which runs under FreeRTOS
 *
 *  License:
 *		This file is copyright 2014 by JR Ridgely and released under the Lesser GNU 
 *		Public License, version 2. It intended for educational use only, but its use
 *		is not limited thereto. */
/*		THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *		AND	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * 		IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * 		ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 * 		LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 * 		TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * 		OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 * 		CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * 		OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * 		OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//**************************************************************************************

#include "dac_driver.h"                     // The header for this very class


//-------------------------------------------------------------------------------------
/** @brief   Set up a simple DAC driver on an STM32.
 *  @details This constructor sets up a simple DAC driver on an STM32 by powering up
 *           the DAC, enabling the clock to the DAC, setting the DAC for software 
 *           triggering, and enabling both DAC channels. Software triggering is used
 *           because automatic, trigger-less operation didn't work in testing. The 
 *           software trigger is used to transfer the D/A output from its memory mapped
 *           holding register into the register that's connected to the DAC hardware. 
 *  @param   channel_mask A bitmask indicating which DAC channels to enable; it must
 *                        be @c 0x01 for channel 1 only, @c 0x02 for channel 2 only, 
 *                        or @c 0x03 for both DAC channels to be on activated
 *  @param   p_ser_dev    A pointer to a serial device which can be used for debugging;
 *                        it's usually left as NULL, which sets up no serial port
 */

simple_DAC::simple_DAC (uint8_t channel_mask, emstream* p_ser_dev)
{
	p_serial = p_ser_dev;                   // Save pointer to the serial port
	channel_bitmask = channel_mask;         // Save the channel bitmask as well

	// Call the on() method to turn things on
	channels_on (channel_mask);
}


//-------------------------------------------------------------------------------------
/** @brief   Configure a pin for use by the D/A converter.
 *  @details This method configures one pin as an analog output. This configuration is
 *           preferred for use with the DAC because it doesn't interfere with the 
 *           analog signal coming out of the pin nor waste power by doing so.
 *  @param   pin_number The number of the pin to configure
 */

void simple_DAC::set_pin (uint8_t pin_number)
{
	// Create the initialization structure
	GPIO_InitTypeDef GP_Init_Struct;
	GPIO_StructInit (&GP_Init_Struct);

	// Set up pin used for transmission for analog mode
	GP_Init_Struct.GPIO_Pin = (1 << pin_number);
	GP_Init_Struct.GPIO_Mode = GPIO_Mode_AN;
	GPIO_Init (GPIOA, &GP_Init_Struct);
}


//-------------------------------------------------------------------------------------
/** @brief   Activate or deactivate the DAC channels. 
 *  @details This method is used to turn DAC channels on or off. If either DAC channel
 *           is activated, it activates the clock for the DAC and turns the DAC on; if
 *           neither channel is enabled, the DAC is shut off. For each channel which is
 *           activated, the pin used by the DAC is completely taken over for DAC use. 
 *  @param   channel_mask A bitmask specifying which DAQ channels, 1 or 2 or both, are
 *           to be turned on. @c 0x01 specifies channel 1; @c 0x02 specifies channel 2;
 *           @c 0x03 turns on both channels. @c 0x00 turns both channels off. 
 */

void simple_DAC::channels_on (uint8_t channel_mask)
{
	channel_bitmask = channel_mask;         // Save the channel bitmask

	// If the channel mask is zero, turn the whole DAC unit off
	if ((channel_mask & 0x03) == 0)
	{
		// Disable the clock to the DAC
		RCC_APB1PeriphClockCmd (RCC_APB1Periph_DAC, DISABLE);

		// Turn off both channels
		DAC_Cmd (DAC_Channel_1, DISABLE);
		DAC_Cmd (DAC_Channel_2, DISABLE);
	}
	// If the channel mask is nonzero, enable the DAC
	else
	{
		// Enable the clock to the DAC
		RCC_APB1PeriphClockCmd (RCC_APB1Periph_DAC, ENABLE);

		// Set up a DAC initialization structure. The same configuration is used for
		// both channels, which is the default except for software triggering
		DAC_InitTypeDef DAC_InitStruct;
		DAC_StructInit (&DAC_InitStruct);
		DAC_InitStruct.DAC_Trigger = DAC_Trigger_Software;

		// Carry out the initialization of whichever channels are to be used
		if (channel_mask & 0x01)
		{
			DAC_Init (DAC_Channel_1, &DAC_InitStruct);
			set_pin (4);
			DAC_Cmd (DAC_Channel_1, ENABLE);
		}
		else
		{
			DAC_Cmd (DAC_Channel_1, DISABLE);
		}

		if (channel_mask & 0x02)
		{
			DAC_Init (DAC_Channel_2, &DAC_InitStruct);
			set_pin (5);
			DAC_Cmd (DAC_Channel_2, ENABLE);
		}
		else
		{
			DAC_Cmd (DAC_Channel_2, DISABLE);
		}
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Send DAC output on channel 1 or 2. 
 *  @details This method writes the given value into the given digital to analog
 *           converter's output register. It works for either of two DAC channels on 
 *           an STM32 which is equipped with at least two DAC's. If @c channel is any
 *           number other than 1 or 2, nothing happens. If the value is greater than
 *           the DAC can handle, it is saturated at the maximum possible value. 
 *  @param   channel The DAQ channel, 1 or 2, to which data is sent
 *  @param   value   The number 
 */

void simple_DAC::put (uint8_t channel, dac_sample_t value)
{
	if (value > DAC_MAX_OUTPUT)
	{
		value = DAC_MAX_OUTPUT;
	}

	if (channel == 1)
	{
		DAC->DHR12R1 = value & 0x0FFF;
		DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;
	}
	else if (channel == 2)
	{
		DAC->DHR12R2 = value & 0x0FFF;
		DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG2;
	}
}

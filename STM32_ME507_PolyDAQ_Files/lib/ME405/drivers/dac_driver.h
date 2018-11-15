//**************************************************************************************
/** @file    dac_driver.h
 *  @brief   Headers for a very simple D/A converter driver for the STM32.
 *  @details This file contains the headers for a very simple, low performance D/A 
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

/// This define prevents this header from being included in a source file more than once
#ifndef DAC_DRIVER_H
#define DAC_DRIVER_H

#include "stdlib.h"                         // Standard C/C++ stuff
#include "stm32f4xx.h"                      // Has special function register names
#include "stm32f4xx_rcc.h"                  // STM32 clock control header
#include "stm32f4xx_gpio.h"                 // For using general purpose I/O ports
#include "stm32f4xx_dac.h"                  // Definitions for D/A converter
#include "emstream.h"                       // Header for ME405 library output streams


/** @brief   The type of data used to hold samples for the D/A converter. 
 *  @details This typedef gives the type of number used to hold DAC data. It's a 
 *           @c uint16_t when a DAC is used in 10-bit or 12-bit mode; it can be a
 *           @c uint8_t if the DAC is used in 8-bit mode. 
 */
typedef uint16_t dac_sample_t;

/** @brief   Maximum possible value for the input of a D/A conversion.
 *  @details This is the maximum value which can be sent to the D/A converter without
 *           saturating the converter. When a value is sent from the D/A, the @c put()
 *           method will check that value and refuse to send any value higher than 
 *           the one given here. Typical values are @c 1023 for a 10-bit D/A and
 *           @c 4095 for a 12-bit D/A.
 */
const dac_sample_t DAC_MAX_OUTPUT = 4095;


//=====================================================================================
/** @brief   Very simple Digital to Analog Converter (DAC) driver.
 *  @details This class implements a really simple DAC driver for the STM32: It 
 *           always enables both DAC channels, and its only mode of use is to put
 *           numbers into the DAC on command. There's no waveform generation, DMA, or
 *           any other such fancy stuff. A fancy waveform generating DAC driver is 
 *           being worked on for ChibiOS as of July 2014; it should be used for high 
 *           performance applications such as audio. 
 */

class simple_DAC
{
protected:
	/// This pointer allows access to a serial device for debugging
	emstream* p_serial;

	/** @brief   Bitmask indicating which D/A channels are currently activated.
	 *  @details This bitmask indicates the active DAC channels. Bits 0 and 1 indicate
	 *           whether channels 1 and 2 are active, so @c 0x01 indicates channel 1 is
	 *           active, @c 0x02 means channel 2 is on, and @c 0x03 means both channels
	 *           are currently on. Only the first two bits are ever examined, so other
	 *           bits are meaningless and should be left at 0.
	 */
	uint8_t channel_bitmask;

	// This method configures a GPIO pin for use with the D/A converter
	void set_pin (uint8_t pin_number);

public:
	// This constructor initializes and powers up two DAC units on an STM32
	simple_DAC (uint8_t channel_mask, emstream* p_ser_dev = NULL);

	// Turn one, both, or no DAC channels on
	void channels_on (uint8_t channel_mask);

	// This function sends the given value to the given channel
	void put (uint8_t channel, dac_sample_t value);
};


#endif // DAC_DRIVER_H

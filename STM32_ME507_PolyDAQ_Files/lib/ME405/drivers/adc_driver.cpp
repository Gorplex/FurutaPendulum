//*************************************************************************************
/** @file    adc_driver.cpp
 *  @brief   Source code for a simplified A/D driver for the STM32F4xx.
 *  @details This file contains the source for a simplified A/D driver for the STM32F4
 *           (and in the future, others) microcontroller. The STM32 A/D driver supplied
 *           by ST Microelectronics is very complex, as is the STM32 A/D system; this 
 *           simplified driver allows the user to perform more mundane functions (as in
 *           "just get me the voltage on pin @e x") without spending hours to days 
 *           figuring out the driver's intricacies.  The full STM32 driver could be 
 *           used instead of this one when complex A/D scan groups are to be 
 *           programmed, data is to be taken very quickly using DMA, and so on. 
 *
 *  Revisions:
 *    \li 06-19-2014 JRR Data acquisition task (thread) split off from \c main.c
 *    \li 07-29-2014 JRR Trying a version which uses the STM32 library, not ChibiOS
 *    \li 08-25-2014 JRR Porting to FreeRTOS instead of ChibiOS
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
//*************************************************************************************

#include "adc_driver.h"


/** @brief   Buffer which holds A/D conversion results.
 *  @details This buffer holds the results of an A/D conversion or conversion set. 
 *           It's statically allocated with enough space to hold one set of readings 
 *           from all available channels.
 */
static adc_sample_t adc_data_buffer[ADC_NUM_CHANNELS];


//-------------------------------------------------------------------------------------
/** @brief   Constructor for the simplified ADC driver.
 *  @details This constructor sets up the ADC driver for use.  It does not activate
 *           the ADC itself (that is, it doesn't turn on the power or the clock); 
 *           activation is done when the A/D is put into an active mode such as single
 *           conversion mode so that power isn't wasted by leaving the ADC on when 
 *           it's not being used. 
 *  @param   p_ser_port A pointer to a serial device which can be used for debugging, 
 *                      or @c NULL (the default) if debugging is to be turned off
 */

adc_driver::adc_driver (emstream* p_ser_port)
{
	// Save the pointer to the serial port on which we can print debugging information
	p_serial = p_ser_port;

	// Create the mutex which will protect the A/D from multiple calls
	if ((mutex = xSemaphoreCreateMutex ()) == NULL)
	{
		ADC_DBG ("Error: No A/D mutex" << endl);
	}

	// Set the channel list to indicate no data being taken
	channel_list = 0;
}


//-------------------------------------------------------------------------------------
/** @brief   Activate the A/D converter to take one reading from one channel at a time.
 *  @details This method activates the A/D converter by turning its clock on, turning
 *           on the A/D hardware, and configuring the A/D to take single readings.
 */

void adc_driver::single_conversion_mode (void)
{
	// Turn on the clock to the A/D
	RCC_APB2PeriphClockCmd (RCC_APB2Periph_ADC1, ENABLE);

	ADC_CommonInitTypeDef ADC_AllStruct;
	ADC_AllStruct.ADC_Mode = ADC_Mode_Independent;
	ADC_AllStruct.ADC_Prescaler = ADC_Prescaler_Div4;
	ADC_AllStruct.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_AllStruct.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_10Cycles;
	ADC_CommonInit (&ADC_AllStruct);

	// Configure the A/D
	ADC_InitTypeDef ADC_InitStruct;
	ADC_InitStruct.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStruct.ADC_ScanConvMode = ENABLE;
	ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStruct.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
	ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_CC1;
	ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStruct.ADC_NbrOfConversion = 1;
	ADC_Init (ADC1, &ADC_InitStruct);

	// Turn the A/D on
	ADC_Cmd (ADC1, ENABLE);
}


//-------------------------------------------------------------------------------------
/** @brief   Take a single A/D reading from the given channel.
 *  @details This method reads the A/D converter once from the given channel and
 *           returns the result. Such an action is remarkably difficult to do on the
 *           STM32 processors, whose A/D converters are designed for high-speed, 
 *           DMA-based conversions for audio and video processing. 
 */

adc_sample_t adc_driver::read_once (uint8_t channel)
{
	// Make sure the channel number is valid
	if (channel >= ADC_NUM_CHANNELS)
	{
		ADC_DBG ("A/D error: No channel " << channel);
		return ((adc_sample_t)(-1));
	}

	ADC_DBG ("A/D: Pin...");

	// Take the mutex, or wait if another task is using this A/D
	xSemaphoreTake (mutex, portMAX_DELAY);

	// Make sure the correct GPIO port is powered up and its pin set to analog
	set_analog_pin (channel);

	ADC_DBG ("channel...");

	// Configure the channel and its sampling time. Params: Which ADC; channel number;
	// the number in the conversion sequence "Rank"; sample time
	ADC_RegularChannelConfig (ADC1, channel, 1, ADC_SampleTime_56Cycles);

	ADC_DBG ("counts...");

	// Tell the A/D that we're going to be taking just one reading
	ADC_DiscModeChannelCountConfig (ADC1, 1);

	ADC_DBG ("start...");

	// Start the conversion...or maybe enable software-initiated conversions?
	// I think, based on looking at stm32f4xx_adc.c, this causes a start
	ADC_SoftwareStartConv (ADC1);

	// Wait 'til the conversion is done
	for (uint32_t tout = 0; tout < 100000L; tout++)
	{
		if (ADC_GetFlagStatus (ADC1, ADC_SR_EOC) == SET)
		{
			adc_sample_t result = (adc_sample_t)(ADC_GetConversionValue (ADC1));
			xSemaphoreGive (mutex);
			ADC_DBG ("got: " << (int16_t)result << endl);
			return (result);
		}
	}

	// We shouldn't get here
	ADC_DBG ("A/D timeout" << endl);
	xSemaphoreGive (mutex);
	return ((adc_sample_t)(-1));
}


//-------------------------------------------------------------------------------------
/** @brief   Set an A/D pin as an analog input pin and ensure its GPIO port is on.
 *  @details This method figures out which pin to set as an analog input in order to 
 *           use a given A/D converter channel, then sets that pin as an analog input.
 *           It also turns the clock on for the given GPIO port, in case the clock for
 *           that port wasn't already on. 
 *           The pin assignments are as follows: 
 *           \li PA0 ADC123_IN0   PA4 ADC12_IN4   PB0 ADC12_IN8    PC2 ADC123_IN12
 *           \li PA1 ADC123_IN1   PA5 ADC12_IN5   PB1 ADC12_IN9    PC3 ADC123_IN13
 *           \li PA2 ADC123_IN2   PA6 ADC12_IN6   PC0 ADC123_IN10  PC4 ADC12_IN14
 *           \li PA3 ADC123_IN3   PA7 ADC12_IN7   PC1 ADC123_IN11  PC5 ADC12_IN15

 *  @param   channel The channel which is to be configured as an analog input
 */

void adc_driver::set_analog_pin (uint8_t channel)
{
	if (channel <= 7)
	{
		RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
		GPIOA->MODER |= (0x03 << (2 * channel));
	}
	else if (channel <= 9)
	{
		RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
		GPIOB->MODER |= (0x03 << (2 * (channel - 8)));
	}
	else if (channel <= 15)
	{
		RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);
		GPIOC->MODER |= (0x03 << (2 * (channel - 10)));
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Set one channel from which single A/D conversions will be made.
 *  @details This method puts the A/D driver into single reading mode. Then the channel
 *           from which single A/D conversions will come is set. In setting the 
 *           channel, this method not only configures software to use a given channel 
 *           but also configures the appropriate pin as an analog input.
 *  @param   new_channel The channel from which single A/D conversions will be taken
 */

void adc_driver::set_channel (uint8_t new_channel)
{
	// If we're already set to this channel, no need to re-set the channel. 
	if (channel_list == (1 << new_channel))
	{
		return;
	}

	// Call the channel-scan setup function with a bitmask that includes just one
	// channel, unless someone is trying to set an illegal channel, which is refused
	if (new_channel >= ADC_NUM_CHANNELS)
	{
		if (p_serial)
		{
			*p_serial << "A/D error: Illegal channel number " << new_channel;
			return;
		}
	}

	// Call the channel list setting function with just one channel bit set
	set_channel_list (1 << new_channel);
}


//-------------------------------------------------------------------------------------
/** @brief   Set the channels for a set of A/D conversions from a bitmask.
 *  @details This method is used to set up the A/D converter to take measurements from
 *           a set of channels. The measurements will be placed into appropriate spots
 *           in a measurements buffer. To enable measurements from channels 0 and 2, 
 *           for example, use the bitmask @c 0b0000000000000101 or @c 0x0005. The same
 *           mask can be made more readable as <tt>(1 << 0) | (1 << 2)</tt>. 
 *  @param   channels_mask A bitmask in which each channel from which we are to
 *                         measure has a corresponding bit which is one
 */

void adc_driver::set_channel_list (adc_ch_mask_t channels_mask)
{
	(void)channels_mask;

// 	// Set the sequence registers all to zero
// 	adc_group_config.sqr3 = 0;
// 	adc_group_config.sqr2 = 0;
// 	adc_group_config.sqr1 = 0;
// 
// 	// Scan through the channel bitmask, and for each active channel, give it a
// 	// sequence number and configure its pin
// 	adc_ch_mask_t test_mask = 1;
// 	uint32_t num_channels = 0;
// 	for (uint8_t chan_num = 0; chan_num < ADC_NUM_CHANNELS; chan_num++)
// 	{
// 		// If each channel is active...
// 		if (channels_mask & test_mask)
// 		{
// 			*p_serial << endl << "Ch: " << chan_num << endl;
// 	
// 			// Set an entry in the correct sequence register and increment the count
// 			// of active channels
// 			set_SQR_register (chan_num, ++num_channels);
// 
// 			// Setup pin(s) of our MCU as analog inputs
// 			palSetPadMode (ADC1_channel_map[chan_num].port, 
// 						   ADC1_channel_map[chan_num].pin, 
// 						   PAL_MODE_INPUT_ANALOG);
// 		}
// 
// 		// Advance the test bitmask to the next bit in the channel mask
// 		test_mask <<= 1;
// 	}
// 
// 	// Now that we've counted the channels, set the count in the register
// 	adc_group_config.sqr1 = ADC_SQR1_NUM_CH (num_channels);
// 
// 	// Set the register that indicates how many channels are being measured
// 	adc_group_config.circular = FALSE;            // No need for a ring buffer
// 	adc_group_config.num_channels = num_channels; // How many channels to convert
// 	adc_group_config.end_cb = NULL;               // Callback function
// 	adc_group_config.error_cb = NULL;             // No error callback (yet)
// 	adc_group_config.cr1 = 0;
// 	adc_group_config.cr2 = ADC_CR2_SWSTART;       // Started by a software trigger
// 
// 	// Set the sample time for every channel to be the same value for simplicity
// 	adc_group_config.smpr1 = ADC_SMPR_ALL_56_CYC & 00777777777;
// 	adc_group_config.smpr2 = ADC_SMPR_ALL_56_CYC;
// 
// 	// Save the channel list so that if someone asks to re-set the same list, we'll
// 	// know that nothing needs to be done
// 	channel_list = channels_mask;
}


#ifdef __ARMEL__
//-------------------------------------------------------------------------------------
/** @brief   Set an A/D channel sequence register in an STM32.
 *  @details This method sets bits in the A/D channel sequence register in an STM32
 *           processor only. The sequence registers are @c ADC_SQR3, @c ADC_SQR2, and
 *           @c ADC_SQR1, with the first numbers in a conversion sequence being in 
 *           @c ADC_SQR3 and the last (if one does more than 12 conversions in a 
 *           sequence!) in @c ADC_SQR1. This method uses a brute-force technique to
 *           figure out which register gets what number for a given entry in the
 *           channel sequence.
 *  @param   channel The channel number to be converted.
 *  @param   seq_num The number in the sequence (1 for the first conversion, 2 for the
 *           next, and so on). 
 */

void adc_driver::set_SQR_register (uint8_t channel, uint8_t seq_num)
{
	if (channel >= ADC_NUM_CHANNELS || seq_num > 15)
	{
		if (p_serial)
		{
			*p_serial << "A/D Error: No channel " << channel << " or sequence number "
			          << seq_num << endl;
		}
	}

	// If the sequence number is 6 or below, use register ADC_SQR3
	if (seq_num <= 6)
	{
		ADC1->SQR3 |= channel << (5 * (seq_num - 1));
	}
	// Register ADC_SQR2 holds sequence numbers 6 through 11
	else if (seq_num <= 12)
	{
		ADC1->SQR2 |= channel << (5 * (seq_num - 7));
	}
	// Any other sequence number must be from 12 through 15...right? 
	else
	{
		ADC1->SQR1 |= channel << (5 * (seq_num - 13));
	}
}
#endif


//-------------------------------------------------------------------------------------
/** @brief   Get the most recent conversion on the given channel.
 *  @details This operator returns the item in the A/D conversion buffer at the given
 *           address. Assuming that the A/D converter has performed a conversion 
 *           recently, that item is recently converted data for the channel whose 
 *           number is specified.
 *  @param   channel The channel whose results are being requested
 *  @return  The contents of the requested element in the A/D conversion buffer, 
 *           unless the requested channel number is wrong, which gets one (-1) 
 */
adc_sample_t adc_driver::operator[] (uint8_t channel)
{
	if (channel < ADC_NUM_CHANNELS)
	{
		return (adc_data_buffer[channel]);
	}
	else
	{
		return (adc_sample_t(-1));
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Operator which prints the status of an A/D converter.
 *  @details This operator prints the status of the A/D converter by showing what is
 *  in its control and status registers.
 *  @param  a_str A reference to the stream on which the data will be printed
 *  @param  an_adc A reference to the A/D converter object which is being printed
 *  @return A reference to the stream on which the data was just printed
 */

emstream& operator << (emstream& a_str, adc_driver& an_adc)
{
	(void)an_adc;

	a_str << "CR1:   " << bin << ADC1->CR1 << endl;
	a_str << "CR2:   " << ADC1->CR2 << endl;
	a_str << "SMPR1: " << ADC1->SMPR1 << endl;
	a_str << "SMPR2: " << ADC1->SMPR2 << endl;
	a_str << "SQR1:  " << ADC1->SQR1 << endl;
	a_str << "SQR2:  " << ADC1->SQR2 << endl;
	a_str << "SQR3:  " << ADC1->SQR3 << endl;
	a_str << "DR:    " << ADC1->DR << endl << dec;

	return a_str;
}


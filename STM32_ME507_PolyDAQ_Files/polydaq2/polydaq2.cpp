//*************************************************************************************
/** @file polydaq2.cpp
 *    This file contains the headers for a class that interacts with the hardware on a
 *    PolyDAQ 2 board. 
 *
 *  Revisions:
 *    @li 09-03-2014 JRR Original file
 *    @li 03-18-2015 JRR Added code to work with the MMA8452Q accelerometer
 *
 *  License:
 *    This file is copyright 2015 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 3. It intended for educational use only, but its use
 *    is not limited thereto. */
/*    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
 *    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
 *    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUEN-
 *    TIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 *    OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
 *    CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 *    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 *    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */
//*************************************************************************************

#include "polydaq2.h"


//-------------------------------------------------------------------------------------
/** @brief   Create a PolyDAQ 2 interface object.
 *  @details This constructor creates a new PolyDAQ 2 driver object. It saves pointers
 *           and creates new analog to digital and digital to analog converter driver
 *           objects.
 *  @param   serpt A pointer to a serial port that can be used for debugging (default:
 *                 NULL, meaning no debugging messages will be sent).
 */

polydaq2::polydaq2 (emstream* serpt = NULL)
{
	// Save the serial port pointer
	p_serial = serpt;

	// Create an A/D converter driver and start it up
	p_adc = new adc_driver (p_serial);
	p_adc->single_conversion_mode ();

	// Create a D/A converter driver, activating both channels, mid voltage out
	p_dac = new simple_DAC (0x03, p_serial);
	set_DAC (1, DAC_MAX_OUTPUT / 2);
	set_DAC (2, DAC_MAX_OUTPUT / 2);

	// Set up the I2C driver
	p_i2c = new i2c_master (GPIOB, 6, 7, serpt);

	// Set up the MMA8452Q accelerometer on the I2C bus, if it's there
	#ifdef PDQ_MMA8452Q_ADDR
		p_accel = new mma8452q (p_i2c, PDQ_MMA8452Q_ADDR, p_serial);
	#else
		p_accel = NULL;
	#endif
	#ifdef PDQ_EXT_MMA8452Q_ADDR
		p_ext_accel = new mma8452q (p_i2c, PDQ_EXT_MMA8452Q_ADDR, p_serial);
	#else
		p_ext_accel = NULL;
	#endif
}


//-------------------------------------------------------------------------------------
/** @brief   Initialize components which need to be set up after the RTOS is started.
 *  @details This method sets up PolyDAQ 2 components which cannot be set up until the
 *           RTOS has been started. For example, and A/D converter and anything on the
 *           I2C bus can only be accessed with the RTOS running because a mutex is 
 *           used to protect each such component and RTOS delays may be used in some
 *           code. 
 */

void polydaq2::initialize (void)
{
	/////////////////////////////////////////////////////
	p_i2c->scan (p_serial);
	/////////////////////////////////////////////////////

	// Initialize each accelerometer; if either is absent, its working flag goes false
	if (p_accel)
	{
		p_accel->initialize ();
	}
	if (p_ext_accel)
	{
		p_ext_accel->initialize ();
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Get and return raw data from one data channel.
 *  @details This method acquires data from one of the available data channels and
 *           returns it. Channels may include an A/D converter, an accelerometer axis,
 *           or whatever other data source may be configured here.
 *  @param   command A one-character command such as @c '6' to get A/D channel 6 data
 *  @return  The raw data read from the given channel
 */

int16_t polydaq2::get_data (char command)
{
	// If the command is from '0' through 'A', take an A/D conversion
	if (command >= '0' && command <= '9')
	{
		return (read_ADC (command - '0'));
	}
	else if (command >= 'A' && command <= 'F')
	{
		return (read_ADC (command - ('A' - 10)));
	}

	// The characters 'X', 'Y', and 'Z' read accelerations 
	else if (command >= 'X' && command <= 'Z')
	{
		return (get_accel (command - 'X'));
	}

	// The characters 'x', 'y', and 'z' read channels from the external accelerometer
	else if (command >= 'x' && command <= 'z')
	{
		return (get_ext_accel (command - 'x'));
	}

	// Anything else is baloney, so return zero
	else
	{
		return 0;
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Get and return raw data from one data channel.
 *  @details This method acquires data from one of the available data channels and
 *           returns it. Channels may include an A/D converter, an accelerometer axis,
 *           or whatever other data source may be configured here.
 *  @param   command A one-character command such as @c '6' to get A/D channel 6 data
 *  @param   samples The number of readings to average in the oversampling process
 *  @return  The raw data read from the given channel
 */

int16_t polydaq2::get_data_oversampled (char command, uint8_t samples)
{
	int32_t sum = 0;                        // Holds sum of samples before averaging

	for (uint32_t count = samples; count > 0; count--)
	{
		sum += get_data (command);
	}
	return (sum / samples);
}


//-------------------------------------------------------------------------------------
/** @brief   Get a raw A/D reading from a strain gauge amplifier.
 *  @details This method gets an A/D reading from a channel connected to a strain 
 *           gauge bridge amplifier on a PolyDAQ 2. The channels are connected as
 *           follows:
 *           @li Strain gauge bridge 1 - A/D channel 14, pin PC4
 *           @li Strain gauge bridge 2 - A/D channel 15, pin PC5
 *  @param   channel The strain bridge channel to read, either 1 or 2
 *  @return  The unscaled, uncalibrated A/D converter output for the given reading, or
 *           (-1) if the channel requested was invalid
 */

int16_t polydaq2::strain_raw (uint8_t channel)
{
	// If a signal from channel 1 or 2 was asked for, we can do that
	if (channel == 1 || channel == 2)
	{
		return (p_adc->read_once (13 + channel));
	}
	else
	{
		if (p_serial) *p_serial << "Invalid strain channel " << channel;
		return (-1);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Get an oversampled raw A/D reading from a strain gauge amplifier.
 *  @details This method takes a bunch of A/D readings from a channel connected to a
 *           strain gauge bridge amplifier on a PolyDAQ 2, then computes the average
 *           reading and returns it. The channels are connected as follows:
 *           @li Strain gauge bridge 1 - A/D channel 14, pin PC4
 *           @li Strain gauge bridge 2 - A/D channel 15, pin PC5
 *           Oversampling is a conceptually simple way to provide low-pass filtering,
 *           usually used to reduce noise in measurements. 
 *  @param   channel The strain bridge channel to read, either 1 or 2
 *  @param   samples The number of samples to take before computing the average. A
 *                   maximum of 
 *  @return  The unscaled, uncalibrated A/D converter output for the given reading, or
 *           (-1) if the channel requested was invalid
 */

int16_t polydaq2::strain_raw_oversampled (uint8_t channel, uint8_t samples)
{
	int32_t sum = 0;                        // A place to store accumulated readings

	// If a signal from channel 1 or 2 was asked for, we can do that
	if (channel == 1 || channel == 2)
	{
		for (uint8_t count = samples; count > 0; count--)
		{
			sum += p_adc->read_once (13 + channel);
		}
		return (sum / samples);
	}
	else
	{
		if (p_serial) *p_serial << "Invalid strain channel " << channel;
		return (-1);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Turn the D/A's which are used to balance the strain gauge bridges off.
 *  @details This method deactivates the D/A converters attached to the strain gauge 
 *           bridge and leaves the D/A pins set as analog inputs. This means that the 
 *           pins float; they will not affect the voltage on the corners of the 
 *           bridge. This setting saves a little bit of power. 
 */

void polydaq2::strain_balancer_off (void)
{
	p_dac->channels_on (0x00);
}


//-------------------------------------------------------------------------------------
/** @brief   Turn the D/A's which are used to balance the strain gauge bridges on.
 *  @details This method reactivates the D/A;s attached to a strain gauge bridge after
 *           the D/A's were deactivated by a call to @c strain_balancer_off(). 
 */

void polydaq2::strain_balancer_on (void)
{
	p_dac->channels_on (0x03);
}


//-------------------------------------------------------------------------------------
/** @brief   Tolerance within which strain gauge balancing is considered acceptable.
 *  @details When a strain gauge bridge is balanced by using the D/A, it is often not
 *           possible to get the zero-strain amplified bridge output to be exactly the
 *           desired value. This tolerance defines a range within which the balancing
 *           is considered good enough; the amplified output can be higher or lower
 *           than the desired value by this amount and the balance will be considered
 *           good enough.
 */
const uint16_t strain_balance_tolerance = 10;

/** @brief   Number of iterations to make when attempting to balance the bridge.
 *  @details This number specifies how many iterations will be made by the bridge
 *           balancer. The automatic balancing process is a sort of successive
 *           approximation beginning at half the full-scale value, but due to noise
 *           things can get wonky near the end of the approximation process, so we
 *           like to give lots of chances to hit the right value. 
 */
const uint8_t strain_balance_retries = 32;


//-------------------------------------------------------------------------------------
/** @brief   Automatically zero a strain gauge bridge using the D/A converter
 *  @details This method uses the D/A converter in the STM32 to balance ("zero") a
 *           strain gauge bridge. Each strain gauge bridge has a D/A converter output
 *           connected to one side through a large (20K nominal) resistor; the voltage
 *           from the D/A is used to tweak the bridge output so that the zero-strain
 *           reading is as specified in @c desired_set. A typical value for 
 *           @c desired_set is 2047, which is half the full scale for a 12-bit A/D
 *           converter; such a zero setting allows positive and negative strains to be
 *           measured. It is assumed that strain is zero when this method is called. 
 *  @param   channel The strain bridge channel to balance ("zero"), either 1 or 2
 *  @param   desired_set The A/D reading which is desired from the bridge amplifier 
 *           when strain is zero (default value: ADC_MAX_OUTPUT / 2)
 *  @return  The zero-strain reading which was actually achieved; this may be slightly
 *           off the desired value if the system is noisy. If it isn't possible to
 *           balance the bridge, (-1) is returned
 */

int16_t polydaq2::strain_auto_balance (uint8_t channel, uint16_t desired_set)
{
	int16_t adc_reading;                    // Most recent reading from A/D
	int16_t current_DAC;                    // Most recent value sent to the D/A
	int16_t error;                          // Difference between desired and actual
	uint16_t halfhalf = DAC_MAX_OUTPUT / 4; // Amount to try for successive approx.
	uint8_t try_count = 0;                  // Keep track of how many tries are made

	// Only do anything if the channel number is 1 or 2
	if (channel == 1 || channel == 2)
	{
		// Use a sort of successive approximation, beginning at half of full scale
		current_DAC = DAC_MAX_OUTPUT / 2;

		if (p_serial) *p_serial << "Balance: Count, D/A, A/D, Error" << endl;
		do
		{
			// Set the D/A to our best guess and wait a moment
			set_DAC (channel, (uint16_t)(current_DAC));
			vTaskDelay (1);

			// Find how far from the desired value is our current output
			adc_reading = strain_raw_oversampled (channel, 16);
			error = desired_set - adc_reading;

			// Print the current status
			if (p_serial) *p_serial << try_count << ',' << current_DAC
				<< ',' << adc_reading << ',' << error << endl;

			// If we're within tolerance, stop here
			if (error < strain_balance_tolerance && error > -strain_balance_tolerance)
			{
				if (p_serial) *p_serial << "Strain bridge " << channel 
										<< " balanced to " << adc_reading << endl;
				return (adc_reading);
			}

			// The DAC's are wired to opposite sides of the bridges, oops...
			if (channel == 1)
			{
				error = -error;
			}

			// Do the successive approximation thing
			if (error > 0)
			{
				current_DAC += halfhalf;
			}
			else
			{
				current_DAC -= halfhalf;
			}
			if (halfhalf > 1)
			{
				halfhalf /= 2;
			}

			// Saturate the DAC output to the range of values the DAC can handle
			if (current_DAC < 0)
			{
				current_DAC = 0;
			}
			else if (current_DAC > DAC_MAX_OUTPUT)
			{
				current_DAC = DAC_MAX_OUTPUT;
			}
			vTaskDelay (1);
		}
		while (try_count++ < strain_balance_retries);

		if (p_serial) *p_serial << "Error: Can't balance strain bridge "
			<< channel << " to " << desired_set << ", best try " 
			<< adc_reading << endl;
		return (adc_reading);
	}

	// Return something lame if we get here (we shouldn't have; channel was invalid)
	return (-1);
}


//-------------------------------------------------------------------------------------
/** @brief   Get a raw voltage reading from the voltage amplifier.
 *  @details This method returns the output of the A/D converter when it reads the
 *           given voltage channel on the PolyDAQ 2. Voltage channels 1, 2, 3, and 4
 *           on the circuit board are connected to A/D channels 10, 11, 12, and 13 on
 *           the microcontroller. 
 *  @param   channel The channel whose voltage is to be measured 
 *  @return  The unscaled, uncalibrated A/D converter output for the given channel or
 *           (-1) if something went terribly wrong
 */

int16_t polydaq2::voltage_raw (uint8_t channel)
{
	if (channel > 0 && channel < 5)
	{
		return (p_adc->read_once (channel + 9));
	}
	else
	{
		if (p_serial) *p_serial << "Invalid voltage channel " << channel;
		return (-1);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Get a raw temperature reading from a thermocouple amplifier. 
 *  @details This method returns the A/D output when the given thermocouple amplifier
 *           channel is read. 
 *  @param   channel The thermocouple channel to be read, 1 or 2
 *  @return  The unscaled, uncalibrated A/D converter output for the given reading
 */

int16_t polydaq2::temperature_raw (uint8_t channel)
{
	if (channel == 1 || channel == 2)
	{
		return (p_adc->read_once (10 - channel));
	}
	else
	{
		if (p_serial) *p_serial << "Invalid thermocouple channel " << channel;
		return (-1);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Initialize the SD card's LED to be used with PWM for fancy effects.
 *  @details This method initializes the LED which is used to indicate SD card status
 *           (or to indicate something else if desired). The LED is connected to a 
 *           timer and GPIO pin specified in constants that are given in @c polydaq2.h.
 */

void polydaq2::init_SD_card_LED (void)
{
	// Enable the clock to the GPIO port
	RCC_AHB1PeriphClockCmd (SD_LED_CLOCK, ENABLE);

	// Initialize the I/O port pin being used
	GPIO_InitTypeDef GPIO_InitStruct;
	GPIO_InitStruct.GPIO_Pin = SD_LED_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init (SD_LED_PORT, &GPIO_InitStruct);

	// Connect the port pin to its alternate source, which is the PWM
	GPIO_PinAFConfig (SD_LED_PORT, SD_LED_SOURCE, GPIO_AF_TIM4);

    // Enable the clock to the timer which will be used by the PWM
    RCC_APB1PeriphClockCmd (SD_LED_TIMER_CLOCK, ENABLE);

    // Compute the timer prescaler value.  FIXME:  Get that magic 21M out of here
    uint32_t PrescalerValue = (uint16_t)((SystemCoreClock / 2) / 21000000) - 1;

	// Set up the time base for the timer
    TIM_TimeBaseInitTypeDef TimeBaseStruct;
    TimeBaseStruct.TIM_Period = SD_CARD_LED_MAX_PWM;
    TimeBaseStruct.TIM_Prescaler = PrescalerValue;
    TimeBaseStruct.TIM_ClockDivision = 0;
    TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit (SD_LED_TIMER, &TimeBaseStruct);

	// Configure the output compare used by the PWM
	TIM_OCInitTypeDef OCInitStruct;
    OCInitStruct.TIM_OCMode = TIM_OCMode_PWM1;
    OCInitStruct.TIM_OutputState = TIM_OutputState_Enable;
    OCInitStruct.TIM_Pulse = 0;
    OCInitStruct.TIM_OCPolarity = TIM_OCPolarity_High;

    // PWM1 Mode configuration for Channel X (GPIOY Pin Z)
    SD_LED_OC_INIT_FN (SD_LED_TIMER, &OCInitStruct);
    SD_LED_OC_PRNF (SD_LED_TIMER, TIM_OCPreload_Enable);

	// Set the initial duty cycle to zero
	SD_LED_SET_DUTY (SD_LED_TIMER, 0);

	// Enable the timer; this really gets things going
    TIM_Cmd (SD_LED_TIMER, ENABLE);
}


//-------------------------------------------------------------------------------------
/** @brief   Set the brightness of the SD card indicator LED.
 *  @details This method sets the brightness of the LED which is used as an indicator
 *           of SD card activity. This LED may be used as a general purpose indicator
 *           if the SD card is not used. 
 *  @param   brightness The brightness at which the LED will shine, from @c 0 to 
 *                      @c SD_CARD_LED_MAX_PWM (which is usually 1000)
 */

void polydaq2::set_SD_card_LED_brightness (uint16_t brightness)
{
	// Saturate the brightness: higher numbers will map to maximum brightness
	if (brightness > SD_CARD_LED_MAX_PWM)
	{
		brightness = SD_CARD_LED_MAX_PWM;
	}

	SD_LED_SET_DUTY (TIM4, brightness);
}


//-------------------------------------------------------------------------------------
/** @brief   Read one channel of the PolyDAQ's accelerometer.
 *  @details This function reads one channel of the accelerometer, if one is present.
 *           If the PolyDAQ is configured not to use an accelerometer, this method 
 *           returns zero.
 *  @param   channel The accelerometer channel to read: X = 0, Y = 1, and Z = 2
 *  @return  The accelerometer reading from the given channel or 0 if there's no 
 *           accelerometer
 */

int16_t polydaq2::get_accel (uint8_t channel)
{
	if (p_accel && (channel <= 2))
	{
		return (p_accel->get_one_axis (channel));
	}
	return (0);
}


//-------------------------------------------------------------------------------------
/** @brief   Read one channel of an externally attached MMA8452Q accelerometer.
 *  @details This function reads one channel of the external accelerometer, if one is 
 *           present. If the PolyDAQ is configured not to use an accelerometer or there
 *           is no external accelerometer connected, this method returns zero.
 *  @param   channel The accelerometer channel to read: x = 0, y = 1, and z = 2
 *  @return  The accelerometer reading from the given channel or 0 if there's no 
 *           accelerometer there
 */

int16_t polydaq2::get_ext_accel (uint8_t channel)
{
	if (p_accel && (channel <= 2))
	{
		return (p_ext_accel->get_one_axis (channel));
	}
	return (0);
}


//-------------------------------------------------------------------------------------
/** @brief   Query the MLX90614 infrared thermometer for a temperature reading.
 *  @details If a Melexis MLX90614 infrared thermometer is present, this method will
 *           ask the sensor for a reading. 
 *  @return  The temperature measured by the IR sensor in the MLX90614.
 */

uint16_t polydaq2::get_IR_temperature (void)
{
	uint8_t buffer[2];
	buffer[0] = buffer[1] = 0;

	p_i2c->read (PDQ_MLX90614_ADDR, 0x07, buffer, 2);
	return ((uint16_t)(buffer[0]));
}

//**************************************************************************************
/** @file    polydaq2.h
 *  @brief   Headers for a class that drives the PolyDAQ 2 board's hardware.
 *  @details This file contains the headers for a class that interacts with the 
 *           hardware on a PolyDAQ 2 board. The hardware includes the STM32's A/D and
 *           D/A converters, an SD card for data logging, an LED used as a status
 *           indicator, and two serial ports (one USB and one Bluetooth). 
 *
 *  Revisions:
 *    \li 09-03-2014 JRR Original file
 *
 *  License:
 *		This file is copyright 2012 by JR Ridgely and released under the Lesser GNU 
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

// This define prevents this .h file from being included multiple times in a .cpp file
#ifndef _POLYDAQ2_H_
#define _POLYDAQ2_H_


#include "stm32f4xx_tim.h"                  // Header for STM32F4 timer/counters
#include "taskbase.h"                       // Header for Cal Poly ME405 style tasks
#include "adc_driver.h"                     // Driver for the A/D converter
#include "dac_driver.h"                     // Driver for digital to analog converter
#include "sdio_card.h"                      // Header for SD card driver class
#include "i2c_bitbang.h"                    // Driver for I2C bus driver class
#include "mma8452q.h"                       // Driver for MMA8452Q accelerometers


/** @brief   The I2C bus address of a Melexis MLX90614 IR thermometer, if present.
 *  @details This is the I2C bus address at which a Melexis MLX90614 passive IR
 *           thermal sensor may be found, if one is present. The factory default
 *           address is 0x5A.
 */
#define PDQ_MLX90614_ADDR      0x5A

/** @brief   The I2C bus address of the MMA8452Q accelerometer, if one is present.
 *  @details This bus address must be left-shifted by one bit so that the read/write
 *           bit can occupy the least significant bit. Therefore, an MMA8452Q will be
 *           at address 0x3A if it is wired as shown for address 0x1D in the data 
 *           sheets and at address 0x38 if wired as shown for 0x1C. If no MMA8452Q is
 *           in use, just undefine this preprocessor variable. 
 */
#define PDQ_MMA8452Q_ADDR      0x38
// #undef PDQ_MMA8452Q_ADDR

/// @brief   The I2C bus address of the external MMA8452Q accelerometer, if present.
#define PDQ_EXT_MMA8452Q_ADDR  0x3A

/** This constant is used to select the port pin used by the SD card indicator LED.
 *  PolyDAQ 2.0 uses pin 9, while PolyDAQ 2.1 uses pin 8.
 */
const uint16_t SD_LED_PIN = GPIO_Pin_8;

/** This definition specifies which GPIO port is used by the SD card indicator LED.
 */
#define SD_LED_PORT         GPIOB

/** This definition specifies which clock is to be turned on to make the SD card LED's
 *  GPIO port works.
 */
#define SD_LED_CLOCK        RCC_AHB1Periph_GPIOB

/** This macro specifies the pin source used by the SD card indicator LED. It refers
 *  to the pin number.  PolyDAQ 2.0 uses @c GPIO_PinSource9, while PolyDAQ 2.1 uses
 *  @c GPIO_PinSource8. 
 */
#define SD_LED_SOURCE       GPIO_PinSource8

/** This macro specifies the timer used to create the LED for the SD card LED. 
 */
#define SD_LED_TIMER        TIM4

/** This macro specifies which timer clock is used for the SD card LED's PWM timer.
 */
#define SD_LED_TIMER_CLOCK  RCC_APB1Periph_TIM4

/** This macro gives the name of the function used to enable the output compare for a
 *  given PWM. The output number for the given timer is the number in the function 
 *  name, so @c TIM_OC4Init means initialize output compare unit 4 for the timer.
 *  PolyDAQ 2.0 uses @c TIM_OC4Init and PolyDAQ 2.1 uses @c TIM_OC3Init.
 */
#define SD_LED_OC_INIT_FN   TIM_OC3Init

/** This macro specifies the name of the function which enables PWM preload for a
 *  given PWM. The output number is the number in the function name, so 
 *  @c TIM_OC4PreloadConfig means enable preloading of output compare 4 in the timer.
 *  PolyDAQ 2.0 uses output 4, while PolyDAQ 2.1 uses @c TIM_OC3PreloadConfig.
 */
#define SD_LED_OC_PRNF      TIM_OC3PreloadConfig

/** This macro specifies the name of the function used to set the duty cycle of the
 *  SD card LED. The number at the end of the function name is the number of the 
 *  output for the given timer, so @c TIM_SetCompare4 sets the duty cycle of output 
 *  number 4. That works for PolyDAQ 2.0, while PolyDAQ 2.1 needs @c TIM_SetCompare3.
 */
#define SD_LED_SET_DUTY     TIM_SetCompare3


/** @brief   The resolution of the SD card LED's PWM timer.
 *  @details This constant specifies the number to which the SD card indicator LED's 
 *           PWM timer will count. The LED duty cycle, and thus brightness, is 
 *           specified as a number betwen 0 and this number. A higher number here
 *           gives a PWM whose duty cycle can be specified more precisely but which 
 *           runs more slowly with the same prescaler. 
 */
#define SD_CARD_LED_MAX_PWM 1000


/** @brief   The GPIO port used for the SD card sense bit.
 */
#define SD_CRD_SNS_PORT     GPIOB

/** @brief   The GPIO port bit used for the SD card sense switch.
 */
const uint16_t SD_CRD_SNS_PIN = GPIO_Pin_4;

/** @brief   The GPIO port clock used to make the SD card's card sense bit work. 
 */
#define SD_CRD_SNS_CLOCK    RCC_AHB1Periph_GPIOB



//=====================================================================================
/** @brief   Class which interacts with PolyDAQ 2 hardware. 
 *  @details This class contains a set of utility functions which interact with the
 *           hardware on a PolyDAQ 2 board. This includes taking readings of voltage,
 *           temperature and strain; balancing the strain gauge bridges using the D/A
 *           converters; saving data on a MicroSD card; and other things which haven't
 *           been written yet. 
 */

class polydaq2
{
protected:
	/** This pointer allows access to a D/A converter driver created by the PolyDAQ 
	 *  driver.
	 */
	simple_DAC* p_dac;

	/** This pointer points to an A/D converter driver which is created by the PolyDAQ
	 *  driver.
	 */
	adc_driver* p_adc;

	/** This pointer points to a serial port which can be used for debugging. If left
	 *  at @c NULL, debugging printouts won't take place.
	 */
	emstream* p_serial;

	/// @brief   The value most recently sent to each digital to analog channel.
	uint16_t previous_DAC_output[2];

	/// @brief   A pointer to the I2C bus on the PolyDAQ board.
	i2c_master* p_i2c;

	/// @brief   A pointer to the MMA8452Q accelerometer on the board, if there is one.
	mma8452q* p_accel;

	/// @brief   A pointer to the extra external MMA8452Q accelerometer, if present.
	mma8452q* p_ext_accel;

public:
	// The constructor creates the PolyDAQ2 interface object
	polydaq2 (emstream* serpt);

	// Initialize components after FreeRTOS has begun running
	void initialize (void);

	/// Get and return raw data from one data channel
	int16_t get_data (char command);

	/// Get and return oversampled raw data from one data channel
	int16_t get_data_oversampled (char command, uint8_t samples);

	// Get a raw A/D reading from a strain gauge amplifier
	int16_t strain_raw (uint8_t channel);

	// Get a strain gauge amplifier reading oversampled by a number of times
	int16_t strain_raw_oversampled (uint8_t channel, uint8_t samples);

	// Automatically zero a strain gauge bridge using the D/A converter
	int16_t strain_auto_balance (uint8_t channel, 
								 uint16_t desired_set = (ADC_MAX_OUTPUT / 2));

	// Deactivate the strain balancer by turning the D/A's off
	void strain_balancer_off (void);

	// Reactivate the strain balancer by turning the D/A's on
	void strain_balancer_on (void);

	// Get a raw voltage reading from the voltage amplifier
	int16_t voltage_raw (uint8_t channel);

	// Get a raw temperature reading from a thermocouple amplifier
	int16_t temperature_raw (uint8_t channel);

	/** @brief   Read the given channel from the A/D converter.
	 *  @details This method performs an A/D conversion on the given channel of the 
	 *           A/D converter, returning the raw binary number given by the A/D.
	 *  @param   channel The A/D channel to be read, from 0 to 15 (internal STM32
	 *           voltage and temperature channels are not supported)
	 *  @return  The raw binary number returned by the A/D when it read the voltage
	 *           on the given channel
	 */
	int16_t read_ADC (uint8_t channel)
	{
		return (p_adc->read_once (channel));
	}

	/** @brief   Set the output of the given D/A channel to the given value.
	 *  @details This method calls the PolyDAQ 2's D/A converter driver, telling it to
	 *           set the output of one of its channels to the requested value.
	 *  @param   channel The D/A channel whose output is to be set, 1 or 2
	 *  @param   value The value to be put in the D/A's output buffer
	 */
	void set_DAC (uint8_t channel, uint16_t value)
	{
		p_dac->put (channel, value);
		previous_DAC_output[channel-1] = value;
	}

	/** @brief   Get the value most recently sent to the given D/A channel.
	 *  @param   channel The channel, 1 or 2, whose output is to be returned
	 */
	uint16_t get_prev_DAC_output (uint8_t channel)
	{
		return (previous_DAC_output[channel-1]);
	}

	// Initialize the SD card's LED to be used with PWM for fancy effects
	void init_SD_card_LED (void);

	// Set the brightness of the SD card indicator LED
	void set_SD_card_LED_brightness (uint16_t brightness);

	// Get a reading from one of the accelerometer's axes
	int16_t get_accel (uint8_t axis);

	// Get an acceleration reading from the external accelerometer
	int16_t get_ext_accel (uint8_t channel);

	/** @brief   Set the accelerometer's range to +/-2, +/-4, or +/-8 g's.
	 */
	void set_accel_range (mma8452q_range_t range)
	{
		p_accel->set_range (range);
	}

	// Query the Melexis MLX90614 infrared thermometer for a temperature reading
	uint16_t get_IR_temperature (void);

	/** @brief   Check all possible addresses on the I2C bus for sensors.
	 *  @details This method calls the I2C's scan method, which prints a table of I2C
	 *           bus addresses and whether a sensor has been found at each one.
	 */
    void scan_I2C_bus (emstream* p_ser_pt)
	{
		p_i2c->scan (p_ser_pt);
	}
};


//=====================================================================================
/** @mainpage 
 * 
 *  @image html polydaq_logo_4s.png
 * 
 *  @section Introduction
 *      PolyDAQ is a customizable, embeddable data acquisition device for educational 
 *      use. It was originally designed for use in the Cal Poly Mechanical Engineering
 *      Thermal Laboratory, but it has been improved with the intention that it will 
 *      be usable for many different applications. It can function as a data 
 *      acquisition card for a small computer or a standalone datalogger.  All hardware
 *      and software is open source.
 * 
 *  @section Links
 *      @li @subpage pd_setup For PolyDAQ setup of power and data connections
 *      @li @subpage pd_sensors For information about how to connect various types of
 *          sensors
 *      @li @subpage pd_channels For tables which show the commands used to access the
 *          data channels on each PolyDAQ 2 board version
 *      @li @subpage pd_py_gui For a user guide to operating the PolyDAQ GUI, which 
 *          runs on Windows<sup>TM</sup>, Mac<sup>TM</sup>, and Linux<sup>TM</sup>
 *          computers. 
 * 
 *  @section Sensors
 *      PolyDAQ's hardware features include thermocouple, voltage, strain, and 
 *      acceleration inputs. Although traces are present on the board to accommodate 
 *      all of these features, some features may be left off an individual board to 
 *      save money and electric power. In addition, several versions of PolyDAQ have
 *      the same processor and run the same software but are equipped with different
 *      sets of sensors: 
 *      @li Four thermocouple inputs, using AD8494 or AD8495 thermocouple signal 
 *          conditioners for linear amplification and cold junction compensation.
 *      @li Four voltage inputs.  Some voltage inputs use a moderately high input 
 *          resistance voltage divider and an op-amp voltage follower with level 
 *          shifter to allow an input range of -10 to +10 volts. This scheme is similar
 *          to that used in other low-cost DAQ devices such as the USB-600X series 
 *          from National Instruments(tm). Other voltage inputs can be set up without
 *          voltage dividers to give 0-3.3V full-scale measurements. The A/D converters
 *          use a precision 3.3V reference for accuracy.
 *      @li Up to four strain gauge bridge amplifiers whose INA122 instrumentation 
 *          amplifiers allow resolution down to the microstrain in typical Wheatstone 
 *          bridge applications.
 * 
 *  @section Components
 *      Basic components common to all PolyDAQ 2's include the following: 
 *      @li An STM32F4 microcontroller which controls the data acquisition process and
 *          communicates with a PC (if used). 
 *      @li A USB-serial interface which can supply power to the board through the USB
 *          cable as well as communicate between the PolyDAQ and the computer.  Power 
 *          can also be supplied as 5 - 12 volts DC through a standard barrel jack.
 *      @li A Bluetooth serial modem for wireless communication with laptops, tablets, 
 *          or phones (if installed). 
 *      @li A micro-SD card socket. SD cards of up to around 32GB capacity can be used, 
 *          and data is stored in files using the old DOS low-level format. CSV text 
 *          files are the standard file format, but other file formats can be utilized
 *          with small changes to the firmware. Using SDIO card access rather than the 
 *          slower SPI protocol, data has been saved at up to 36KB/s in tests so far, 
 *          and we expect speed improvements as the software is refined.
 *      @li Programmability (to flash updated firmware) through a 6-pin STLink2 connector.
 * 
 *  @section Firmware
 *      The PolyDAQ firmware is based on the STM32 Standard Peripheral Library and 
 *      FreeRTOS.  This software was chosen for its compatibility with software used 
 *      in Cal Poly ME mechatronics courses, which was in turn selected as the 
 *      preferred RTOS environment for educational use.  FreeRTOS isn't known as an 
 *      especially high performance RTOS, but its internal structure is more easily 
 *      understood than those of many competing products - multithreading and device 
 *      access are not as hidden from the application programmer as in many other 
 *      RTOSes, so students can learn more about many important fundamentals.
 *      Performance is not that much of an issue, especially when we have an STM32F4
 *      processor which runs at 168 MHz and has hardware floating point support. 
 * 
 *      GUI applications are being written for use on major PC operating systems 
 *      (Linux, Mac<sup>TM</sup>, Windows<sup>TM</sup>) and if we have good luck in 
 *      porting, Android<sup>TM</sup>.  The applications are written in Python and 
 *      use the Qt GUI libraries, NumPy, and PyQwt.
 * 
 *  @section Version
 *      This page documents PolyDAQ Version 2.1.  As of this writing (March 2015), 
 *      PolyDAQ 2 is a work in progress.  Prototypes have been made and are undergoing
 *      testing, and a production run of a dozen or so boards is being produced. 
 */

#endif // _POLYDAQ2_H_

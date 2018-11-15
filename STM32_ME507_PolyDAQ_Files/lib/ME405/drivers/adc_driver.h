//**************************************************************************************
/** @file    adc_driver.h
 *  @brief   Headers for a simplified A/D driver for the STM32F4xx.
 *  @details This file contains the headers for a simplified A/D driver for the STM32F4
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
//**************************************************************************************

/// This define prevents this header from being included in a source file more than once
#ifndef ADC_DRIVER_H
#define ADC_DRIVER_H

#include "stm32f4xx.h"                      // Has special function register names
#include "stm32f4xx_rcc.h"                // Extra special function timer register 
#include "stm32f4xx_adc.h"                  // Really special registers for the A/D

#include "FreeRTOS.h"                       // Main header for FreeRTOS
#include "task.h"                           // Header for FreeRTOS tasks
#include "queue.h"                          // Header for FreeRTOS queues
#include "semphr.h"                         // Header for FreeRTOS semaphores/mutices

#include "emstream.h"                       // Header for ME405 library output streams


/** @brief   Debugging printout for the A/D converter.
 *  @details If @c ADC_SERIAL_DEBUG is defined as a statement which prints something, 
 *           this function macro writes debugging information to the serial device 
 *           pointed to by @c p_serial, if that serial device pointer points to an 
 *           actual device. If the @c ADC_SERIAL_DEBUG is defined as nothing, this 
 *           macro evaluates to nothing, turning debugging statements off. 
 *  @param   line A line of printable stuff, separated by @c << operators, which 
 *           follows the first @c << in a printing line
 */
// #define ADC_DBG(line) if (p_serial) *p_serial << line
#define ADC_DBG(line)

/** @brief   Type of integer which holds an A/D channel mask.
 *  @details This is an integral type which is just large enough to hold a bitmask 
 *           that specifies which channels are being measured. For an 8-channel A/D,
 *           use a @c uint8_t for example. 
 */
#if (ADC_NUM_CHANNELS <= 8)
	typedef uint8_t adc_ch_mask_t;
#elif(ADC_NUM_CHANNELS <= 16)
	typedef uint16_t adc_ch_mask_t;
#else
	typedef uint32_t adc_ch_mask_t;
#endif

/** @brief   Type of integer which holds samples from the A/D converter.
 *  @details This integer type is just large enough to hold the readings taken by an
 *           A/D converter. It holds the raw, uncalibrated number from the A/D. For 
 *           a 10-bit or 12-bit converter, for example, a 16-bit unsigned integer is
 *           needed.
 */
typedef uint16_t adc_sample_t;

/** @brief   The maximum number which can come from the A/D converter.
 *  @details This is the maximum raw value which the A/D converter can produce. It is
 *           found as (2^N - 1), where N is the number of bits of A/D resolution. For
 *           example, it's 1023 for a 10-bit A/D converter and 4095 for a 12-bit one.
 */
const adc_sample_t ADC_MAX_OUTPUT = 4095;

/** @brief   The number of channels from which we take A/D conversions.
 *  @details This constant is set to the number of channels for which this A/D
 *           driver is set up. For example, on AVR processors it's generally 8, and
 *           on STM32's it's usually 16. The STM32's extra channels for processor
 *           temperature and such aren't usually used with this driver.
 */
#ifdef __AVR
	const uint8_t ADC_NUM_CHANNELS = 8;
#elif __ARMEL__
	const uint8_t ADC_NUM_CHANNELS = 16;
#else
	#error The number of ADC channels is undefined for this processor
#endif

/// This define sets the size of the ADC queue, in elements (not bytes)
const uint8_t ADC_QUEUE_SIZE = 32;

/** @brief   Sampling rate code for the number of clock cycles per A/D sample.
 *  @details This constant sets the number of clock cycles per A/D sample. The 
 *           sampling time is set individually for each channel in the @c ADC_SMPR1
 *           or @c ADC_SMPR2 register. It can be
 *           set to the following values:
 *           @li 0 - 3 cycles
 *           @li 1 - 15 cycles
 *           @li 2 - 28 cycles
 *           @li 3 - 56 cycles
 *           @li 4 - 84 cycles
 *           @li 5 - 112 cycles
 *           @li 6 - 144 cycles
 *           @li 7 - 480 cycles
 */
const uint32_t STM32_ADC_SMP_TM = 0x03;


/** \brief   The type of integer used to hold one A/D sample.
 */
typedef uint16_t adc_sample_t;


/** \brief   Enumeration of codes mixed in with A/D output numbers.
 *  \details This enumeration contains codes that are used to indicate that something
 *           is fishy about A/D readings. Since the A/D's output is an unsigned 12-bit
 *           number but the \c adc_sample_t data type is at least 16 bits in size, 
 *           there are lots of numbers which can't represent a valid A/D conversion. 
 *           We use just a few of those to create codes. 
 */

typedef enum : adc_sample_t
{
	ADC_DR_ERROR = 0xFFFE,          ///< Code to indicate an error during A/D conversion
	ADC_DR_NO_DATA = 0xFFFD,        ///< Code that means no A/D data is present
	ADC_DR_START_DATA = 0xFFEF,     ///< Code that indicates beginning of A/D data set
	ADC_DR_END_DATA = 0xFFEE        ///< Code that indicates the end of an A/D data set
} adc_codes;


//-------------------------------------------------------------------------------------
/** \brief Structure used to access the pin associated with a given A/D channel.
 *  \details This structure is used in an A/D channel map to show the GPIO port and
 *  the pin on the port to which a given A/D channel is connected. It's needed because
 *  one needs to set up the pin associated with a given A/D channel as an analog input.
 */

typedef struct
{
	GPIO_TypeDef* port;                 ///< The GPIO port on which the pin sits
	uint8_t pin;                        ///< The pin number within the port
	uint32_t ADC_SMPR1_data;            ///< Data which will go in register ADC_SMPR1
	uint32_t ADC_SMPR2_data;            ///< Data which will go in register ADC_SMPR2
} adc_channel_stuff;


//-------------------------------------------------------------------------------------
/** \brief Map of the pins used by A/D coverter 1 in an STM32F40X.
 *  \details This table holds a mapping between the A/D converter channel (numbered 
 *  0 through 15) and the pin at which the voltage is measured. Pins are specified 
 *  by giving a port. In ChibiOS, general-purpose I/O ports are named as \c IOPORT1 
 *  for \c GPIOA, whose pins are described as \c PA0, \c PA1, etc., \c IOPORT2 for
 *  \c GPIOB, and so on. The mapping of the A/D channels to pins is as follows for
 *  the STM32F407:
 *  \li PA0  ADC123_IN0     PA4  ADC12_IN4      PB0  ADC12_IN8       PC2  ADC123_IN12
 *  \li PA1  ADC123_IN1     PA5  ADC12_IN5      PB1  ADC12_IN9       PC3  ADC123_IN13
 *  \li PA2  ADC123_IN2     PA6  ADC12_IN6      PC0  ADC123_IN10     PC4  ADC12_IN14
 *  \li PA3  ADC123_IN3     PA7  ADC12_IN7      PC1  ADC123_IN11     PC5  ADC12_IN15
 *  This table also holds values that will be put into the sample time registers
 *  \c ADC_SMPR1 and \c ADC_SMPR2. 
 */

const adc_channel_stuff ADC1_channel_map[] = 
{
	// Port, Pin, SMPR1, SMPR2
	{GPIOA, 0, 0, (STM32_ADC_SMP_TM << 0)},
	{GPIOA, 1, 0, (STM32_ADC_SMP_TM << 3)}, 
	{GPIOA, 2, 0, (STM32_ADC_SMP_TM << 6)}, 
	{GPIOA, 3, 0, (STM32_ADC_SMP_TM << 9)},
	{GPIOA, 4, 0, (STM32_ADC_SMP_TM << 12)}, 
	{GPIOA, 5, 0, (STM32_ADC_SMP_TM << 15)}, 
	{GPIOA, 6, 0, (STM32_ADC_SMP_TM << 18)}, 
	{GPIOA, 7, 0, (STM32_ADC_SMP_TM << 21)},
	{GPIOB, 0, 0, (STM32_ADC_SMP_TM << 24)}, 
	{GPIOB, 1, 0, (STM32_ADC_SMP_TM << 27)}, 
	{GPIOC, 0, (STM32_ADC_SMP_TM << 0), 0}, 
	{GPIOC, 1, (STM32_ADC_SMP_TM << 3), 0},
	{GPIOC, 2, (STM32_ADC_SMP_TM << 6), 0}, 
	{GPIOC, 3, (STM32_ADC_SMP_TM << 9), 0}, 
	{GPIOC, 4, (STM32_ADC_SMP_TM << 12), 0}, 
	{GPIOC, 5, (STM32_ADC_SMP_TM << 15), 0}
};


//-------------------------------------------------------------------------------------
/** \brief Modes in which the A/D converter can be used.
 *  \details This enumeration contains a set of modes for the simplified A/D converter
 *  driver.
 */

typedef enum
{
	ADC_mode_single,                        ///< Mode to get data from just one channel
	ADC_mode_scan_set,                      ///< Mode to scan a set of channels
	ADC_mode_timer_scan_set                 ///< Scan set mode triggered by a timer
} adc_mode;


//-------------------------------------------------------------------------------------
/** @brief   Class which encapsulates a simplified A/D converter driver.
 *  @details This class is a simplified C++ wrapper for an analog to digital converter
 *           (ADC) driver. The A/D drivers which come with RTOS's are usually very 
 *           complex and highly flexible; this driver is intended to be much simpler 
 *           to use. It is provided to ease coding when the highest performance is 
 *           @b not required, but code needs to be written without spending many hours
 *           learning about the details of the RTOS's driver. The RTOS's driver or
 *           custom code and should be used directly via C calls when the highest 
 *           performance is desired.
 * 
 *           @b Warning: In order to use A/D channels, this driver must take over pins
 *           on the microcontroller. Some pins are shared with other functions. For 
 *           example, taking a reading on A/D channel 2 on an STM32F4 shuts off USART2,
 *           which is a commonly used serial port. 
 */

class adc_driver
{
private:

protected:
	/// @brief   Mutex used to prevent simultaneous uses of one A/D converter.
	SemaphoreHandle_t mutex;

	/// This pointer points to the serial port which is used for debugging
	emstream* p_serial;

	/** @brief   Bitmask for the current channel list.
	 *  @details This bitmask has ones for each channel from which data is currently
	 *           to be taken, or zero if data hasn't recently been taken or the 
	 *           channel list has not yet been set. 
	 */
	adc_ch_mask_t channel_list;

	// This method sets a channel sequence register for one conversion in a sequence
	#ifdef __ARMEL__
		void set_SQR_register (uint8_t channel, uint8_t seq_num);
	#endif

public:
	// This constructor creates an A/D controller object but doesn't start the A/D yet
	adc_driver (emstream* p_ser_port = NULL);

	// Function to activate the A/D in one conversion at a time mode
	void single_conversion_mode (void);

	/** @brief   Turn the A/D hardware on.
	 *  @details This method turns on the A/D hardware by activating the clock for
	 *           the A/D and turning the microcontroller's internal A/D power switch
	 *           on.
	 */
	void on (void)
	{
		RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
		ADC1->CR2 |= ADC_CR2_ADON;
	}

	/** @brief   Turn the A/D hardware off.
	 *  @details This method turns off the A/D hardware by deactivating the A/D clock
	 *           in the microcontroller and turning the A/D power switch off. This is
	 *           usually done to save power.
	 */
	void off (void)
	{
		RCC->APB2ENR &= ~RCC_APB2ENR_ADC1EN;
		ADC1->CR2 &= ~ADC_CR2_ADON;
	}

	// A simple method that reads one channel one time
	adc_sample_t read_once (uint8_t channel);

	// Set an A/D's pin to analog input mode
	void set_analog_pin (uint8_t channel);

// 	/** @brief   Set an A/D channel's sample time according to the given code.
// 	 */
// 	void set_sample_time (uint8_t channel, uint8_t samp_tm_code);

	// This method puts the A/D into single-conversion mode on the given channel
	void set_channel (uint8_t new_channel);

	// This method puts the A/D into a mode to convert several channels in one run
	void set_channel_list (adc_ch_mask_t channels_mask);

// 	/** @brief   Start an A/D conversion.
// 	 *  @details This method starts an A/D conversion triggered in the "software 
// 	 *           start" mode rather than by hardware events such as created by a 
// 	 *           timer. The group configuration must previously have been set
// 	 *           and memory for the sample buffer allocated. 
// 	 */
// 	void start_conversion (void)
// 	{
// 		adcStartConversion (p_adc_driver, &adc_group_config, p_sample_buffer, 1);
// 	}

	// This method causes the A/D converter to perform the specified conversion set
	void do_conversion (void);

	// This operator returns the contents of the A/D sample buffer for one channel
	adc_sample_t operator[] (uint8_t channel);
};


// This operator is used to print an A/D's status, generally for debugging use
emstream& operator << (emstream& a_str, adc_driver& an_adc);

#endif // ADC_DRIVER_H

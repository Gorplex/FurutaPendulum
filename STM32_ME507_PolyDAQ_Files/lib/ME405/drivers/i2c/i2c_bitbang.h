//*************************************************************************************
/** @file i2c_bitbang.h
 *    This file contains a base class for classes that use the I2C (also known as TWI)
 *    interface on an AVR. The terms "I2C" (the two means squared) and "TWI" are
 *    essentially equivalent; Philips has trademarked the former, and Atmel doesn't pay
 *    them a license fee, so Atmel chips that meet exactly the same specification are
 *    not allowed to use the "I2C" name, even though everything works the same. 
 *
 *    Note: The terms "master" and "slave" are standard terminology used in the
 *    electronics industry to describe interactions between electronic components only.
 *    The use of such terms in this documentation is made only for the purpose of 
 *    usefully documenting electronic hardware and software, and such use must not be
 *    misconstrued as diminishing our revulsion at the socially diseased human behavior 
 *    which is described using the same terms, nor implying any insensitivity toward
 *    people from any background who have been affected by such behavior. 
 *
 *  Revised:
 *    @li 12-24-2012 JRR Original file, as a standalone HMC6352 compass driver
 *    @li 12-28-2012 JRR I2C driver split off into a base class for optimal reusability
 *    @li 02-12-2015 JRR Version made for STM32F4 processors
 *
 *  License:
 *    This file is copyright 2012-15 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 3. It is intended for educational use only, but its use
 *    is not limited thereto. Portions of this software closely follow that of Tilen
 *    Majerle; see http://stm32f4-discovery.com/2014/05/library-09-i2c-for-stm32f4xx/
 */
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

// This define prevents this file from being included more than once in a *.cpp file
#ifndef _I2C_BITBANG_H_
#define _I2C_BITBANG_H_

// #include <stdlib.h>                         // Standard C/C++ library stuff
#include "stm32f4xx.h"                      // Base STM32F4 stuff
#include "stm32f4xx_rcc.h"                  // Clock control for STM32F4
#include "stm32f4xx_gpio.h"                 // General-purpose I/O pins for STM32F4
#include "stm32f4xx_i2c.h"                  // The STM32 I2C header if needed

#include "FreeRTOS.h"                       // Header for the FreeRTOS system
#include "semphr.h"                         // Header for FreeRTOS semaphores/mutices

#include "emstream.h"                       // Header for base serial devices



// This definition is activated if we want to use a serial device to debug I2C.
// #undef I2C_DBG
#define I2C_DBG(x) if (p_serial) *p_serial << x


//-------------------------------------------------------------------------------------
/** @brief   A simple driver for an I2C (also known as TWI) bus. 
 *  @details It encapsulates basic I2C functionality such as the ability to send and
 *           receive bytes through the I2C bus. Currently only operation of the 
 *           microcontroller as an I2C bus master is supported; this is what's needed 
 *           for the microcontroller to interface with most I2C based sensors. 
 * 
 *           @section STM32 Pins
 *           For the STM32, each of the I2C ports can be configured to use various sets
 *           of pins. For simplicity, this driver is coded to use the following pins:
 * 
 *           Port |  SCL  |  SDA
 *           :---:|:-----:|:-----:
 *           I2C1 |  PB6  |  PB7
 *           I2C2 |  PB10 |  PB11
 *           I2C3 |  PA8  |  PA9
 * 
 *           Due to the design of the STM32 standard peripheral library, these pin
 *           assignments are most easily changed by editing the source code. 
 */ 

class i2c_master
{
protected:
	#ifdef I2C_DBG
		/// @brief   Pointer to a serial port object used for debugging the I2C code.
		emstream* p_serial;
	#endif

	/// @brief   The GPIO port used by the pins which carry SCL and SDA signals.
	GPIO_TypeDef* the_port;

	/// @brief   The number of the pin used for the SCL line
	uint8_t scl_pin;

	/// @brief   The number of the pin used for the SDA line
	uint8_t sda_pin;

	/// @brief   A bitmask for the pin used for the SCL line
	uint16_t scl_mask;

	/// @brief   A bitmask for the pin used for the SDA line
	uint16_t sda_mask;

	// Function which implements a dumb delay loop for I2C bitbang timing
	void dumb_delay (uint16_t howlong);

	// Method to write an I2C address and check for an acknowledgement
	bool write_byte (uint8_t address);

	/// @brief   Mutex used to prevent simultaneous uses of the I2C port.
	SemaphoreHandle_t mutex;

	// Read a byte with or without acknowledgement
	uint8_t read_byte (bool ack);

public:
	// This constructor sets up the driver
	i2c_master (
				GPIO_TypeDef* port, uint8_t SCL_pin, uint8_t SDA_pin
				#ifdef I2C_DBG
					, emstream* = NULL
				#endif
			   );

	// This destructor doesn't exist...psych

	// Function to send a start condition to the bus and set an address
	bool start (void);

	// Function to send a repeated start condition on the bus
	bool restart (void);

	// Function to send a stop condition to the bus
	bool stop (void);

	// Read one byte from the slave
	uint8_t read (uint8_t address, uint8_t reg);

	// Read multiple bytes from the slave
	bool read (uint8_t address, uint8_t reg, uint8_t *data, uint8_t count);

	// Write a single byte to the slave
	bool write (uint8_t address, uint8_t reg, uint8_t data);

	// Write multiple bytes to the slave
	bool write (uint8_t address, uint8_t reg, uint8_t* p_buf, uint8_t count);

	// Check if a device is connected to the I2C bus and talking
	bool ping (uint8_t address);

	// Scan the I2C bus, checking all addresses for an acknowledgement
	void scan (emstream* p_ser);

// 	// Scan a set of registers within a device at one address
// 	void scan_dev (uint8_t address, uint8_t first_reg, uint8_t last_reg, 
// 				   emstream* p_ser);

	/// @brief   Set the SDA line high.
	void sda_high (void) { the_port->BSRRL = sda_mask; }

	/// @brief   Set the SDA line low.
	void sda_low (void) { the_port->BSRRH = sda_mask; }

	/// @brief   Set the SCL line high.
	void scl_high (void) { the_port->BSRRL = scl_mask; }

	/// @brief   Set the SCL line low.
	void scl_low (void) { the_port->BSRRH = scl_mask; }
};

#endif // _I2C_BITBANG_H_


//*************************************************************************************
/** @file i2c_bitbang.cpp
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

#include "FreeRTOS.h"                       // Main header for FreeRTOS
#include "semphr.h"                         // Header for semaphores and mutices
#include "task.h"                           // Needed for the vTaskDelay() function
#include "i2c_bitbang.h"                    // Header for this class


/** @brief   A multiplier used to scale all the I2C delay loops, higher being slower.
 *  @details This multiplier is multiplied by each of the I2C delay loop values to
 *           conveniently scale all of them. It is nominally set to 10 for an STM32F4
 *           running at its full speed of 168 MHz; this gives a clock rate of about
 *           100 KHz. 
 */
#define I2CBB_DEL_MULT      2

/// @brief   The number used for a dumb delay loop waiting for the I2C start to finish.
#define I2CBB_DEL_START     (4 * I2CBB_DEL_MULT)

/// @brief   The number used for a dumb delay loop waiting for the I2C stop to finish.
#define I2CBB_DEL_STOP      (4 * I2CBB_DEL_MULT)

/// @brief   The number used for a dumb delay loop holding the SCL line low.
#define I2CBB_DEL_CKLOW     (3 * I2CBB_DEL_MULT)

/// @brief   The number used for a dumb delay loop holding the SCL line high.
#define I2CBB_DEL_CKHIGH    (3 * I2CBB_DEL_MULT)

/// @brief   The number for a dumb delay loop holding data high before a clock pulse.
#define I2CBB_DEL_SETUP     (5 * I2CBB_DEL_MULT)


//-------------------------------------------------------------------------------------
/** @brief   Create an I2C driver object and initialize the I2C port for it.
 *  @details This constructor creates an I2C driver object. It configures the I/O pins 
 *           for the SDA and SCL lines as open-drain outputs. 
 *  @param   port The GPIO port whose pins are used for the SCL and SDA lines
 *  @param   SCL_pin The number of the pin used for the SCL (serial clock) line
 *  @param   SDA_pin The number of the pin used for the SDA (serial data) line
 *  @param   p_debug_port A serial port, often RS-232, for debugging text 
 *                        (default: NULL)
 */

i2c_master::i2c_master (
						GPIO_TypeDef* port, uint8_t SCL_pin, uint8_t SDA_pin
						#ifdef I2C_DBG
							, emstream* p_debug_port
						#endif
					   )
{
	#ifdef I2C_DBG
		p_serial = p_debug_port;            // Set the debugging serial port pointer
	#endif
	the_port = port;                        // Save a pointer to the GPIO port
	scl_pin = SCL_pin;
	sda_pin = SDA_pin;                      // Save pin numbers as class member data
	scl_mask = (1 << SCL_pin);
	sda_mask = (1 << SDA_pin);              // Save pin bitmasks as member data also

	// Enable the clock for the GPIO port. This is a bit of a kludge but it works...
	if (port == GPIOA) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOA, ENABLE);
	if (port == GPIOB) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOB, ENABLE);
	if (port == GPIOC) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOC, ENABLE);
	if (port == GPIOD) RCC_AHB1PeriphClockCmd (RCC_AHB1Periph_GPIOD, ENABLE);

	// Set up the GPIO port pins as open drain outputs by default
	GPIO_InitTypeDef GP_Init_Struct;
	GP_Init_Struct.GPIO_Mode = GPIO_Mode_OUT;
	GP_Init_Struct.GPIO_OType = GPIO_OType_OD;
	GP_Init_Struct.GPIO_PuPd = GPIO_PuPd_NOPULL;       // Use external 4.7K resistors
	GP_Init_Struct.GPIO_Speed = GPIO_Speed_50MHz;
	GP_Init_Struct.GPIO_Pin = scl_mask | sda_mask;
	GPIO_Init (port, &GP_Init_Struct);

	// Create the mutex which will protect the I2C bus from multiple calls
	if ((mutex = xSemaphoreCreateMutex ()) == NULL)
	{
		I2C_DBG ("Error: No I2C mutex" << endl);
	}

	// Leave the pins sitting at logic 1
	the_port->ODR |= scl_mask | sda_mask;
}


//-------------------------------------------------------------------------------------
/** @brief   Cause a start condition on the I2C bus.
 *  @details This method causes a start condition on the I2C bus. In hardware, a start
 *           condition means that the SDA line is dropped while the SCL line stays 
 *           high. This gets the attention of all the other devices on the bus so that
 *           they will listen for their addresses. @b Important: The SDA and SCL lines
 *           must have been high before this function is called, or it won't work. 
 *  @return  @c true if there was an error, @c false if the I2C start was successful
 */

bool i2c_master::start (void)
{
	// If either the SCL or SDA line is low, we have a problem
	if ((the_port->IDR & scl_mask) == 0)
	{
		I2C_DBG ("<S:SCL>");
		return true;
	}
	if ((the_port->IDR & sda_mask) == 0)
	{
		I2C_DBG ("<S:SDA>");
		return true;
	}

	sda_low ();                             // Drop SDA while leaving SCL high
	dumb_delay (I2CBB_DEL_START);

	scl_low ();                             // Drop the clock so it's ready to pulse
	dumb_delay (I2CBB_DEL_START);

	return false;                           // Things presumably worked OK
}


//-------------------------------------------------------------------------------------
/** @brief   Send a repeated start condition on the I2C bus.
 *  @details This method causes a repeated start condition on the I2C bus. It should
 *           only be used when a session has already been started with @c start(), a 
 *           byte has been transmitted or received, and it's time for more bytes to be
 *           transmitted or received to or from the same device. 
 *  @return  @c true if there was an error, @c false if the restart was successful
 */

bool i2c_master::restart (void)
{
	if ((the_port->IDR & scl_mask) == 0)    // Set clock high to cause a restart
	{
// 		I2C_DBG ("<RS:SCL>");
		scl_high ();
		dumb_delay (I2CBB_DEL_START);
	}

	return (start ());                      // The rest is just following start()
}


//-------------------------------------------------------------------------------------
/** @brief   Send a stop condition onto the I2C bus.
 *  @details This method causes a stop condition on the I2C bus. The stop condition
 *           occurs when the SDA line is raised while the SCL line is high.
 *  @return  @c true if an error occurred or @c false if the stop condition worked OK
 */

bool i2c_master::stop (void)
{
	scl_high ();                            // Raise the clock line
	dumb_delay (I2CBB_DEL_STOP);            // Wait for it to settle
	sda_high ();                            // Now raise data, leaving clock high
	dumb_delay (I2CBB_DEL_STOP);            // Wait a short time

	return (false);                         // Things presumably worked OK
}


//-------------------------------------------------------------------------------------
/** @brief   Read a byte from the I2C slave device.
 *  @details This method reads one byte from the I2C slave device. An acknowledgement
 *           bit (ACK) is sent if parameter @c ack is @c true, while a negative 
 *           acknowledgement (NACK) is sent if @c ack is @c false.  An ACK is usually 
 *           sent when reading a byte of data being sent by the slave device and 
 *           expecting more bytes after this one, while a NACK is usually sent if this
 *           byte is the last one. 
 *  @param   ack @c true if an ACK is to be sent, @c false if a NACK is to be sent
 *  @return  The byte of data which was read from the slave device
 */

uint8_t i2c_master::read_byte (bool ack)
{
	uint8_t got_byte = 0;                   // Byte to be returned by this method

	if ((the_port->IDR & scl_mask))         // Make sure SCL is low as it should be
	{
		I2C_DBG ("<R:SCL>");
		scl_low ();
		dumb_delay (I2CBB_DEL_CKLOW);
	}

	sda_high ();                            // Set data high with clock low
	dumb_delay (I2CBB_DEL_SETUP);

	for (uint8_t mask = 0x80; mask; mask >>= 1)   // For each bit in the byte
	{
		scl_high ();                        // Raise clock, then wait a moment
		dumb_delay (I2CBB_DEL_CKHIGH);

		if (the_port->IDR & sda_mask)       // If data bit is high
		{
			got_byte |= mask;
		}

		scl_low ();                         // Drop clock line low again
		dumb_delay (I2CBB_DEL_CKLOW);
	}

	if (ack)                                // Send ack if asked to, otherwise NACK
	{
		sda_low ();
	}

	scl_high ();                            // Leave clock line high
	dumb_delay (I2CBB_DEL_CKHIGH);
	scl_low ();                             // Drop clock line low again
	dumb_delay (I2CBB_DEL_CKLOW);

	return got_byte;
}


//-------------------------------------------------------------------------------------
/** @brief   Write a byte to the slave device which has already been selected.
 *  @details This method writes a byte to an I2C slave device. 
 *  @param   byte The byte to be written to the device
 *  @return  @c true if an acknowledge bit was detected, @c false if none was seen
 */

bool i2c_master::write_byte (uint8_t byte)
{
	bool ack_bit = false;                   // Acknowledge bit returned by this method

	for (uint8_t bit = 0; bit < 8; bit++)   // For each bit in the address
	{
		dumb_delay (I2CBB_DEL_CKLOW);       // Leave clock low a little while

		if (byte & 0x80)                    // If the current MSB is a one
		{
			sda_high ();                    // SDA goes high
		}
		else                                // else
		{
			sda_low ();                     // SDA goes low
		}

		dumb_delay (I2CBB_DEL_SETUP);       // Wait for data line to settle
		scl_high ();                        // Set clock line high
		dumb_delay (I2CBB_DEL_CKHIGH);      // Leave clock high for a short time
		scl_low ();                         // Drop clock line low again

		byte <<= 1;                         // Shift to next bit
	}

	sda_high ();                            // Release SDA, wait for acknowledgement
	dumb_delay (I2CBB_DEL_SETUP);
	scl_high ();                            // Set clock line high
	dumb_delay (I2CBB_DEL_CKHIGH);          // Leave clock high for a short time
	if ((the_port->IDR & sda_mask) == 0)
	{
		ack_bit = true;                     // Check the acknowledgement bit
	}
	scl_low ();                             // Drop clock line low again
	dumb_delay (I2CBB_DEL_CKLOW);           // Leave clock low a little while

	return ack_bit;
}


//-------------------------------------------------------------------------------------
/** @brief   Read one byte from a slave device on the I2C bus.
 *  @details This method reads a single byte from the device on the I2C bus at the
 *           given address.
 *  @param   address The I2C address for the device. The address must already have 
 *                   been shifted so that it fills the 7 @b most significant bits of 
 *                   the byte. 
 *  @param   reg The register address within the device from which to read
 *  @return  The byte which was read from the device
 */

uint8_t i2c_master::read (uint8_t address, uint8_t reg)
{
	xSemaphoreTake (mutex, portMAX_DELAY);  // Take the mutex or wait for it

	start ();                               // Start the discussion
	if (!write_byte (address) || !write_byte (reg))
	{
		I2C_DBG ("<r:0>");
		xSemaphoreGive (mutex);
		return true;
	}
// 	if (!write_byte (address))              // Write the I2C address of the slave
// 	{
// 		I2C_DBG ("<R:a>");
// 		return 0xFF;
// 	}
// 	if (!write_byte (reg))                  // Write the register in the slave
// 	{
// 		I2C_DBG ("<R:r>");
// 		return 0xFF;
// 	}
	restart ();                             // Repeated start condition
	if (!write_byte (address | 0x01))       // Address with read bit set
	{
		I2C_DBG ("<R:d>");
		xSemaphoreGive (mutex);
		return 0xFF;
	}
	uint8_t data = read_byte (false);       // Read a byte, sending a NACK
	stop ();                                // Stop the conversation

	xSemaphoreGive (mutex);                 // Return the mutex, as we're done
	return (data);
}


//-------------------------------------------------------------------------------------
/** @brief   Read multiple bytes from a slave device on the I2C bus.
 *  @details This method reads multiple bytes from the device on the I2C bus at the
 *           given address.
 *  @param   address The I2C address for the device. The address should already have 
 *                   been shifted so that it fills the 7 @b most significant bits of 
 *                   the byte. 
 *  @param   reg The register address within the device from which to read
 *  @param   p_buffer A pointer to a buffer in which the received bytes will be stored
 *  @param   count The number of bytes to read from the device
 *  @return  @c true if a problem occurred during reading, @c false if things went OK
 */

bool i2c_master::read (uint8_t address, uint8_t reg, uint8_t *p_buffer, uint8_t count)
{
	xSemaphoreTake (mutex, portMAX_DELAY);  // Take the mutex or wait for it

	start ();                               // Start the discussion

	if (!write_byte (address) || !write_byte (reg))
	{
		I2C_DBG ("<R:0>");
		xSemaphoreGive (mutex);
		return true;
	}
	restart ();                             // Repeated start condition
	if (!write_byte (address | 0x01))       // Address with read bit set
	{
		I2C_DBG ("<R:d>");
		xSemaphoreGive (mutex);
		return true;
	}
	for (uint8_t index = count - 1; index; index--)
	{
		*p_buffer++ = read_byte (true);     // Read bytes 
	}
	*p_buffer++ = read_byte (false);        // Last byte requires acknowledgement
	stop ();

	xSemaphoreGive (mutex);                 // Return the mutex, as we're done
	return false;
}


//-------------------------------------------------------------------------------------
/** @brief   Write one byte to a slave device on the I2C bus.
 *  @details This method writes a single byte to the device on the I2C bus at the
 *           given address.
 *  @param   address The I2C address for the device. The address should already have 
 *                   been shifted so that it fills the 7 @b most significant bits of 
 *                   the byte. 
 *  @param   reg The register address within the device to which to write
 *  @param   data The byte of data to be written to the device
 *  @return  @c true if there were problems or @c false if everything worked OK
 */

bool i2c_master::write (uint8_t address, uint8_t reg, uint8_t data)
{
	xSemaphoreTake (mutex, portMAX_DELAY);  // Take the mutex or wait for it

	start ();                               // Start the discussion
	if (!write_byte (address) || !write_byte (reg) || !write_byte (data))
	{
		I2C_DBG ("<w:0>");
		xSemaphoreGive (mutex);
		return true;
	}
// 	if (!write_byte (address))              // Write the I2C address of the slave
// 	{
// 		I2C_DBG ("<W:a>");
// 		return true;
// 	}
// 	if (!write_byte (reg))                  // Write the register in the slave
// 	{
// 		I2C_DBG ("<W:r>");
// 		return true;
// 	}
// 	if (!write_byte (data))                 // Data to be written
// 	{
// 		I2C_DBG ("<R:d>");
// 		return true;
// 	}
	stop ();                                // Stop the conversation

	xSemaphoreGive (mutex);                 // Return the mutex, as we're done
	return false;
}


//-------------------------------------------------------------------------------------
/** @brief   Write a bunch of bytes to a slave device on the I2C bus.
 *  @details This method writes a number of bytes to the device on the I2C bus at the
 *           given address.
 *  @param   address The I2C address for the device. The address should already have 
 *                   been shifted so that it fills the 7 @b most significant bits of 
 *                   the byte. 
 *  @param   reg The register address within the device to which to write
 *  @param   p_buf Pointer to a memory address at which is found the bytes of data to 
 *                 be written to the device
 *  @param   count The number of bytes to be written from the buffer to the device
 *  @return  @c true if there were problems or @c false if everything worked OK
 */

bool i2c_master::write (uint8_t address, uint8_t reg, uint8_t* p_buf, uint8_t count)
{
	xSemaphoreTake (mutex, portMAX_DELAY);  // Take the mutex or wait for it

	start ();                               // Start the discussion
	if (!write_byte (address) || !write_byte (reg))
	{
		I2C_DBG ("<W:0>");
		xSemaphoreGive (mutex);
		return true;
	}
// 	if (!write_byte (address))              // Write the I2C address of the slave
// 	{
// 		I2C_DBG ("<W:a>");
// 		return true;
// 	}
// 	if (!write_byte (reg))                  // Write the register in the slave
// 	{
// 		I2C_DBG ("<W:r>");
// 		return true;
// 	}
	for (uint8_t index = 0; index < count; index++)
	{
		if (!write_byte (*p_buf++))
		{
			I2C_DBG ("<W:" << index << '>');
			xSemaphoreGive (mutex);
			return true;
		}
	}
	stop ();

	xSemaphoreGive (mutex);                 // Return the mutex, as we're done
	return false;
}


//-------------------------------------------------------------------------------------
/** @brief   Check if a device is located at the given address.
 *  @details This method causes an I2C start, then sends the given address and checks
 *           for an acknowledgement. After that, it just sends a stop condition. 
 *  @param   address The I2C address for the device. The address should already have 
 *                   been shifted so that it fills the 7 @b most significant bits of 
 *                   the byte. 
 *  @return  @c true if a device acknowledged the given address, @c false if not
 */

bool i2c_master::ping (uint8_t address)
{
	start ();
	bool found_one = write_byte (address);
	stop ();
	return found_one;
}


//-------------------------------------------------------------------------------------
/** @brief   Scan the I2C bus, pinging each address, and print the results.
 *  @details This handy dandy utility function scans each address on the I2C bus and
 *           prints a display showing the addresses at which devices responded to a 
 *           "ping" with acknowledgement.
 *  @param   p_ser A pointer to a serial device on which the scan results are printed
 */

void i2c_master::scan (emstream* p_ser)
{
	*p_ser << PMS ("   0 2 4 6 8 A C E") << hex << endl;
	for (uint8_t row = 0x00; row < 0x10; row++)
	{
		*p_ser << (uint8_t)row << '0';
		for (uint8_t col = 0; col < 0x10; col += 2)
		{
			p_ser->putchar (' ');
			if (ping ((row << 4) | col))
			{
				p_ser->putchar ('@');
			}
			else
			{
				p_ser->putchar ('-');
			}
		}
		*p_ser << endl;
	}
	*p_ser << dec;
}


// //-------------------------------------------------------------------------------------
// /** @brief   Scan registers in one device on the I2C bus and print the results.
//  *  @details This nifty utility function scans each of the given registers in a device
//  *           at the given address on the I2C bus and prints a display showing the 
//  *           contents of those registers. 
//  *  @param   address The I2C address of the device to be scanned
//  *  @param   first_reg The first register address within the device to be scanned
//  *  @param   last_reg The last register address within the device to be scanned
//  *  @param   p_ser A pointer to a serial device on which the scan results are printed
//  */
// 
// void i2c_master::scan_dev (uint8_t address, uint8_t first_reg, uint8_t last_reg, 
// 					   emstream* p_ser)
// {
// 	*p_ser << "I2C device at 0x" << hex << address << ':' << endl;
// 
// 	for (uint8_t reg = first_reg; reg <= last_reg; reg++)
// 	{
// 		*p_ser << "0x" << reg << ':' << read (address, reg) << endl;
// 	}
// 	*p_ser << dec;
// }


//-------------------------------------------------------------------------------------
/** @brief   Function which implements a dumb delay loop for I2C bitbang timing.
 */

void i2c_master::dumb_delay (uint16_t howlong)
{
	for (volatile uint16_t count = howlong; count; count--)
	{
	}
}

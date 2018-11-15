//*************************************************************************************
/** \file mma8452q.cpp
 *    This file contains a driver class for a Freescale MMA8452Q cheapo acceleromter.
 *    The dumbhead accelerometer can have one of only two possible I2C addresses, 0x1C
 *    or 0x1D. 
 *
 *  Revised:
 *    \li 12-24-2012 JRR Original file, written for a Honeywell HMC6352 compass
 *    \li 04-12-2013 JRR Modified to work with the MMA8452Q acceleromter
 *    \li 08-17-2016 JRR Added code to make it work with MMA8451 accelerometer also
 *
 *  License:
 *    This file is copyright 2013 by JR Ridgely and released under the Lesser GNU 
 *    Public License, version 2. It is intended for educational use only, but its use
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

#include "FreeRTOS.h"                       // Main header for FreeRTOS
#include "task.h"                           // Needed for the vTaskDelay() function
#include "mma8452q.h"                       // Header for this accelerometer driver


//-------------------------------------------------------------------------------------
/** @brief   Create a driver for an MMA8452Q accelerometer and its I2C port.
 *  @details This constructor creates an MMA8452Q driver object. The I2C port is 
 *           initialized and the address of the accelerometer is set to the given
 *           value. The address must already have been shifted left into the 7 @b most 
 *           significant bits of the given byte, so the given address should be 0x38 
 *           or 0x3A if the MMA8452Q's SA0 pin is 0 or 1 respectively. The data sheet
 *           for the MMA8452Q shows the 7-bit address @e before left shifting. 
 *           The method @c initialize() must be called after this constructor and after
 *           the RTOS has been started; it does initial communication and setup with 
 *           accelerometer, using the RTOS for some of its timing. 
 *  @param   p_I2C_drv A pointer to the I2C port driver which will be used
 *  @param   address The I2C bus address at which to find the accelerometer. 
 *  @param   p_dbg_port A serial port, often RS-232, for debugging text 
 *           (default: @c NULL)
 */

mma8452q::mma8452q (i2c_master* p_I2C_drv, uint8_t address
					#ifdef I2C_DBG
						, emstream* p_ser_dev
					#endif
				   )
{
	#ifdef I2C_DBG
		p_serial = p_ser_dev;               // Set the debugging serial port pointer
	#endif

	p_I2C = p_I2C_drv;                      // Save the I2C interface pointer
	i2c_address = address;                  // Set the I2C address to the default value
	working = false;                        // Not working unless it's checked out
}


//-------------------------------------------------------------------------------------
/** @brief   Initialize and check out an MMA8452Q accelerometer.
 *  @details Because the I2C port needs to use an RTOS mutex, we can't talk to the
 *           accelerometer until the RTOS is running. This method is therefore to be
 *           called from initialization code of one task that uses the accelerometer.
 */

void mma8452q::initialize (void)
{
	// Assume there's no working accelerometer present
	working = false;

	// Make sure there's something at the I2C address
	if (p_I2C->ping (i2c_address))
	{
		// Get the "what am I" code: 0x1A for MMA8451 or 0x2A for MMA8452
		uint8_t who_am_i = p_I2C->read (i2c_address, 0x0D);

		if (who_am_i = 0x1A || who_am_i == 0x2A)
		{
			working = true;
			standby ();                         // Set the
			set_range (MMA_RANGE_2g);           // range to the default +/-2g's
			active ();
		}
	}

	if (!working)                           // No good code; accelerometer not working
	{
		working = false;
		I2C_DBG (PMS ("No MMA8452Q at 0x") << hex << i2c_address << dec << endl);
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Put the MMA8452Q into active mode.
 *  @details This method puts the MMA8452Q into active mode by setting the @c ACTIVE 
 *           bit in the @c CTRL_REG1 register. In active mode, the accelerometer takes
 *           data, but its settings cannot be messed with. 
 */

void mma8452q::active (void)
{
	if (working)
	{
		uint8_t ctrl_reg1_value = p_I2C->read (i2c_address, MMA_CTRL_REG1);
		p_I2C->write (i2c_address, MMA_CTRL_REG1, (ctrl_reg1_value | 0x01));
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Put the MMA8452Q into standby mode.
 *  @details This method puts the MMA8452Q into standby mode by clearing the @c ACTIVE 
 *           bit in the @c CTRL_REG1 register. In standby mode, the accelerometer's
 *           settings may be messed with. 
 */

void mma8452q::standby (void)
{
	if (working)
	{
		uint8_t ctrl_reg1_value = p_I2C->read (i2c_address, MMA_CTRL_REG1);
		p_I2C->write (i2c_address, MMA_CTRL_REG1, (ctrl_reg1_value & ~0x01));
	}
}


//-------------------------------------------------------------------------------------
/** @brief   Reset the MMA8452Q.
 *  @details This method performs a software reset on the MMA8452Q by writing a 1 to
 *           the @b RST bit in the @c CTRL_REG2 register. 
 */

void mma8452q::reset (void)
{
	if (working)
	{
		p_I2C->write (i2c_address, MMA_CTRL_REG1, 0x00);
		p_I2C->write (i2c_address, MMA_CTRL_REG2, 0x00);
		p_I2C->write (i2c_address, MMA_CTRL_REG2, 0x40);
	}
}


//-------------------------------------------------------------------------------------
/** This method gets an acceleration reading for one axis of the accelerometer.  The
 *  axis is specified by the function parameter.
 *  @param the_axis The axis to be measured: 0 = X, 1 = Y, 2 = Z
 *  @return The measured acceleration
 */

int16_t mma8452q::get_one_axis (uint8_t the_axis)
{
	union                                   // This union holds the raw data of a
	{                                       // reading from the sensor. It gives us
		uint8_t byte[2];                    // two bytes which we put together to make
		int16_t word;                       // a 16-bit word that will be returned
	}
	raw_data;

	if (!working || (the_axis > 2))         // If no accelerometer is present or an 
	{                                       // invalid axis was selected, return
		return (0x7FFF);                    // an unreasonable value as an error code
	}

	the_axis <<= 1;                         // Read 2 bytes of data for 1 channel
	the_axis++;
	p_I2C->read (i2c_address, the_axis, (uint8_t*)(&(raw_data.word)), 2);

	the_axis = raw_data.byte[1];            // The bytes are in the wrong order as
	raw_data.byte[1] = raw_data.byte[0];    // they're read, so reverse them
	raw_data.byte[0] = the_axis;

	return (raw_data.word);                 // Return the bytes we read, as one word
}


//-------------------------------------------------------------------------------------
/** @brief   Method to set the full-scale acceleration range to +/-2, 4, or 8 g's.
 *  @details Before this method is called, the accelerometer must be put into standby
 *           mode by calling @c standby(). Afterwards, it's usually a good idea to put
 *           the accelerometer in active mode with @c active() so that measurements
 *           can be taken.
 *  @param   range The full-scale range desired for the accelerometer
 */

void mma8452q::set_range (mma8452q_range_t range)
{
	if (working)
	{
		p_I2C->write (i2c_address, MMA_XYZ_DATA_CFG_REG, range);
	}
}


//-------------------------------------------------------------------------------------
/** This overloaded operator writes information about the status of an HMC6352 sensor 
 *  to the serial port. The printout shows the current heading as text, with the 
 *  integer part of the heading, a decimal point, and the fractional part. It is 
 *  assumed that the sensor is in standby mode, so the "A" command is sent to the 
 *  sensor, then the driver waits for the sensor to compute and make available the 
 *  heading. This process means that this \c << operator is quite slow to run. 
 *  @param ser_dev A reference to the serial device on which we're writing information
 *  @param sensor A reference to the sensor object whose status is being written
 *  @return A reference to the same serial device on which we write information.
 *          This is used to string together things to write with "<<" operators
 */

emstream& operator << (emstream& ser_dev, mma8452q& sensor)
{
	ser_dev << PMS ("MMA8452Q: ADDRESS = 0x") << hex << sensor.i2c_address 
			<< PMS (", STATUS = ") << bin 
			<< sensor.p_I2C->read (sensor.i2c_address, 0x00)
			<< PMS (", CTRL_REG1 = ") 
			<< sensor.p_I2C->read (sensor.i2c_address, MMA_CTRL_REG1) 
			<< PMS (", CTRL_REG2 = ") 
			<< sensor.p_I2C->read (sensor.i2c_address, MMA_CTRL_REG2)
			<< PMS (", XYZ_DATA_CFG = ")
			<< sensor.p_I2C->read (sensor.i2c_address, MMA_XYZ_DATA_CFG_REG) << dec;

	return (ser_dev);
}


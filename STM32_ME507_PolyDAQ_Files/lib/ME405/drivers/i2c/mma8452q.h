//*************************************************************************************
/** \file mma8452q.h
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

// This define prevents this file from being included more than once in a *.cpp file
#ifndef _MMA8452Q_H_
#define _MMA8452Q_H_

#include <stdlib.h>                         // Standard C/C++ library stuff
#include "i2c_bitbang.h"                    // Header for I2C (AKA TWI) bus driver
#include "emstream.h"                       // Header for base serial devices


/** This is the I2C bus address of the sensor as used for writing commands and data to
 *  the sensor. Sensors are shipped with a default 7-bit address of 0x1C. This address 
 *  doesn't include the read/write bit, so in the terminology used in the AVR data 
 *  sheets, we form a \c SLA+W number by adding the seventh bit (0) as the LSB.
 */
const uint8_t MMA8452Q_WRITE_ADDRESS = (0x1D << 1);


/// @brief   The register address of @c CTRL_REG1 within the MMA8452Q accelerometer.
#define MMA_CTRL_REG1       0x2A

/// @brief   The register address of @c CTRL_REG2 within the MMA8452Q accelerometer.
#define MMA_CTRL_REG2       0x2B

/// @brief   The register address of @c XYZ_DATA_CFG within the MMA8452Q.
#define MMA_XYZ_DATA_CFG_REG  0x0E

/** @brief   Values of the two-bit number used to set the MMA8452Q's full scale range.
 */
enum mma8452q_range_t 
{
	MMA_RANGE_2g = 0x00,   ///< @brief A code to set the range of the MMA8452Q to +/-2g
	MMA_RANGE_4g = 0x01,   ///< @brief A code to set the range of the MMA8452Q to +/-4g
	MMA_RANGE_8g = 0x02    ///< @brief A code to set the range of the MMA8452Q to +/-8g
};


//-------------------------------------------------------------------------------------
/** @brief   Driver for an MMA8452Q accelerometer on an I2C bus.
 *  @details This class is a driver for a Freescale MMA8452Q accelerometer. It's very 
 *           basic and does not use all the MMA8452Q's functions; it is set up only to 
 *           grab acceleration data from the X, Y, and Z channels using the I2C bus. 
 * 
 *  @section use_mma8452q Usage
 *           To use this driver, one need only create the driver object, put the
 *           accelerometer into active mode using @c active(), and then ask it for a 
 *           reading now and then. 
 *           \code
 *           mma8452q* p_sheep = new mma8452q (my_I2C_driver, p_serial);
 *           p_sheep->active ();
 *           ...
 *           *p_serial << PMS ("Y Axis: ") << p_sheep->get_one_axis (1) << endl;
 *           \endcode
 */

class mma8452q
{
protected:
	/// @brief   Pointer to the I2C port driver used for this accelerometer.
	i2c_master* p_I2C;

	#ifdef I2C_DBG
		/// @brief Pointer to a serial device used for showing debugging information.
		emstream* p_serial;
	#endif

	/** This is the 8-bit I2C address for the MMA8452Q sensor. This number contains the
	 *  7-bit I2C address (either 0x1C or 0x1D) as bits 7:1, and a 0 for the least
	 *  significant bit. This is a SLA+W address as described in the AVR datasheets. To
	 *  make a read address SLA+R, just or this number with 0x01. 
	 */
	uint8_t i2c_address;

	/// @brief   Flag which indicates a working MMA8452Q is at the given I2C address.
	bool working;

public:
	// This constructor sets up the driver
	mma8452q (
			  i2c_master* p_I2C_drv, uint8_t address
			  #ifdef I2C_DBG
				  , emstream* p_ser_dev = NULL
			  #endif
			 );

	/** This method sets the I2C address of the device.
	 *  @param new_addr The new I2C address to be set
	 */
	void set_i2c_address (uint8_t new_addr)
	{
		i2c_address = new_addr;
	}

	// Set up the accelerometer in a default mode
	void initialize (void);

	// Put the MMA8452Q into active mode by setting bit ACTIVE in register CTRL_REG1
	void active (void);

	// Put the MMA8452Q into standby mode so that its settings may be adjusted
	void standby (void);

	// Reset the accelerometer using its software reset functionality
	void reset (void);

	// This method reads the current acceleration in one direction: 0 = X, 1 = Y, 2 = Z
	int16_t get_one_axis (uint8_t);

	// Method to set the acceleration range to 2, 4, or 8 g's
	void set_range (mma8452q_range_t range);

	/** @brief   Tells whether there is a working MMA8452Q attached to this driver.
	 *  @return  @c true if there's a working MMA8452 or @c false if not
	 */
	bool is_working (void)
	{
		return working;
	}

	// Give access to printing operator
	friend emstream& operator << (emstream&, mma8452q&);
};

// This operator "prints" a _MMA8452Q_H_ sensor by showing its current measured outputs
emstream& operator << (emstream&, mma8452q&);

#endif // _MMA8452Q_H_


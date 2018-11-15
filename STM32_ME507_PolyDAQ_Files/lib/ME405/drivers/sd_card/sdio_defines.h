//*************************************************************************************
/** @file    sdio_defines.h
 *  @brief   Application specific adjustments for the SD card driver.
 *  @details This file defines parameter(s) which are used to adjust the operation of
 *           the SDIO card driver for a specific system. The locations of the card 
 *           detect and write protect pins are specified, as is the SDIO mode (one or
 *           four data bits transferred at a time). 
 * 
 *  Revisions:
 *    \li ??-??-2013 STM File created by the ST Microelectronics folks
 *    \li ??-??-2013 TMJ Appears to have been modified for Majerle's projects
 *    \li 10-16-2014 JRR Modified and commented by JR Ridgely
 *
 *  License:
 *    This file is copyright 2014 by JR Ridgely, whose contributions are released 
 *    under the Lesser GNU Public License, version 3. This software is intended for 
 *    educational use only, but its use is not limited thereto. The ELM-FAT-FS package
 *    on which this program relies is subject to its own free software license, which
 *    is nearly unrestricted; see the ELM-FAT-FS link above for details about the 
 *    license governing FatFS use. Software from the ST Microelectronics STM32 
 *    Standard Peripheral Library is used, and users must abide by that license in 
 *    using this software. In addition, software by Tilen Majerle is included; 
 *    Majerle's components are licensed under the GNU General Public License, version 
 *    3, which is more restrictive than other licenses which apply to this code. A 
 *    result of all these licenses is that this software is free software, and any 
 *    software and/or systems in which it is used (derivative works) must also be free
 *    software and systems, and anyone who uses or distributes such software or 
 *    systems must make this software freely available to users or customers. */
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

/** @brief   Whether to use the SD card's card detect pin.
 *  @details This definition is set to 1 to use the card detect pin on an SD card
 *           socket if the socket is so equipped. It should be set to 0 if the card
 *           detect pin is not present, not connected, or not to be used for some
 *           other reason. If this define is true then @c FATFS_USE_DETECT_PIN_RCC, 
 *           @c FATFS_USE_WRITEPROTECT_PIN_PORT, and @c FATFS_USE_DETECT_PIN_PIN 
 *           must be set to match the hardware connections.
 */
#define FATFS_USE_DETECT_PIN            1

/** @brief   Clock for GPIO port used by SD card's card detect pin.
 */
#define FATFS_USE_DETECT_PIN_RCC        RCC_AHB1Periph_GPIOB    

/** @brief   GPIO port used by SD card's card detect pin.
 */
#define FATFS_USE_DETECT_PIN_PORT       GPIOB

/** @brief   Designation for pin on GPIO port used by SD card's card detect pin.
 */
#define FATFS_USE_DETECT_PIN_PIN        GPIO_Pin_4

/** @brief   Whether to use the SD card's write protect pin.
 *  @details This define controls whether the SD card's write protect pin will be used
 *           by the driver. If it is set to 1, the write protect pin will be used; if
 *           it is 0, the write protect pin will be ignored. If this define is true
 *           then @c FATFS_USE_WRITEPROTECT_PIN_RCC, @c FATFS_USE_WRITEPROTECT_PIN_PIN,
 *           and @c FATFS_USE_WRITEPROTECT_PIN_PORT must be set to match the hardware.
 *           Note that some SD card sockets don't even @b have write protect pins. 
 */
#define FATFS_USE_WRITEPROTECT_PIN      0

/** @brief   Clock for GPIO port used by SD card's write protect pin.
 */
#define FATFS_USE_WRITEPROTECT_PIN_RCC  RCC_AHB1Periph_GPIOB

/** @brief   GPIO port used by SD card's write protect pin.
 */
#define FATFS_USE_WRITEPROTECT_PIN_PORT GPIOB

/** @brief   Designation for pin on GPIO port used by SD card's write protect pin.
 */
#define FATFS_USE_WRITEPROTECT_PIN_PIN  GPIO_Pin_7

/** @brief   Whether to use a custom @c get_fattime() function instead of built-in one.
 *  @details This setting causes the use of a custom @c get_fattime() function instead
 *           of the function which is supplied in @c diskio.c. A custom function might
 *           use a real-time clock to create a meaningful timestamp rather than just
 *           using a single date and time for all files as the function in @c diskio.c
 *           does.
 */
#define TM_FATFS_CUSTOM_FATTIME         1

/** @brief   Activate SDIO 1-bit mode.
 *  @details If set true, this define causes the SDIO connection to the SD card to use
 *           one-bit mode rather than four-bit mode. Both 1-bit and 4-bit modes use 
 *           the SDIO protocol, not the SPI protocol, to talk to the card.
 */
#define FATFS_SDIO_1BIT                 0

/** @brief   Activate SDIO 4-bit mode.
 *  @details This define, if set to 1, causes the SDIO connection to the SD card to
 *           use four-bit mode. This requires that the card is connected to the 
 *           microcontroller with the full set of SDIO pins: @c CMD, @c CLK, @c DAT0,
 *           @c DAT1, @c DAT2, and @c DAT3 as well as power, ground, and card detect
 *           (if card detect is used). An excellent comment in @c tm_stm32f4_fatfs.h
 *           shows how the connections are made. 
 */
#define FATFS_SDIO_4BIT                 1

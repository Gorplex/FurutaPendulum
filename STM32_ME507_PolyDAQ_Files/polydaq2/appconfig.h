//=====================================================================================
/** @file appconfig.h
 */
//=====================================================================================

#ifndef _APPCONFIG_H_
#define _APPCONFIG_H_

#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"

#include "FreeRTOS.h"
#include "task.h"

#include "rs232.h"

/** @brief   The name of the application program.
 *  @details This name is used to build the program version string which can be shown
 *           for debugging or tracking purposes.
 */
#define APPL_NAME           "PolyDAQ 2"

/** @brief   The version number of the program.
 *  @details This string gives a version number for the program. It is combined with
 *           the application name and date to give a program identification string
 *           that can be used for debugging or tracking purposes.
 */
#define APPL_VERSION        "2.1.5"

/** @brief   A string showing the name, version, and date of this application program.
 */
#define VERSION_STRING      APPL_NAME " Version " APPL_VERSION " compiled " __DATE__

//---------------------------------- Serial Ports -------------------------------------

/** @brief   Configuration switch to enable UART 0.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 0 is to
 *           be used in this application or 0 if it is not. Note that UART 0 exists
 *           on most AVR processors, but there is no UART numbered 0 on STM32's. 
 */
#define USART_0_ENABLE      0

/** @brief   Configuration switch to enable UART 1.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 1 is to
 *           be used in this application or 0 if it is not. 
 */
#define USART_1_ENABLE      0

/** @brief   Configuration switch to enable UART 2.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 2 is to
 *           be used in this application or 0 if it is not. 
 */
#define USART_2_ENABLE      1

/** @brief   Configuration switch to enable UART 3.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 3 is to
 *           be used in this application or 0 if it is not. 
 */
#define USART_3_ENABLE      1

/** @brief   Configuration switch to enable UART 4.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 4 is to
 *           be used in this application or 0 if it is not. 
 */
#define UART_4_ENABLE       0

/** @brief   Configuration switch to enable UART 5.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 5 is to
 *           be used in this application or 0 if it is not. 
 */
#define UART_5_ENABLE       0

/** @brief   Configuration switch to enable UART 6.
 *  @details This preprocessor definition is set to 1 if UART (serial port) 6 is to
 *           be used in this application or 0 if it is not. 
 */
#define USART_6_ENABLE      0


#endif // _APPCONFIG_H_

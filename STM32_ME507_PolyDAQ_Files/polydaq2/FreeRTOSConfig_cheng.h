//*****************************************************************************
/** @file    FreeRTOSConfig.h 
 *  @brief   Configuration settings for FreeRTOS as it's used in this project.
 *  @details This file contains defines which are used to tune FreeRTOS for 
 *           the processor and performance needs of this project. This includes
 *           configuring the RTOS kernel's performance and specifying which 
 *           of the many optional features to compile into the program. 
 */
//*****************************************************************************

/*
    FreeRTOS V7.0.1 - Copyright (C) 2011 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/


/* The following #error directive is to remind users that a batch file must be
 * executed prior to this project being built.  The batch file *cannot* be 
 * executed from within CCS4!  Once it has been executed, re-open or refresh 
 * the CCS4 project and remove the #error line below. Or, don't use Windows(tm)
 * and don't deal with its lame batch files.
 */
// #error Ensure CreateProjectDirectoryStructure.bat has been executed before building.
// See comment immediately above.


#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *
 * See http://www.freertos.org/a00110.html.
 *----------------------------------------------------------*/

/// @brief Flag which enables pre-emptive (interrupt based) multitasking in FreeRTOS.
#define configUSE_PREEMPTION                  1

/// @brief Flag which causes the idle hook function to be called by FreeRTOS.
#define configUSE_IDLE_HOOK                   0

/// @brief Flag which causes the timer tick hook function to be called by FreeRTOS.
#define configUSE_TICK_HOOK                   0

/** @brief   The rate at which the CPU core's clock runs.
 *  @details The CPU core clock is timed by a PLL and runs at a multiple of the 
 *           frequency of the quartz crystal oscillator on the circuit board. An
 *           STM32F4 can be configured to run at up to 168 MHz.  */
#define configCPU_CLOCK_HZ                    ( 84000000UL )

/** @brief   The rate in Hertz at which the STM32 SysTick clock runs.
 *  @details The SysTick is a timer that is used to create the RTOS timer tick
 *           interrupt.  */
#define configSYSTICK_CLOCK_HZ                ( 84000000UL )

/** @brief   The rate in Hertz at which RTOS ticks are to occur.
 *  @details RTOS ticks are the events where a timer interrupt (a SysTick on STM32's)
 *           interrupts the processor and allows the RTOS to take control. The RTOS
 *           then does internal housekeeping and decides which task is to run next. */
#define configTICK_RATE_HZ                    ( ( portTickType ) 3360 )

/// @brief The maximum number of priority levels which can be used in the program.
#define configMAX_PRIORITIES                  ( ( unsigned portBASE_TYPE ) 5 )

/// @brief The size in bytes of the stack area used by the idle task.
#define configMINIMAL_STACK_SIZE              ( ( unsigned short ) 320 )

/// @brief The number of bytes to be managed by the dynamic memory allocator.
#define configTOTAL_HEAP_SIZE                 ( ( size_t ) ( 50 * 1024 ) )

/// @brief The number of bytes to be reserved for each task's name.
#define configMAX_TASK_NAME_LEN               ( 10 )

/// @brief Switch that activates extra code for execution tracing and visualization.
#define configUSE_TRACE_FACILITY              0

/// @brief Switch that forces the tick counter to be a 16 bit instead of 32 bit number.
#define configUSE_16_BIT_TICKS                0

/// @brief Switch that causes the idle task to yield for other lowest priority tasks.
#define configIDLE_SHOULD_YIELD               1

/// @brief Switch that enables mutexes to be used in the program. 
#define configUSE_MUTEXES                     1

/** @brief The maximum number of queues and semaphores that can be used @e with @e a
 *         @e kernel @e aware @e debugger.  */
#define configQUEUE_REGISTRY_SIZE             0

/// @brief Switch that enables RTOS code to be compiled to keep run time statistics.
#define configGENERATE_RUN_TIME_STATS         0

/// @brief Switch which enables run-time checking for stack overflows.
#define configCHECK_FOR_STACK_OVERFLOW        0

/// @brief Switch which enables the use of recursive mutexes. 
#define configUSE_RECURSIVE_MUTEXES           0

/** @brief Switch which enables the use of a function that runs in case of memory 
 *         allocation failure.  */
#define configUSE_MALLOC_FAILED_HOOK          0

/// @brief Switch which allows user-defined tags to be attached to tasks.
#define configUSE_APPLICATION_TASK_TAG        0

/// @brief Switch which allows the use of counting semaphores. 
#define configUSE_COUNTING_SEMAPHORES         1

/// @brief Switch which allows the use of co-routines for cooperative multitasking.
#define configUSE_CO_ROUTINES                 0

/// @brief The maximum number of co-routine priorities which can be used. 
#define configMAX_CO_ROUTINE_PRIORITIES       ( 2 )

/// @brief Switch which allows the use of software timers.
#define configUSE_TIMERS                      0

/// @brief Priority for task which runs software timers (if timers are enabled).
#define configTIMER_TASK_PRIORITY             ( 3 )

/// @brief Size of queue buffer for software timers (if timers are enabled).
#define configTIMER_QUEUE_LENGTH              6

/// @brief Size of stack for software timer task (if timers are enabled).
#define configTIMER_TASK_STACK_DEPTH          ( configMINIMAL_STACK_SIZE )

/// @brief Switch to enable inclusion of task priority setting function.
#define INCLUDE_vTaskPrioritySet              1

/// @brief Switch to enable inclusion of task priority retrieving function.
#define INCLUDE_uxTaskPriorityGet             1

/// @brief Switch to enable inclusion of function that deletes a task. 
#define INCLUDE_vTaskDelete                   0

/// @brief Switch to enable inclusion of a function that cleans up after a task.
#define INCLUDE_vTaskCleanUpResources         0

/// @brief Switch to enable inclusion of a function that suspends a task.
#define INCLUDE_vTaskSuspend                  1

/** @brief Switch to enable inclusion of a function that delays a task for a
 *         precise time interval since the previous task run.  */
#define INCLUDE_vTaskDelayUntil               1

/// @brief Switch to enable inclusion of a function that delays a task. 
#define INCLUDE_vTaskDelay                    1

/** @brief Switch to enable inclusion of a function that checks maximum stack
 *         usage by a task.  */
#define INCLUDE_uxTaskGetStackHighWaterMark   1

/// @brief Switch to enable inclusion of a function that gets a task's name string.
#define INCLUDE_pcTaskGetTaskName             1

/// @brief Switch to enable inclusion of a function that gets the idle task's handle.
#define INCLUDE_xTaskGetIdleTaskHandle        1

/** @brief Macro that defines how many bits are used to specify interrupt priorities.
 *  @details We use the system definition, if there is one.  */
#ifdef __NVIC_PRIO_BITS
	#define configPRIO_BITS                   __NVIC_PRIO_BITS
#else
	#define configPRIO_BITS                   4        // 15 priority levels
#endif

/** @brief Macro that specifies the lowest interrupt priority used by FreeRTOS
 *  @details This is equivalent to @c configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY.
 *           Details are at 
 * http://www.freertos.org/FreeRTOS_Support_Forum_Archive/February_2012/freertos_STM32_Interrupt_Priorites_5052889.html
 */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY			15

/** @brief   The highest system interrupt priority used by FreeRTOS.
 *  @details Details are at 
 * http://www.freertos.org/FreeRTOS_Support_Forum_Archive/February_2012/freertos_STM32_Interrupt_Priorites_5052889.html
 */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY	5

/// @brief The lowest interrupt priority, in hardware terms (not a CMSIS macro).
#define configKERNEL_INTERRUPT_PRIORITY \
	( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/// @brief Priority 5, or 95 as only the top four bits are implemented.
#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
	( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/** @brief   Macro which implements a simple assertion that @e must be true.
 *  @details This macro hangs the system when the value of an assertion is false.  */
#define configASSERT( x )          \
	if( ( x ) == 0 )               \
	{                              \
		taskDISABLE_INTERRUPTS();  \
		for( ;; );                 \
	}	

/// @brief Interrupt handler for the SVCall interrupt. 
#define vPortSVCHandler SVC_Handler

/// @brief Interrupt handler for the PendSV interrupt. 
#define xPortPendSVHandler PendSV_Handler

/// @brief Intterrupt handler for the SysTick interrupt that runs the RTOS scheduler.
#define xPortSysTickHandler SysTick_Handler

#endif /* FREERTOS_CONFIG_H */


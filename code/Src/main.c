
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

/* USER CODE BEGIN Includes */
#include "trig.h"
#include "math.h"
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM3_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM2_Init(void);
void StartDefaultTask(void const * argument);
                                    
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);
                                

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */
char * debug;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM3_Init();
  MX_SPI1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */
 

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

    /**Configure the Systick interrupt time 
    */
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* SPI1 init function */
static void MX_SPI1_Init(void)
{

  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
  hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

  TIM_MasterConfigTypeDef sMasterConfig;
  TIM_OC_InitTypeDef sConfigOC;

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535/2;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.OCMode = TIM_OCMODE_PWM2;
  sConfigOC.Pulse = 30000;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.Pulse = 1000;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigOC.Pulse = 60000;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  HAL_TIM_MspPostInit(&htim2);

}

/* TIM3 init function */
static void MX_TIM3_Init(void)
{

  TIM_SlaveConfigTypeDef sSlaveConfig;
  TIM_IC_InitTypeDef sConfigIC;
  TIM_MasterConfigTypeDef sMasterConfig;

  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sSlaveConfig.SlaveMode = TIM_SLAVEMODE_RESET;
  sSlaveConfig.InputTrigger = TIM_TS_TI1FP1;
  sSlaveConfig.TriggerPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sSlaveConfig.TriggerPrescaler = TIM_ICPSC_DIV1;
  sSlaveConfig.TriggerFilter = 0;
  if (HAL_TIM_SlaveConfigSynchronization(&htim3, &sSlaveConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_RISING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 0;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_INDIRECTTI;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_2) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

/** Configure pins as 
        * Analog 
        * Input 
        * Output
        * EVENT_OUT
        * EXTI
*/
static void MX_GPIO_Init(void)
{

  GPIO_InitTypeDef GPIO_InitStruct;

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

//-------------------------------------------------------------------------------------
/** @brief    Set PWM duty cycle for a, b, and c outputs (12 bit inputs)
 *  @details  This function is used to set the duty cycle for each of the
 *            3 pwm output waveforms by setting the timer compare register.
 *            It assumes a 12 bit input, thus bit shift..
 *          
 *  @param   a The 12 bit duty cycle for tim2 channel 1 output
 *  @param   b The 12 bit duty cycle for tim2 channel 1 output
 *  @param   c The 12 bit duty cycle for tim2 channel 1 output
 */

inline void pwmOut( uint16_t a, uint16_t b, uint16_t c) {
   /* for some reason, the polarity must be low for the numbers
      to make sense (i.e. bigger == longer high pulse) */
   __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, a << 3 ); 
   __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, b << 3 );
   __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, c << 3 );
}



//-------------------------------------------------------------------------------------
/** @brief   When called, update the PWM output to keep specified torque (12 bit)
 *  @details This function uses the encoder to calculate the correct phase
 *           to output via PWM to generate a field 90 degrees to the current
 *           location. Then this value is scaled by the ratio of torque/1000
 *  @param   torque Controls how much 'torque' is applied from -1000 to 1000
 */
void setMotorTorque( int16_t torque) {
   uint16_t theta;
   int16_t offset = -1195 - 150; //65536/28;//-1146;
   
   theta = ( (4095 * 7 * htim3.Instance->CCR2 / 65535) + offset ) % 4096;n
   pwmOut( torque * sinShift03(theta)/1000 + (torque<0 ? 4096: 0),
	   torque * sinShift13(theta)/1000 + (torque<0 ? 4096: 0),
	   torque * sinShift23(theta)/1000 + (torque<0 ? 4096: 0));
      
   /* At theta = 0, pulse =
      64389 <-- ~zero  (0-1146)
      54784
      45064
      35581
      26081
      16398
      6892

      1/7th of 2^16-1 ~= 9362
      1/7th / 4 (to get 90 degrees ahead) ~= 2341
      2341 - 1146 = 1195  <-- this is ~90 degrees to zero (maybe?)

   */

}


//define addresses (lowest if multiple bytes) (lowest byte in higest addr)


/** @brief 2 bytes Extended Write Address **/
#define EWA 0x02
/** @brief 4 bytes Extended Write Data **/
#define EWD 0x04 //
/** @brief 2 bytes Extended Write Control and Status **/
#define EWCS 0x08 //
/** @brief 2 bytes Extended Read Address **/
#define ERA 0x0A //
/** @brief 2 bytes Extended Read Control and Status **/
#define ERCS 0x0C //
/** @brief 4 bytes Extended Read Data **/
#define ERD 0x0E //

/** @brief Address of the control register  **/
#define CTRL 0x1E //2 bytes 
/** @brief address of the Angle register**/
#define ANG 0x20 //2 bytes 
/** @brief address of the status register**/
#define STA 0x22 //2 bytes 
/** @brief address of the field strength register**/
#define FIELD 0x2A //2 bytes 
/** @brief key needed to start running spi encoder**/
#define CDS_KEYCODE 0x46 



//-------------------------------------------------------------------------------------
/** @brief   Writes val to the addr via SPI
 *  @details This function sets SS pin low, transmits the 16 bits
 *           (0x4000 | addr << 8 | val) and then releases SS pin.
 *           it returns the data recieved via spi.
 *  @param   addr The address to write to
 *  @param   val The value to put in the address
 *  @return   The data recieved via spi
 */
uint16_t spiWrite(uint8_t addr, uint8_t val) {
   uint16_t num = 0x4000 | addr << 8 | val;
   uint16_t rxData;
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
   HAL_SPI_TransmitReceive(&hspi1, & num, &rxData, 1, 0xFF);
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
   return rxData;
}

//-------------------------------------------------------------------------------------
/** @brief   Reads the value at the given address
 *  @details Transmits the address to read, then transmits again to
 *           get the response. This function then returns the value
 *           recieved via spi
 *  @param   addr The address to read from the spi encoder
 *  @return   The value at the given address
 */
uint16_t spiRead(uint8_t addr) {
   uint16_t num = addr << 8;
   uint16_t pRxData;
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
   HAL_SPI_TransmitReceive(&hspi1, & num, &pRxData, 1, 0xFF);
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

   num = 0;
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_RESET);
   HAL_SPI_TransmitReceive(&hspi1, & num, &pRxData, 1, 0xFF);
   HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
   return pRxData;
}


//-------------------------------------------------------------------------------------
/** @brief   Calculate the torque to return motor to setpoint (using simple P controller)
 *  @details This functions returns the torque calculated by a simple P controller
 *           that attempts to keep a constant setpoint. The output is not linear with
 *           positon, but instead has an adjustment to increase low end values to make
 *           sure the motor has enough torque at the low end to reach the target.
 *  @param   setpoint The position that the motor tries to maintain (16 bit number)
 *  @return  The new torque value that should be sent to the motor from -1000 to 1000
 */
int16_t getNewTorque(int16_t setpoint){
   int32_t out = (htim3.Instance->CCR2 -setpoint +65536/2) % 65536 -65536/2;
   out = out >> 4;		/* bit shift to get scalling right */

   /* Adjusting the respones of our motor so that there is more torque on the low end*/
   out = 32*sqrt(abs(out)) * out /abs(out) ;

   /* constrain output to valid numbers */
    if(out>1000){
        out = 1000;
    } 
    if(out<-1000){
        out = -1000;
    }

    return out;
}

/* USER CODE END 4 */



//-------------------------------------------------------------------------------------
/** @brief   Task that sets up the pins, timers, etc. and run basic constant torque example
 *  @details This task configures the gpio to blink an led, enables interrupts for
 *           pwm input, starts the 3 pwm output channels, configures the spi encoder.
 *           Then, this task loops, blinking the led, updating torque, and printing
 *           the current angle of the motor via uart.
 *  @param   argument Not used, but kept for rtos
 */
void StartDefaultTask(void const * argument)
{

  /* USER CODE BEGIN 5 */

   GPIO_InitTypeDef GPIO_InitStructure;
 
   GPIO_InitStructure.Pin = GPIO_PIN_12;
 
   GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;
   //GPIO_InitStructure.Pull = GPIO_NOPULL;
   HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

   debug = "none";

   //Enable interrupt
   HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_2);
   HAL_TIM_IC_Start(&htim3, TIM_CHANNEL_1);

   
   HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); 
   HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
   HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);

   setMotorTorque(0);
   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
   osDelay(100);
   
   spiWrite(CTRL, 0xC0);
   spiWrite(CTRL, 0xC0);
   spiWrite(CTRL+1, CDS_KEYCODE);
   spiRead(ANG);

   
   uint32_t count=0;
   uint32_t loop=0;
  /* Infinite loop */
  for(;;)
  {
     /* if(count % 50 == 0) { */
     /* 	osDelay(1); */
     /* } */
    if ( count == 2000) {
       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    }
    if (count == 4095) {
       HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
       count=0;

       /* loop++; */

       /* if(loop == 300) { */
       /* 	  setMotorTorque( -250); */
       /* } */
       /* if(loop == 600) { */
       /* 	  setMotorTorque( 250); */
       /* 	  loop=0; */
       // }       
       char buffer[100];
       sprintf(buffer, "Angle: %d\n", spiRead(ANG));
       HAL_UART_Transmit(&huart1, buffer ,strlen(buffer) , HAL_MAX_DELAY);

    }
    count++;
    

    /* setMotorTorque(getNewTorque(10000)); */
    setMotorTorque(1000);


    
  }
  /* USER CODE END 5 */ 
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM4 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM4) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

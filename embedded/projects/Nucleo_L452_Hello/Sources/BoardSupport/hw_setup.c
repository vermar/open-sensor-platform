/* OSP Hello World Project
 * https://github.com/vermar/open-sensor-platform
 *
 * Copyright (C) 2016 Rajiv Verma
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include "common.h"
#include "hw_setup.h"
#include "debugprint.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
DeviceUid_t *gDevUniqueId = (DeviceUid_t *)(DEV_UID_OFFSET);

GPioPinInfo_t DiagLEDs[NUM_LEDS] = {
    { //LD4 (Green)
        RCC_AHB2ENR_GPIOBEN,
        GPIOB,
        GPIO_PIN_13
    },
#if 0
    { //LED3 (White)
        RCC_AHB2ENR_GPIOEEN,
        GPIOE,
        GPIO_PIN_8
    },
    { //LED5 (White)
        RCC_AHB2ENR_GPIODEN,
        GPIOD,
        GPIO_PIN_12
    },
    { //LED6 (White)
        RCC_AHB2ENR_GPIOCEN,
        GPIOC,
        GPIO_PIN_6
    },
#endif
};

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static TIM_HandleTypeDef htim;

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      UartDMAConfiguration
 *          Configures the DMA for use with the selected UART
 *
 ***************************************************************************************************/
static void UartDMAConfiguration( UARThandle_t *phUart )
{
    /* Configure the DMA handler for Transmission process */
    gDbgUartPort.hDMA->Instance                 = DBG_UART_TX_DMA_CHANNEL;
    gDbgUartPort.hDMA->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    gDbgUartPort.hDMA->Init.PeriphInc           = DMA_PINC_DISABLE;
    gDbgUartPort.hDMA->Init.MemInc              = DMA_MINC_ENABLE;
    gDbgUartPort.hDMA->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    gDbgUartPort.hDMA->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    gDbgUartPort.hDMA->Init.Mode                = DMA_NORMAL;
    gDbgUartPort.hDMA->Init.Priority            = DMA_PRIORITY_LOW;
    gDbgUartPort.hDMA->Init.Request             = DBG_UART_TX_DMA_REQUEST;

    HAL_DMA_Init(gDbgUartPort.hDMA);

    /* Associate the initialized DMA handle to the UART handle */
    __HAL_LINKDMA(phUart, hdmatx, *gDbgUartPort.hDMA);
}
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      LED_Init
 *          Configures the diagnostic LEDs in the system
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void LED_Init( void )
{
    GPIO_InitTypeDef  GPIO_InitStructure;
    uint8_t index;

    if (NUM_LEDS > 0)
    {
        for (index = 0; index < NUM_LEDS; index++)
        {
            RCC_GPIO_CLK_ENABLE( DiagLEDs[index].rccPeriph );
            GPIO_InitStructure.Pin = DiagLEDs[index].pin;
            GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
            GPIO_InitStructure.Pull = GPIO_PULLUP;
            GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            HAL_GPIO_Init(DiagLEDs[index].grp, &GPIO_InitStructure);
        }
    }
}


/****************************************************************************************************
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (MSI)
  *            SYSCLK(Hz)                     = 80000000
  *            HCLK(Hz)                       = 80000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 1
  *            APB2 Prescaler                 = 1
  *            MSI Frequency(Hz)              = 4000000
  *            PLL_M                          = 1
  *            PLL_N                          = 40
  *            PLL_R                          = 2
  *            PLL_P                          = 7
  *            PLL_Q                          = 4
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
 ***************************************************************************************************/
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* MSI is enabled after System reset, activate PLL with MSI as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState = RCC_MSI_ON;
    RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
    RCC_OscInitStruct.PLL.PLLM = 1;
    RCC_OscInitStruct.PLL.PLLN = 40;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLP = 7;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        /* Initialization Error */
        while(1);
    }
    
    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
    RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;  
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  

    if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        /* Initialization Error */
        while(1);
    }
}


/****************************************************************************************************
 * @fn      SystemGPIOConfig
 *          Configures the various GPIO ports on the chip according to the usage by various
 *          peripherals.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void SystemGPIOConfig( void )
{
    /* Set all GPIOs to analog input mode to begin with */
    //GPIO_AINConfig();
}


/****************************************************************************************************
 * @fn      SystemInterruptConfig
 *          Configures the nested vectored interrupt controller.
 *
 ***************************************************************************************************/
void SystemInterruptConfig( void )
{
}


/****************************************************************************************************
 * @fn      RTC_Configuration
 *          Configures the RTC (or any general purpose timer used for the purpose).
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void RTC_Configuration( void )
{
    RCC_ClkInitTypeDef sClokConfig;
    uint32_t TimerClk, APB1Prescaler = 0;
    uint32_t pFLatency;
    uint32_t PrescalerValue = 0;

    RTC_TIMER_CLK_EN();

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&sClokConfig, &pFLatency);

    /* Get APB1 prescaler */
    APB1Prescaler = sClokConfig.APB1CLKDivider;

    /* Compute RTC Timer clock */
    if (APB1Prescaler == 0) 
    {
        TimerClk = HAL_RCC_GetPCLK1Freq();
    }
    else
    {
        TimerClk = 2*HAL_RCC_GetPCLK1Freq();
    }

    /* Compute the prescaler value to have timer counter clock tick at RTC Tick rate */
    PrescalerValue = (uint32_t) ((TimerClk / 1000000) * US_PER_RTC_TICK - 1);

    /* Time base configuration */
    htim.Instance = RTC_TIMER;
    htim.Init.Period = 0xFFFFFFFF;
    htim.Init.Prescaler = PrescalerValue;
    htim.Init.ClockDivision = 0;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;

    HAL_TIM_Base_Init(&htim);

    /* Enable Timer counter */
    HAL_TIM_Base_Start(&htim);
}


/****************************************************************************************************
 * @fn      RTC_GetCounter
 *          Returns the counter value of the timer register
 *
 * @param   none
 *
 * @return  32-bit counter value
 *
 ***************************************************************************************************/
uint32_t RTC_GetCounter( void )
{
    return __HAL_TIM_GET_COUNTER( &htim );
}


/****************************************************************************************************
 * @fn      HAL_UART_MspInit
 *          This function configures the hardware resources used in this example:
 *           - Peripheral's clock enable
 *           - Peripheral's GPIO Configuration
 *           - DMA configuration for transmission request by peripheral
 *           - NVIC configuration for DMA interrupt request enable
 *
 * @param   huart - Pointer to Uart Handle
 *
 * @return  none
 *
 ***************************************************************************************************/
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef  GPIO_InitStruct;

    /*##-1- Enable peripherals and GPIO Clocks #################################*/
    /* Enable GPIO TX/RX clock */
    DBG_UART_TX_GPIO_CLK_ENABLE();
    DBG_UART_RX_GPIO_CLK_ENABLE();

    /* Enable USARTx clock */
    DBG_UART_CLK_ENABLE();

    /* Enable DMA clock */
    DMAx_CLK_ENABLE();

    /*##-2- Configure peripheral GPIO ##########################################*/
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = DBG_UART_TX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_HIGH;
    GPIO_InitStruct.Alternate = DBG_UART_TX_AF;

    HAL_GPIO_Init(DBG_UART_TX_GPIO_PORT, &GPIO_InitStruct);

    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin = DBG_UART_RX_PIN;
    GPIO_InitStruct.Alternate = DBG_UART_RX_AF;

    HAL_GPIO_Init(DBG_UART_RX_GPIO_PORT, &GPIO_InitStruct);

    /*##-3- Configure the DMA ##################################################*/
    UartDMAConfiguration( huart );

    /*##-4- Configure the NVIC for DMA #########################################*/
    /* NVIC configuration for DMA transfer complete interrupt (DBG_UART_TX) */
    HAL_NVIC_SetPriority(DBG_UART_DMA_TX_IRQn, DBG_UART_DMA_INT_PREEMPT_PRIORITY, DBG_UART_TX_DMA_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(DBG_UART_DMA_TX_IRQn);

    __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);
    /* NVIC configuration for USART, to catch the TX complete */
    HAL_NVIC_SetPriority(DBG_UART_IRQn, DBG_UART_INT_PREEMPT_PRIORITY, DBG_UART_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(DBG_UART_IRQn);
}


/****************************************************************************************************
 * @fn      HAL_UART_MspDeInit
 *          This function frees the hardware resources used:
 *          - Disable the Peripheral's clock
 *          - Revert GPIO, DMA and NVIC configuration to their default state
 *
 * @param   huart - Pointer to Uart Handle
 *
 * @return  none
 *
 ***************************************************************************************************/
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    /*##-1- Reset peripherals ##################################################*/
    DBG_UART_FORCE_RESET();
    DBG_UART_RELEASE_RESET();

    /*##-2- Disable peripherals and GPIO Clocks ################################*/
    /* Configure UART Tx as alternate function  */
    HAL_GPIO_DeInit(DBG_UART_TX_GPIO_PORT, DBG_UART_TX_PIN);
    /* Configure UART Rx as alternate function  */
    HAL_GPIO_DeInit(DBG_UART_RX_GPIO_PORT, DBG_UART_RX_PIN);

    /*##-3- Disable the DMA Streams ############################################*/
    /* De-Initialize the DMA Stream associated to transmission process */
    HAL_DMA_DeInit(huart->hdmatx);

    /*##-4- Disable the NVIC for DMA ###########################################*/
    HAL_NVIC_DisableIRQ(DBG_UART_DMA_TX_IRQn);
    HAL_NVIC_DisableIRQ(DBG_UART_DMA_RX_IRQn);
}


/****************************************************************************************************
 * @fn      DebugUARTConfig
 *          Configures the selected USART for UART operation with the given baud rate parameters.
 *          Note that flow control is not supported due to I/O limitations.
 *
 * @param   baud    Baud rate value
 * @param   dataLen Word length of the serial data
 * @param   stopBits Number of stop bits
 * @param   parity  Parity - Odd/Even/None
 *
 * @return  none
 *
 ***************************************************************************************************/
void DebugUARTConfig( uint32_t baud, uint32_t dataLen, uint32_t stopBits, uint32_t parity )
{
    gDbgUartPort.hUart->Instance          = DBG_IF_UART;
    gDbgUartPort.hUart->Init.BaudRate     = baud;
    gDbgUartPort.hUart->Init.WordLength   = dataLen;
    gDbgUartPort.hUart->Init.StopBits     = stopBits;
    gDbgUartPort.hUart->Init.Parity       = parity;
    gDbgUartPort.hUart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    gDbgUartPort.hUart->Init.Mode         = UART_MODE_TX_RX;
    gDbgUartPort.hUart->Init.OverSampling = UART_OVERSAMPLING_16;

    /* Note that HAL_UART_Init() internally calls HAL_UART_MspInit() to configure the hardware interfaces */
    HAL_UART_Init(gDbgUartPort.hUart);
}


#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      UartTxDMAStart
 *          Setup the DMA for transfer
 *
 * @param   pPort - Pointer to the port control structure
 * @param   pTxBuffer - Pointer to buffer that needs to be DMA-ed to Uart
 * @param   txBufferSize - Size of the buffer
 *
 * @return  none
 ***************************************************************************************************/
void UartTxDMAStart( PortInfo *pPort, uint8_t *pTxBuffer, uint16_t txBufferSize )
{
    HAL_StatusTypeDef result;

    /* Note that HAL_UART_Transmit_DMA() uses internal callback on DMA Transfer Complete (TC) that
     * Sets the UART TX Complete interrupt flag upon completion of DMA transfer
     */
    result = HAL_UART_Transmit_DMA(pPort->hUart, pTxBuffer, txBufferSize);
    ASF_assert(result == HAL_OK);
}
#endif


/****************************************************************************************************
 * @fn      HAL_InitTick
 *          Overrides the default implementation in stm32f7xx_hal.c file.
 *
 ***************************************************************************************************/
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    /* Do nothing since we don't want anyone messing with SysTick that is used by RTOS */
    return HAL_OK;
}


/****************************************************************************************************
 * @fn      HAL_GetTick
 *          Overrides the default implementation in stm32f7xx_hal.c file.
 *
 ***************************************************************************************************/
uint32_t HAL_GetTick(void)
{
    return ((__HAL_TIM_GET_COUNTER( &htim ) * US_PER_RTC_TICK) / 1000);
}

/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

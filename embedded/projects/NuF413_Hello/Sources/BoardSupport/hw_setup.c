/*==================================================================================================
** Copyright (C) 2024 Rajiv Verma
**
** Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except
** in compliance with the License. You may obtain a copy of the License at:
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**==================================================================================================
**  @file hw_setup.c
**  @brief Implements various hardware initialization helper routines
**
**  In general this file implements common and/or early hardware initializations - such as input/
**  output GPIOs, RTC, Timers, etc. Other peripheral interfaces (e.g. SPI/I2C) are initialized in
**  the respective drivers or tasks that handle those interfaces. Note that certain logic signals
**  e.g. Chip Select for SPI interface should be initialized early to ensure that those signals
**  are not held in undefined state for too long after boot up.
**
*/
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "common.h"
#include "debugprint.h"

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
extern void DbgUartTxCompleteCallback(UARThandle_t* huart);

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define FREQ_1MHZ                       1000000

//==================================================================================================
//    P R I V A T E   T Y P E   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
static TIM_HandleTypeDef s_hRtcTimer;

//==================================================================================================
//    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
DeviceUid_t *gDevUniqueId = (DeviceUid_t *)(DEV_UID_OFFSET);

GPioOutputInfo_t OutputGPIOs[NUM_GPIO_OUTPUTS] =
{
    { //USB Power Enable
        GPIOD,
        GPIO_PIN_10,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    }
};

GPioInputInfo_t InputGPIOs[NUM_GPIO_INPUTS] =
{
    { //GPIO_IN_USR_BTN - User Button (Blue)
        GPIOC,
        GPIO_PIN_13,
        GPIO_PULLDOWN,
        HW_REV_COMMON
    },
};

GPioOutputInfo_t DiagLEDs[NUM_LEDS] =
{
    { //LED_GREEN (LD1)
        GPIOB,
        GPIO_PIN_0,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    },
    { //LED_RED (LD3)
        GPIOB,
        GPIO_PIN_14,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    },
    { //LED_YELLOW (LD2)
        GPIOE,
        GPIO_PIN_1,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    },
};

//==================================================================================================
//    P R I V A T E     F U N C T I O N S
//==================================================================================================
#ifdef UART_DMA_ENABLE
/***************************************************************************************************
** @brief Configures the DMA for use with the selected UART
**
** @param phUart: ASF specific handle to the Uart Peripheral
**
** @return None
*/
static void UartDMAConfiguration( UARThandle_t *phUart )
{
    /* Configure the DMA handler for Transmission process */
    gDbgUartPort.hDMA->Instance                 = DBG_UART_TX_DMA_STREAM;
    gDbgUartPort.hDMA->Init.Channel             = DBG_UART_TX_DMA_CHANNEL;
    gDbgUartPort.hDMA->Init.Direction           = DMA_MEMORY_TO_PERIPH;
    gDbgUartPort.hDMA->Init.PeriphInc           = DMA_PINC_DISABLE;
    gDbgUartPort.hDMA->Init.MemInc              = DMA_MINC_ENABLE;
    gDbgUartPort.hDMA->Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    gDbgUartPort.hDMA->Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    gDbgUartPort.hDMA->Init.Mode                = DMA_NORMAL;
    gDbgUartPort.hDMA->Init.Priority            = DMA_PRIORITY_LOW;
    gDbgUartPort.hDMA->Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    gDbgUartPort.hDMA->Init.FIFOThreshold       = DMA_FIFO_THRESHOLD_FULL;
    gDbgUartPort.hDMA->Init.MemBurst            = DMA_MBURST_INC4;
    gDbgUartPort.hDMA->Init.PeriphBurst         = DMA_PBURST_INC4;

    HAL_DMA_Init(gDbgUartPort.hDMA);

    /* Associate the initialized DMA handle to the UART handle */
    __HAL_LINKDMA(phUart, hdmatx, *gDbgUartPort.hDMA);
}
#endif

/***************************************************************************************************
** @brief MCU Support Package (MSP) initialization related to debug Uart interface
**
** @param phUart: HAL specific handle to the Uart Peripheral
**
** @return None
*/
static void DbgUartMspInit(UARThandle_t* huart)
{
    GPIO_InitTypeDef  gpioInit;

    /* Enable GPIO TX/RX clock */
    DBG_UART_TX_GPIO_CLK_ENABLE();
    DBG_UART_RX_GPIO_CLK_ENABLE();

    /* Enable USARTx clock */
    DBG_UART_CLK_ENABLE();

    /* Enable DMA clock */
    DBG_UART_DMAx_CLK_ENABLE();

    /* ### Configure peripheral GPIOs */
    /* UART TX GPIO pin configuration  */
    gpioInit.Pin   = DBG_UART_TX_PIN;
    gpioInit.Mode  = GPIO_MODE_AF_PP;
    gpioInit.Pull  = GPIO_PULLUP;
    gpioInit.Speed = GPIO_SPEED_HIGH;
    gpioInit.Alternate = DBG_UART_TX_AF;

    HAL_GPIO_Init(DBG_UART_TX_GPIO_PORT, &gpioInit);

    /* UART RX GPIO pin configuration  */
    gpioInit.Pin = DBG_UART_RX_PIN;
    gpioInit.Alternate = DBG_UART_RX_AF;

    HAL_GPIO_Init(DBG_UART_RX_GPIO_PORT, &gpioInit);

    /* ### Configure the DMA */
    UartDMAConfiguration(huart);

    /* ### Configure the NVIC for DMA */
    /* NVIC configuration for DMA transfer complete interrupt (DBG_UART_TX) */
    HAL_NVIC_SetPriority(DBG_UART_DMA_TX_IRQn, DBG_UART_DMA_INT_PREEMPT_PRIORITY, DBG_UART_TX_DMA_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(DBG_UART_DMA_TX_IRQn);

    __HAL_UART_ENABLE_IT(huart, UART_IT_RXNE);

    /* NVIC configuration for USART, to catch the TX complete */
    HAL_NVIC_SetPriority(DBG_UART_IRQn, DBG_UART_INT_PREEMPT_PRIORITY, DBG_UART_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(DBG_UART_IRQn);
}

/***************************************************************************************************
** @brief Configures the GPIO clock (RCC) for the given GPIO instance
**
** Optional details about the function
*/
static void enableRcc_GPIOx(GPIO_TypeDef* GPIOx)
{
    ASF_assert(IS_GPIO_ALL_INSTANCE(GPIOx));

    switch ((uint32_t) GPIOx)
    {
    case GPIOA_BASE:
        __HAL_RCC_GPIOA_CLK_ENABLE();
        break;

    case GPIOB_BASE:
        __HAL_RCC_GPIOB_CLK_ENABLE();
        break;

    case GPIOC_BASE:
        __HAL_RCC_GPIOC_CLK_ENABLE();
        break;

    case GPIOD_BASE:
        __HAL_RCC_GPIOD_CLK_ENABLE();
        break;

    case GPIOE_BASE:
        __HAL_RCC_GPIOE_CLK_ENABLE();
        break;

    case GPIOF_BASE:
        __HAL_RCC_GPIOF_CLK_ENABLE();
        break;

    case GPIOG_BASE:
        __HAL_RCC_GPIOG_CLK_ENABLE();
        break;

    case GPIOH_BASE:
        __HAL_RCC_GPIOH_CLK_ENABLE();
        break;

    default:
        D0_printf("Unhandled GPIOx!\r\n");
        break;
    }
}

/***************************************************************************************************
** @brief Configures the GPIO Outputs in the system that are specific for given HW revisions
**
** @param  hwRev: Hardware revision
**
** @return None
*/
static void GPIO_OutputInit(uint16_t hwRev)
{
    GPIO_InitTypeDef  gpioInit;
    uint8_t index;

    if (NUM_GPIO_OUTPUTS > 0)
    {
        for (index = 0; index < NUM_GPIO_OUTPUTS; index++)
        {
            if (OutputGPIOs[index].hwCompat & hwRev)
            {
                enableRcc_GPIOx(OutputGPIOs[index].grp);
                gpioInit.Pin   = OutputGPIOs[index].pin;
                gpioInit.Mode  = GPIO_MODE_OUTPUT_PP;
                gpioInit.Pull  = GPIO_NOPULL;
                gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                HAL_GPIO_Init(OutputGPIOs[index].grp, &gpioInit);
                //Set initial output state
                if (OutputGPIOs[index].initState == GPIO_STATE_LOW)
                {
                    GPIO_SetLow(index);
                }
                else
                {
                    GPIO_SetHigh(index);
                }
            }
        }
    }
}

/***************************************************************************************************
** @brief Configures the GPIO Inputs in the system that are specific for given HW revisions
**
** @param  hwRev: Hardware revision
**
** @return None
*/
static void GPIO_InputInit(uint16_t hwRev)
{
    GPIO_InitTypeDef  gpioInit;
    uint8_t index;

    if (NUM_GPIO_INPUTS > 0)
    {
        for (index = 0; index < NUM_GPIO_INPUTS; index++)
        {
            if (InputGPIOs[index].hwCompat & hwRev)
            {
                enableRcc_GPIOx(InputGPIOs[index].grp);
                gpioInit.Pin   = InputGPIOs[index].pin;
                gpioInit.Mode  = GPIO_MODE_INPUT;
                gpioInit.Pull  = InputGPIOs[index].pullMode;
                gpioInit.Speed = GPIO_SPEED_FREQ_LOW;
                HAL_GPIO_Init(InputGPIOs[index].grp, &gpioInit);
            }
        }
    }
}

/***************************************************************************************************
** @brief Get the timer clocking frequency based on timer id
**
** @param  pTim: Reference to the timer
**
** @return Frequency value in Hz for the timer clock
*/
static uint32_t GetTimerClock(TIM_TypeDef *pTim)
{
    RCC_ClkInitTypeDef clkConfig;
    uint32_t timerClkHz, apbPrescalar = 0;
    uint32_t pFLatency;

    /* Get clock configuration */
    HAL_RCC_GetClockConfig(&clkConfig, &pFLatency);

    /* NOTE: Timer availability depends on actual part number of the MCU */
    /* 32-Bit Timers: 2, 5 */
    /* 16-Bit Timers: 1, 3, 4, 9-14 */
    /* Adv. Control Timers: 1, 8 */
    /* Gen purpose: 2,3,4,5,9-14 */
    /* Timer 1,8 are APB2 Peripherals */
    if ((pTim == TIM1) || (pTim == TIM8))
    {
        /* Get APBx prescaler */
        apbPrescalar = clkConfig.APB2CLKDivider;

        /* Compute RTC Timer clock frequency */
        if (apbPrescalar == RCC_HCLK_DIV1)
        {
            timerClkHz = HAL_RCC_GetPCLK2Freq();
        }
        else
        {
            timerClkHz = 2 * HAL_RCC_GetPCLK2Freq();
        }
    }
    else    /* Timer 2-7, 9-14 are APB1 */
    {
        /* Get APBx prescaler */
        apbPrescalar = clkConfig.APB1CLKDivider;

        /* Compute RTC Timer clock frequency */
        if (apbPrescalar == RCC_HCLK_DIV1)
        {
            timerClkHz = HAL_RCC_GetPCLK1Freq();
        }
        else
        {
            timerClkHz = 2 * HAL_RCC_GetPCLK1Freq();
        }
    }
    return timerClkHz;
}

//==================================================================================================
//    P U B L I C     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Configures the diagnostic LEDs in the system that are specific for given HW revisions
**
** @param hwRev: HW revision of the current board
**
** @return None
*/
void LED_Init( uint16_t hwRev )
{
    GPIO_InitTypeDef  gpioInit;
    uint8_t index;

    if (NUM_LEDS > 0)
    {
        for (index = 0; index < NUM_LEDS; index++)
        {
            if (DiagLEDs[index].hwCompat & hwRev)
            {
                enableRcc_GPIOx(DiagLEDs[index].grp);
                gpioInit.Pin   = DiagLEDs[index].pin;
                gpioInit.Mode  = GPIO_MODE_OUTPUT_PP;
                gpioInit.Pull  = GPIO_NOPULL;
                gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                HAL_GPIO_Init(DiagLEDs[index].grp, &gpioInit);
                //Set initial output state
                if (DiagLEDs[index].initState == GPIO_STATE_LOW)
                {
                    LED_Off(index);
                }
                else
                {
                    LED_On(index);
                }
            }
        }
    }
}

/***************************************************************************************************
** @brief System Clock Configuration
**
**  The system Clock is configured as follow :
**     System Clock source            = PLL (HSE Bypass)
**     SYSCLK(Hz)                     = 100000000 (CPU Clock)
**     HCLK(Hz)                       = 100000000 (AHB Clock)
**     AHB Prescaler                  = 1
**     APB1 Prescaler                 = 2
**     APB2 Prescaler                 = 1
**     HSE Frequency(Hz)              = 8000000 (ext clock from STLink)
**     PLL_M                          = 8
**     PLL_N                          = 200
**     PLL_P                          = 2
**     PLL_Q                          = 7
**     PLL_R                          = 2
**     VDD(V)                         = 3.3
**     Main regulator output voltage  = Scale1 mode
**     Flash Latency(WS)              = 3
**
** @param  None
**
** @return None
*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscInit = {0};
    RCC_ClkInitTypeDef clkInit = {0};
    HAL_StatusTypeDef ret;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    /* Enable HSE Oscillator and activate PLL with HSE as source */
    oscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscInit.HSEState = RCC_HSE_ON;
    oscInit.PLL.PLLState = RCC_PLL_ON;
    oscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscInit.PLL.PLLM = 8;
    oscInit.PLL.PLLN = 200;
    oscInit.PLL.PLLP = RCC_PLLP_DIV2;
    oscInit.PLL.PLLQ = 7;
    oscInit.PLL.PLLR = 2;
    ret = HAL_RCC_OscConfig(&oscInit);
    ASF_assert(ret == HAL_OK);

    /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
       clocks dividers */
    clkInit.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
    clkInit.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clkInit.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clkInit.APB1CLKDivider = RCC_HCLK_DIV2;
    clkInit.APB2CLKDivider = RCC_HCLK_DIV1;
    ret = HAL_RCC_ClockConfig(&clkInit, FLASH_LATENCY_3);
    ASF_assert(ret == HAL_OK);
}

/***************************************************************************************************
** @brief Configures the various GPIO ports on the chip according to the usage by various
**        peripherals.
**
** @param None
**
** @return None
*/
void SystemGPIOConfig( void )
{
    GPIO_InputInit(HW_REV_COMMON);
    GPIO_OutputInit(HW_REV_COMMON);
}

/***************************************************************************************************
** @brief Configures the nested vectored interrupt controller.
**
** @param None
**
** @return None
*/
void SystemInterruptConfig( void )
{
    /* Note: NVIC Priority setting is done in HAL_Init. Can be overridden here */
    //HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/***************************************************************************************************
** @brief Configures the RTC (or any general purpose timer used for the purpose).
**
** @param None
**
** @return None
*/
void RTC_Configuration( void )
{
    HAL_StatusTypeDef retVal;
    uint32_t timerClk;
    uint32_t prescalarVal;

    RTC_TIMER_CLK_EN();

    /* Get timer clock frequency */
    timerClk = GetTimerClock(RTC_TIMER);

    /* Compute the prescaler value to have timer counter clock tick at RTC Tick rate */
    prescalarVal = (uint32_t)((timerClk / FREQ_1MHZ) * US_PER_RTC_TICK - 1);

    /* Time base configuration */
    s_hRtcTimer.Instance = RTC_TIMER;
    __HAL_TIM_SET_COUNTER(&s_hRtcTimer, 100);
    s_hRtcTimer.Init.Period = 0xFFFFFFFF;
    s_hRtcTimer.Init.Prescaler = prescalarVal;
    s_hRtcTimer.Init.ClockDivision = 0;
    s_hRtcTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
    s_hRtcTimer.Init.RepetitionCounter = 0;
    s_hRtcTimer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&s_hRtcTimer);

    /* Enable Timer counter */
    retVal = HAL_TIM_Base_Start(&s_hRtcTimer);
    ASF_assert(retVal == HAL_OK);
}

/***************************************************************************************************
** @brief Returns the counter value of the timer register
**
** @param None
**
** @return 32-bit counter value (unsigned)
*/
uint32_t RTC_GetCounter( void )
{
    return __HAL_TIM_GET_COUNTER( &s_hRtcTimer );
}

/***************************************************************************************************
** @brief This function configures the hardware resources used by debug UART
**
**    - Peripheral's clock enable
**    - Peripheral's GPIO Configuration
**    - DMA configuration for transmission request by peripheral
**    - NVIC configuration for DMA interrupt request enable
**
** @param huart:   Pointer to Uart Handle
**
** @return None
*/
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == DBG_IF_UART)
    {
        DbgUartMspInit(huart);
    }
}

/***************************************************************************************************
** @brief This function frees the hardware resources used by debug UART
**
**    - Disable the Peripheral's clock
**    - Revert GPIO, DMA and NVIC configuration to their default state
**
** @param huart:   Pointer to Uart Handle
**
** @return None
*/
void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance == DBG_IF_UART)
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
}

/***************************************************************************************************
** @brief Configures the selected USART for UART operation with the given baud rate parameters.
**        Note that flow control is not supported.
**
** @param baud:    Baud rate value
** @param dataLen: Word length of the serial data
** @param stopBits: Number of stop bits
** @param parity:  Parity - Odd/Even/None
**
** @return None
*/
void DebugUARTConfig( uint32_t baud, uint32_t dataLen, uint32_t stopBits, uint32_t parity )
{
    HAL_StatusTypeDef result;

    gDbgUartPort.hUart->Instance          = DBG_IF_UART;
    gDbgUartPort.hUart->Init.BaudRate     = baud;
    gDbgUartPort.hUart->Init.WordLength   = dataLen;
    gDbgUartPort.hUart->Init.StopBits     = stopBits;
    gDbgUartPort.hUart->Init.Parity       = parity;
    gDbgUartPort.hUart->Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    gDbgUartPort.hUart->Init.Mode         = UART_MODE_TX_RX;
    gDbgUartPort.hUart->Init.OverSampling = UART_OVERSAMPLING_16;

    /* Note that HAL_UART_Init() internally calls HAL_UART_MspInit() to configure the hardware interfaces */
    result = HAL_UART_DeInit(gDbgUartPort.hUart);
    ASF_assert(result == HAL_OK);
    result = HAL_UART_Init(gDbgUartPort.hUart);
    ASF_assert(result == HAL_OK);
}

#ifdef UART_DMA_ENABLE
/***************************************************************************************************
** @brief Setup DMA transfer with the given buffer
**
** @param pPort:   Pointer to the port control structure
** @param pTxBuffer: Pointer to buffer that needs to be DMA-ed to Uart
** @param txBufferSize: Size of the buffer
**
** @return None
*/
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
* @fn      HAL_UART_TxCpltCallback
*          Callback function defined 'weak' in HAL and can be overridden by user. This is invoked by
*          HAL_UART_IRQHandler(). For DMA based UART transmit, this function is invoked because the
*          USART Transmit Complete Interrupt is enabled as part of DMA transfer complete interrupt
*          handling. Compared to the Std.Periph. Driver implementation this implementation has this
*          extra interrupt invocation and handling as opposed to just having the DMA TC interrupt
*          take care of queuing the next print buffer.
*
* @param   huart Handle to the USART peripheral
*
* @return  none
*
***************************************************************************************************/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == DBG_IF_UART)
    {
        DbgUartTxCompleteCallback(huart);
    }
}

/***************************************************************************************************
** @brief Initialize the global MSP
**
** @param None
**
** @return None
*/
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();
}

/***************************************************************************************************
** @brief Overrides the default implementation in stm32f4xx_hal.c file.
**
** @param tickPriority: (unused)
**
** @return HAL status code (always HAL_OK)
*/
HAL_StatusTypeDef HAL_InitTick(uint32_t tickPriority)
{
    /* Do nothing since we don't want anyone messing with SysTick that is used by RTOS */
    return HAL_OK;
}

/***************************************************************************************************
** @brief Overrides the default implementation in stm32f4xx_hal.c file.
**
** @param None
**
** @return Tick values in ms from the RTC timer
*/
uint32_t HAL_GetTick(void)
{
    return ((__HAL_TIM_GET_COUNTER( &s_hRtcTimer ) * US_PER_RTC_TICK) / 1000);
}

//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

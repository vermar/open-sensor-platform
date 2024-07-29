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
#include <string.h>

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
extern void DbgUartTxCompleteCallback(UARThandle_t* huart);
#ifdef __ICCARM__
extern const uint32_t __region_SRAM1_start__;
#elif defined (__CC_ARM) || defined (__clang__)
extern uint32_t Image$$RW_IRAM1$$Base; //"RW_IRAM1" symbol is from the scatter file that is auto-generated
extern uint32_t Image$$RW_IRAM2$$Base; //"RW_IRAM2" symbol is from the scatter file
#elif defined (__GNUC__)
extern const uint32_t __bss_start__[]; //TBD - fix for GCC
#endif

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define FREQ_1MHZ                       1000000
#define RTC_INITIAL_COUNT               0

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

const GPioOutputInfo_t OutputGPIOs[NUM_GPIO_OUTPUTS] =
{
    { //USB Power Enable
        "USB_FS_PWR_EN",        //Schematic name
        GPIOD,                  //Port
        GPIO_PIN_10,            //Pin
        GPIO_STATE_LOW,         //Initial state (on power-up)
        HW_REV_COMMON           //HW version this applies to
    }
};

const GPioInputInfo_t InputGPIOs[NUM_GPIO_INPUTS] =
{
    { //GPIO_IN_USR_BTN - User Button (Blue)
        "USER_BUTTON",          //Also marked as WAKEUP_KEY or USER
        GPIOC,
        GPIO_PIN_13,
        GPIO_PULLDOWN,
        HW_REV_COMMON
    },
    { //GPIO_IN_CN9_IO1
        "IO (PG0)",
        GPIOG,
        GPIO_PIN_0,
        GPIO_PULLDOWN,
        HW_REV_COMMON
    },
};

const GPioOutputInfo_t DiagLEDs[NUM_LEDS] =
{
    { //LED_GREEN (LD1)
        "LED GREEN",
        GPIOB,
        GPIO_PIN_0,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    },
    { //LED_RED (LD3)
        "LED RED",
        GPIOB,
        GPIO_PIN_14,
        GPIO_STATE_LOW,
        HW_REV_COMMON
    },
    { //LED_YELLOW (LD2)
        "LED YELLOW",
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
    gDbgUartPort.hDMA->Init.Request             = DBG_UART_TX_DMA_REQUEST;
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
    RCC_PeriphCLKInitTypeDef rccInit;

    /* Enable GPIO TX/RX clock */
    DBG_UART_TX_GPIO_CLK_ENABLE();
    DBG_UART_RX_GPIO_CLK_ENABLE();

    /* Select clock source for the USART clocks */
    rccInit.PeriphClockSelection = DBG_UART_CLK_SELECTION;
    DBG_UART_RCC_SELECTION = DBG_UART_CLK_SOURCE;
    HAL_RCCEx_PeriphCLKConfig(&rccInit);

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

    /* Enable FE/NE/ORE as it can trigger when debug cable is hot disconnected/reconnected */
    __HAL_UART_ENABLE_IT(huart, UART_IT_FE | UART_IT_NE | UART_IT_ORE);

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

    case GPIOI_BASE:
        __HAL_RCC_GPIOI_CLK_ENABLE();
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
                gpioInit.Pin = InputGPIOs[index].pin;
                if (M_CheckAnalogMode(InputGPIOs[index].pullMode))
                {
                    gpioInit.Mode = GPIO_MODE_ANALOG;
                }
                else
                {
                    gpioInit.Mode = GPIO_MODE_INPUT;
                }
                gpioInit.Pull  = InputGPIOs[index].pullMode & GPIO_IN_PULLMODE_MASK;
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
    /* STM32H7xx series timers: */
    /* 32-Bit Timers: 2, 5 */
    /* 16-Bit Timers: 1, 3, 4, 6-14 */
    /* Adv. Control Timers: 1, 8 */
    /* Basic Timers: 6, 7 */
    /* Gen purpose: 2,3,4,5,9-14 */
    /* Timer 1,8,15,16,17 are APB2 Peripherals */
    if ((pTim == TIM1) || (pTim == TIM8) || (pTim == TIM15) || (pTim == TIM16) || (pTim == TIM17))
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
    else    /* Timer 2-7, 12-14 are APB1 */
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
**     SYSCLK(Hz)                     = 400000000 (CPU Clock)
**     HCLK(Hz)                       = 200000000 (AXI and AHBs Clock)
**     AHB Prescaler                  = 2
**     D1 APB3 Prescaler              = 2 (APB3 Clock 100MHz)
**     D2 APB1 Prescaler              = 2 (APB1 Clock 100MHz)
**     D2 APB2 Prescaler              = 2 (APB2 Clock 100MHz)
**     D3 APB3 Prescaler              = 2 (APB4 Clock 100MHz)
**     APB2 Prescaler                 = 2
**     HSE Frequency(Hz)              = 8000000 (ext clock from STLink)
**     PLL_M                          = 4
**     PLL_N                          = 400
**     PLL_P                          = 2
**     PLL_Q                          = 4
**     PLL_R                          = 2
**     VDD(V)                         = 3.3
**     Main regulator output voltage  = Scale1 mode
**     Flash Latency(WS)              = 4
**
** @param  None
**
** @return None
*/
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef oscInit;
    RCC_ClkInitTypeDef clkInit;
    HAL_StatusTypeDef ret;

    /* Supply configuration update enable */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    /* The voltage scaling allows optimizing the power consumption when the device is
       clocked below the maximum system frequency, to update the voltage scaling value
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    oscInit.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    oscInit.HSEState = RCC_HSE_BYPASS;
    oscInit.HSIState = RCC_HSI_OFF;
    oscInit.CSIState = RCC_CSI_OFF;
    oscInit.PLL.PLLState  = RCC_PLL_ON;
    oscInit.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    oscInit.PLL.PLLM = PLL_M;
    oscInit.PLL.PLLN = PLL_N;
    oscInit.PLL.PLLP = PLL_P;
    oscInit.PLL.PLLQ = PLL_Q;
    oscInit.PLL.PLLR = PLL_R;

    oscInit.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    oscInit.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
    oscInit.PLL.PLLFRACN = 0;
    ret = HAL_RCC_OscConfig(&oscInit);
    ASF_assert(ret == HAL_OK);

    /* Select PLL as system clock source and configure  bus clocks dividers */
    clkInit.ClockType      = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_PCLK1 |
                              RCC_CLOCKTYPE_PCLK2  | RCC_CLOCKTYPE_D3PCLK1);
    clkInit.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clkInit.SYSCLKDivider  = RCC_SYSCLK_DIV1;
    clkInit.AHBCLKDivider  = RCC_HCLK_DIV2;
    clkInit.APB3CLKDivider = RCC_APB3_DIV2;
    clkInit.APB1CLKDivider = RCC_APB1_DIV2;
    clkInit.APB2CLKDivider = RCC_APB2_DIV2;
    clkInit.APB4CLKDivider = RCC_APB4_DIV2;

    ret = HAL_RCC_ClockConfig(&clkInit, FLASH_LATENCY_4);
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
** @brief Prints the status of all GPIO input signals defined under "InputGPIOs"
**
** @param None
**
** @return None
*/
void DumpGpioInputStatusAll(void)
{
    uint16_t index;
    uint16_t hwRevMask = (1 << gHardwareRev) | HW_REV_COMMON;

    D0_printf(G_BLACK_BOLD"All GPIO Inputs State:\r\n"G_NORM);
    for (index = 0; index < NUM_GPIO_INPUTS; index++)
    {
        if (InputGPIOs[index].hwCompat & hwRevMask)
        {
            /* Analog mode signals are prefixed by '*' and their status read as digital input */
            D0_printf("\t%c%15s : %u\r\n", InputGPIOs[index].pullMode == GPIO_IN_MODE_ANALOG ? '*' : ' ',
                InputGPIOs[index].schRef, GPIO_GetInputState(index));
        }
    }
}

/***************************************************************************************************
** @brief Prints the status of all GPIO output signals defined under "OutputGPIOs"
**
** @param None
**
** @return None
*/
void DumpGpioOutputStatusAll(void)
{
    uint16_t index;
    uint16_t hwRevMask = (1 << gHardwareRev) | HW_REV_COMMON;

    D0_printf(G_BLACK_BOLD"All GPIO Outputs State:\r\n"G_NORM);
    for (index = 0; index < NUM_GPIO_OUTPUTS; index++)
    {
        if (OutputGPIOs[index].hwCompat & hwRevMask)
        {
            D0_printf("\t%16s : %u\r\n", OutputGPIOs[index].schRef, GPIO_GetOutputState(index));
        }
    }
}

/***************************************************************************************************
** @brief Returns the status of a given GPIO signal name (schematic reference)
**
** @param schRef: Schematic reference name
**
** @return GPIO_STATE_LOW or GPIO_STATE_HIGH if GPIO name found; GPIO_INVALID if not found
*/
GpioState_t GetGpioStateByName(const char* schRef)
{
    uint16_t index;
    uint16_t hwRevMask = (1 << gHardwareRev) | HW_REV_COMMON;

    if (!schRef)
    {
        return GPIO_INVALID;
    }

    for (index = 0; index < NUM_GPIO_OUTPUTS; index++)
    {
        if (strcmp(OutputGPIOs[index].schRef, schRef) == 0)
        {
            if (!(OutputGPIOs[index].hwCompat & hwRevMask))
            {
                return GPIO_INVALID;
            }
            return HAL_GPIO_ReadPin(OutputGPIOs[index].grp, OutputGPIOs[index].pin) == GPIO_PIN_SET ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
        }
    }

    for (index = 0; index < NUM_GPIO_INPUTS; index++)
    {
        if (strcmp(InputGPIOs[index].schRef, schRef) == 0)
        {
            if (!(InputGPIOs[index].hwCompat & hwRevMask))
            {
                return GPIO_INVALID;
            }
            return HAL_GPIO_ReadPin(InputGPIOs[index].grp, InputGPIOs[index].pin) == GPIO_PIN_SET ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
        }
    }

    return GPIO_INVALID;
}

/***************************************************************************************************
** @brief Sets the state of a given GPIO output or LED signal name (schematic reference)
**
** @param schRef: Schematic reference name
** @param state: GPIO state (GPIO_STATE_LOW or GPIO_STATE_HIGH)
**
** @return None
*/
void SetGpioStateByName(const char* schRef, GpioState_t state)
{
    uint16_t index;
    uint16_t hwRevMask = (1 << gHardwareRev) | HW_REV_COMMON;

    if (schRef)
    {
        for (index = 0; index < NUM_GPIO_OUTPUTS; index++)
        {
            if (strcmp(OutputGPIOs[index].schRef, schRef) == 0)
            {
                if (!(OutputGPIOs[index].hwCompat & hwRevMask))
                {
                    D0_printf(G_RED"\t%s Not supported in current HW\r\n"G_NORM, schRef);
                    return;
                }
                else if (state > GPIO_STATE_LOW)
                {
                    D0_printf("\tNew %s state: HIGH\r\n", OutputGPIOs[index].schRef);
                    GPIO_SetHigh(index);
                    return;
                }
                else
                {
                    D0_printf("\tNew %s state: LOW\r\n", OutputGPIOs[index].schRef);
                    GPIO_SetLow(index);
                    return;
                }
            }
        }
        for (index = 0; index < NUM_LEDS; index++)
        {
            if (strcmp(DiagLEDs[index].schRef, schRef) == 0)
            {
                if (!(DiagLEDs[index].hwCompat & hwRevMask))
                {
                    D0_printf(G_RED"\t%s Not supported in current HW\r\n"G_NORM, schRef);
                    return;
                }
                else if (state > GPIO_STATE_LOW)
                {
                    D0_printf("\tNew %s state: ON\r\n", DiagLEDs[index].schRef);
                    LED_On(index);
                    return;
                }
                else
                {
                    D0_printf("\tNew %s state: OFF\r\n", DiagLEDs[index].schRef);
                    LED_Off(index);
                    return;
                }
            }
        }
        D0_printf("\t%s not found!\r\n", schRef);
    }
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
    __HAL_TIM_SET_COUNTER(&s_hRtcTimer, RTC_INITIAL_COUNT);
    s_hRtcTimer.Init.Period = 0xFFFFFFFF;
    s_hRtcTimer.Init.Prescaler = prescalarVal;
    s_hRtcTimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
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

    /* Add other Uart IF MSP init calls below */
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
    /* Disable OverRun Error detection */
    gDbgUartPort.hUart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_RXOVERRUNDISABLE_INIT;
    gDbgUartPort.hUart->AdvancedInit.OverrunDisable = UART_ADVFEATURE_OVERRUN_DISABLE;

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

    /* Add handlers for other Uart IF */
}

/***************************************************************************************************
 ** @brief Configure the MPU attributes for Ethernet buffers and descriptors
 **
 ** @param None
 **
 ** @return None
 */
void MPU_Config(void)
{
    MPU_Region_InitTypeDef mpuInit;
#ifdef __ICCARM__
    uint32_t sram1StartAddr = (uint32_t)&__region_SRAM1_start__;
    uint32_t sram2StartAddr = (uint32_t)&__region_SRAM2_start__;
#elif defined (__CC_ARM) || defined (__clang__)  /* Keil C/Clang compilers */
    uint32_t sram1StartAddr = (uint32_t)&Image$$RW_IRAM1$$Base;
    uint32_t sram2StartAddr = (uint32_t)&Image$$RW_IRAM2$$Base;
#elif defined (__GNUC__)
#error Not implemented for GCC yet
#endif
    extern const osPoolDef_t* gDbgPrintPool;

    /* Disable the MPU */
    HAL_MPU_Disable();

    /* Configure the MPU as Strongly ordered for not defined regions */
    mpuInit.Enable = MPU_REGION_ENABLE;
    mpuInit.BaseAddress = 0x00;
    mpuInit.Size = MPU_REGION_SIZE_4GB;
    mpuInit.AccessPermission = MPU_REGION_NO_ACCESS;
    mpuInit.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
    mpuInit.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    mpuInit.IsShareable = MPU_ACCESS_SHAREABLE;
    mpuInit.Number = MPU_REGION_NUMBER0;
    mpuInit.TypeExtField = MPU_TEX_LEVEL0;
    mpuInit.SubRegionDisable = 0x87;
    mpuInit.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

    HAL_MPU_ConfigRegion(&mpuInit);

    /* Configure IRAM1 region as Normal "Write-back, write and read allocate" for whole of SRAM1 */
    mpuInit.Enable           = MPU_REGION_ENABLE;
    mpuInit.BaseAddress      = sram1StartAddr;
    mpuInit.Size             = MPU_REGION_SIZE_128KB;
    mpuInit.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpuInit.IsBufferable     = MPU_ACCESS_BUFFERABLE;
    mpuInit.IsCacheable      = MPU_ACCESS_CACHEABLE;
    mpuInit.IsShareable      = MPU_ACCESS_SHAREABLE;
    mpuInit.Number           = MPU_REGION_NUMBER1; /* See Note below */
    mpuInit.TypeExtField     = MPU_TEX_LEVEL1;
    mpuInit.SubRegionDisable = 0x00;
    mpuInit.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&mpuInit);

    /* Configure IRAM2 region as Normal "Write-back, write and read allocate" for whole of SRAM1 */
    mpuInit.Enable           = MPU_REGION_ENABLE;
    mpuInit.BaseAddress      = sram2StartAddr;
    mpuInit.Size             = MPU_REGION_SIZE_512KB;
    mpuInit.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpuInit.IsBufferable     = MPU_ACCESS_BUFFERABLE;
    mpuInit.IsCacheable      = MPU_ACCESS_CACHEABLE;
    mpuInit.IsShareable      = MPU_ACCESS_SHAREABLE;
    mpuInit.Number           = MPU_REGION_NUMBER1; /* See Note below */
    mpuInit.TypeExtField     = MPU_TEX_LEVEL1;
    mpuInit.SubRegionDisable = 0x00;
    mpuInit.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&mpuInit);

    /* Configure a MPU region as Normal "Write thru, no write allocate" for debug print buffers in SRAM1 */
    mpuInit.Enable           = MPU_REGION_ENABLE;
    mpuInit.BaseAddress      = (uint32_t)gDbgPrintPool->pool;
    mpuInit.Size             = MPU_REGION_SIZE_16KB;
    mpuInit.AccessPermission = MPU_REGION_FULL_ACCESS;
    mpuInit.IsBufferable     = MPU_ACCESS_NOT_BUFFERABLE;
    mpuInit.IsCacheable      = MPU_ACCESS_CACHEABLE;
    mpuInit.IsShareable      = MPU_ACCESS_SHAREABLE;
    mpuInit.Number           = MPU_REGION_NUMBER4;
    mpuInit.TypeExtField     = MPU_TEX_LEVEL0;
    mpuInit.SubRegionDisable = 0x00;
    mpuInit.DisableExec      = MPU_INSTRUCTION_ACCESS_ENABLE;

    HAL_MPU_ConfigRegion(&mpuInit);

    /* Enable the MPU */
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

 /***************************************************************************************************
 ** @brief Enable CPU L1-Cache.
 **
 ** @param None
 **
 ** @return None
 */
void CPU_CACHE_Enable(void)
{
    SCB_InvalidateICache();
    SCB_InvalidateDCache();

    /* Enable I-Cache */
    SCB_EnableICache();

    /* Enable D-Cache */
    SCB_EnableDCache();
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

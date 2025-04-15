/* OSP Hello World Project
 * https://github.com/vermar/open-sensor-platform
 *
 * Copyright (C) 2024 Rajiv Verma
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
#if !defined (HW_SETUP_NUCLEO_F413_H)
#define   HW_SETUP_NUCLEO_F413_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "osp-types.h"

//==================================================================================================
//    C O N S T A N T S   &   M A C R O S
//==================================================================================================
/* ########################################################################## */
/* #    H W   V E R S I O N   I D E N T I F I E R S                         # */
/* ########################################################################## */
/* The following defines are bit masks that can be combined for declarations that are compatible
 * See GPIO definitions in hw_setup.c file
 */
#define HW_REV_COMMON                           1
#define HW_REV_1                                (1 << 1)
#define HW_REV_2                                (1 << 2)
#define HW_REV_3                                (1 << 3)
#define HW_REV_4                                (1 << 4)

/* ########################################################################## */
/* #    T I M I N G S   A N D   T I M E R S                                 # */
/* ########################################################################## */
/** System clock & tick configuration */
#define SYSTEM_CLOCK_FREQ                       100000000  //Make sure this matches the settings in system_stm32fxx.c
#define USEC_PER_TICK                           5000       ///< (in uS) = 200Hz
#define TICS_PER_SEC                            ((uint32_t)(1000000/USEC_PER_TICK))
#define MSEC_PER_TICK                           ((uint32_t)(USEC_PER_TICK/1000))

/** RTC Configuration */
/* Note Timer-2 (32-bit) is used as free running counter instead of RTC peripheral */
#define US_PER_RTC_TICK                         10  //micro-seconds
#define RTC_PRESCALAR                           (uint32_t)((((uint64_t)SystemCoreClock * US_PER_RTC_TICK) / 1000000) - 1)
#define RTC_TIMER                               TIM2
#define RTC_TIMER_CLK_EN()                      __HAL_RCC_TIM2_CLK_ENABLE()

/* Tick conversion macros */
#define TICS_TO_SEC(T)                          ((uint32_t)(((T) + (TICS_PER_SEC/2))/TICS_PER_SEC))
#define SEC_TO_TICS(S)                          ((uint32_t)((S) * TICS_PER_SEC))
#define MSEC_TO_TICS(M)                         ((uint32_t)(((M) + (MSEC_PER_TICK-1))/MSEC_PER_TICK))

/* ########################################################################## */
/* #    F L A S H  S T O R A G E                                            # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    G P I O  I N T E R F A C E S / A S S I G N M E N T S                # */
/* ########################################################################## */
/* Diagnostic GPIOs */
/* Note: For each GPIO enum defined here there should be a corresponding initialization
 * of OutputGPIOs[] struct array in hw_setup.c file in the order as these enums.
 */
enum _GpioOutputs
{
    GPIO_USB_PWR_EN,     //USB_FS_PWR_EN
    NUM_GPIO_OUTPUTS
};

enum _GpioInputs
{
    GPIO_IN_USR_BTN,         //User blue button
    NUM_GPIO_INPUTS
};

/* Diagnostic LEDs on this board */
enum _Leds {
    LED_GREEN,
    LED_RED,
    LED_YELLOW,
    NUM_LEDS
};


/* User Friendly LED designation - unused ones should be assigned 0xFF */
#define FRONT_LED                               LED_GREEN
#define HARD_FAULT_LED                          LED_RED

#define LED_On(led)                                  \
    if (led < NUM_LEDS)                              \
    {                                                \
        DiagLEDs[led].grp->BSRR = DiagLEDs[led].pin; \
    }

#define LED_Off(led)                                                 \
    if (led < NUM_LEDS)                                              \
    {                                                                \
        DiagLEDs[led].grp->BSRR = (uint32_t)DiagLEDs[led].pin << 16; \
    }

#define GPIO_SetHigh(gpio)                                   \
    if (gpio < NUM_GPIO_OUTPUTS)                             \
    {                                                        \
        OutputGPIOs[gpio].grp->BSRR = OutputGPIOs[gpio].pin; \
    }

#define GPIO_SetLow(gpio)                                                    \
    if (gpio < NUM_GPIO_OUTPUTS)                                             \
    {                                                                        \
        OutputGPIOs[gpio].grp->BSRR = (uint32_t)OutputGPIOs[gpio].pin << 16; \
    }

#define LED_Toggle(led)                              \
    if (led < NUM_LEDS)                              \
    {                                                \
        DiagLEDs[led].grp->ODR ^= DiagLEDs[led].pin; \
    }

#define RCC_GPIO_CLK_ENABLE(rcc_gpio)                      \
    do                                                     \
    {                                                      \
        __IO uint32_t tmpreg = 0x00;                       \
        SET_BIT(RCC->AHB1ENR, rcc_gpio);                   \
        /* Delay after an RCC peripheral clock enabling */ \
        tmpreg = READ_BIT(RCC->AHB1ENR, rcc_gpio);         \
        UNUSED(tmpreg);                                    \
    } while (0)

/* GPIO Input Macros for reading values */
#define GPIO_GetState(gpioIn)                              \
    HAL_GPIO_ReadPin(InputGPIOs[gpioIn].grp, InputGPIOs[gpioIn].pin)

/* Assert LED assignment */
#define AssertIndication()                      LED_On(LED_RED)

/* ########################################################################## */
/* #    U A R T  I N T E R F A C E S                                        # */
/* ########################################################################## */
/** Debug Console UART configuration */
#define DBG_UART_BAUD                           921600
#define DBG_IF_UART                             USART3
#define DBG_UART_CLK_ENABLE()                   __HAL_RCC_USART3_CLK_ENABLE();
#define DBG_UART_RX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DBG_UART_TX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DBG_UART_DMAx_CLK_ENABLE()              __HAL_RCC_DMA1_CLK_ENABLE()
#define DBG_UART_CLK_SELECTION                  RCC_PERIPHCLK_USART3
#define DBG_UART_CLK_SOURCE                     RCC_USART3CLKSOURCE_SYSCLK
#define DBG_UART_RCC_SELECTION                  rccInit.Usart234578ClockSelection

#define DBG_UART_FORCE_RESET()                  __HAL_RCC_USART3_FORCE_RESET()
#define DBG_UART_RELEASE_RESET()                __HAL_RCC_USART3_RELEASE_RESET()

/* Definition for USARTx Pins */
#define DBG_UART_TX_PIN                         GPIO_PIN_8
#define DBG_UART_TX_GPIO_PORT                   GPIOD
#define DBG_UART_TX_AF                          GPIO_AF7_USART3
#define DBG_UART_RX_PIN                         GPIO_PIN_9
#define DBG_UART_RX_GPIO_PORT                   GPIOD
#define DBG_UART_RX_AF                          GPIO_AF7_USART3

/* Definition for USARTx's DMA */
#define DBG_UART_TX_DMA_STREAM                  DMA1_Stream3
#define DBG_UART_RX_DMA_STREAM                  DMA1_Stream1

#define DBG_UART_TX_DMA_CHANNEL                 DMA_CHANNEL_4
#define DBG_UART_RX_DMA_CHANNEL                 DMA_CHANNEL_4

/* Definition for USARTx's NVIC */
#define DBG_UART_DMA_TX_IRQn                    DMA1_Stream3_IRQn
#define DBG_UART_DMA_RX_IRQn                    DMA1_Stream1_IRQn
#define DBG_UART_DMA_TX_IRQHandler              DMA1_Stream3_IRQHandler
#define DBG_UART_DMA_RX_IRQHandler              DMA1_Stream1_IRQHandler

/* Definition for USARTx's NVIC */
#define DBG_UART_IRQn                           USART3_IRQn
#define DBG_UART_IRQHandler                     USART3_IRQHandler

/* Flush macro used in assert */
#ifdef UART_DMA_ENABLE
# define FlushUart()                            HAL_UART_DMAStop(gDbgUartPort.hUart)
#else
# define FlushUart()
#endif

/* ########################################################################## */
/* #    S P I   I N T E R F A C E S                                         # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I 2 C   I N T E R F A C E S                                         # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    E X T E R N A L   I N P U T   I N T E R R U P T S                   # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I N T E R R U P T  P R I O R I T Y   A S S I G N M E N T S          # */
/* ########################################################################## */
/* Note: HAL initialization sets NVIC_PRIORITYGROUP_4. It can be overridden here by defining
 * SYSTEM_INTERRUPT_PRIORITY_GRP. Peripheral priorities would need to be changed accordingly
 * Interrupt Channels, Group & Priority assignments:
 * Note that lower number = higher priority (subpriority). All priorities are in relationship to
 * each other & the priority group so changing one may affect others.
 */
#define DBG_UART_DMA_INT_PREEMPT_PRIORITY       9   // Uart DMA channel preemption prio. group
#define DBG_UART_TX_DMA_INT_SUB_PRIORITY        0   // Uart DMA channel subpriority within group

#define DBG_UART_INT_PREEMPT_PRIORITY           10  // Lowest group Preemption priority
#define DBG_UART_INT_SUB_PRIORITY               0   // Lowest Priority within group

/* ########################################################################## */
/* #    M I S C E L L A N E O U S                                           # */
/* ########################################################################## */
/* Device Unique ID register for STM32F4 series devices */
#define DEV_UID_OFFSET                          0x1FFF7A10
#define DBG_MCU_IDCODE_OFFSET                   0xE0042000

//==================================================================================================
//    T Y P E   D E F I N I T I O N S
//==================================================================================================
typedef DMA_HandleTypeDef   DMAhandle_t;
typedef UART_HandleTypeDef  UARThandle_t;

typedef union DeviceUidTag
{
    uint32_t uidWords[3];
    uint8_t  uidBytes[12];
} DeviceUid_t;

typedef enum _GPIO_State
{
    GPIO_STATE_LOW,
    GPIO_STATE_HIGH
} GpioState_t;

typedef struct _GPIO_OutputInfo
{
    GPIO_TypeDef*   grp;
    uint16_t        pin;
    GpioState_t     initState; //LOW/HIGH
    uint16_t        hwCompat;  //Compatibility mask
} GPioOutputInfo_t;

typedef struct _GPIO_InputInfo
{
    GPIO_TypeDef*   grp;
    uint16_t        pin;
    uint32_t        pullMode;  //Pull-up/Pull-down/No-pull
    uint16_t        hwCompat;  //Compatibility mask
} GPioInputInfo_t;

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
extern DeviceUid_t *gDevUniqueId;
extern GPioOutputInfo_t DiagLEDs[NUM_LEDS];
extern GPioOutputInfo_t OutputGPIOs[NUM_GPIO_OUTPUTS];
extern GPioInputInfo_t InputGPIOs[NUM_GPIO_INPUTS];

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================
void SystemClock_Config( void );
void SystemGPIOConfig( void );
void SystemInterruptConfig( void );
void DebugUARTConfig( uint32_t baud, uint32_t dataLen, uint32_t stopBits, uint32_t parity );
void LED_Init( uint16_t hwRev );


#endif /* HW_SETUP_NUCLEO_F413_H */
//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

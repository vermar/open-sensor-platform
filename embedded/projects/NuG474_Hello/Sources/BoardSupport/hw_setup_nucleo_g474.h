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
#if !defined (HW_SETUP_NUCLEO_G4_H)
#define   HW_SETUP_NUCLEO_G4_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "stm32g4xx.h"
#include "stm32g4xx_hal.h"
#include "osp-types.h"

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/* ########################################################################## */
/* #    T I M I N G S                                                       # */
/* ########################################################################## */
/** System clock & tick configuration */
#define SYSTEM_CLOCK_FREQ                       170000000  //Make sure this matches the settings in system_stm32xxxx.c
#define USEC_PER_TICK                           5000       ///< (in uS) = 200Hz
#define TICS_PER_SEC                            ((uint32_t)(1000000/USEC_PER_TICK))
#define MSEC_PER_TICK                           ((uint32_t)(USEC_PER_TICK/1000))

/** RTC Configuration */
/* Note Timer-2 is used as free running counter instead of RTC */
#define US_PER_RTC_TICK                         25  //micro-seconds
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
/* #    D I A G N O S T I C  (LED/GPIOs) I N T E R F A C E                  # */
/* ########################################################################## */
/* Diagnostic GPIOs */

/* Diagnostic LEDs on this board */
enum _Leds {
    LED_LD2,    //Green
    NUM_LEDS
};


/* User Friendly LED designation - unused ones should be assigned 0xFF */
#define FRONT_LED                               LED_GREEN
#define LED_GREEN                               LED_LD2
#define HARD_FAULT_LED                          LED_GREEN

#define LED_On(led)                                     \
    if (led < NUM_LEDS) {                               \
         DiagLEDs[led].grp->BSRR = DiagLEDs[led].pin;   \
    }

#define LED_Off(led)                                    \
    if (led < NUM_LEDS) {                               \
         DiagLEDs[led].grp->BRR = DiagLEDs[led].pin;    \
    }

#define LED_Toggle(led)                                 \
    if (led < NUM_LEDS) {                               \
        DiagLEDs[led].grp->ODR ^= DiagLEDs[led].pin;    \
    }

#define RCC_GPIO_CLK_ENABLE(rcc_gpio)                       \
    do {                                                    \
        __IO uint32_t tmpreg = 0x00;                        \
        SET_BIT(RCC->AHB2ENR, rcc_gpio);                    \
        /* Delay after an RCC peripheral clock enabling */  \
        tmpreg = READ_BIT(RCC->AHB2ENR, rcc_gpio);          \
        UNUSED(tmpreg);                                     \
    } while(0)

/* Assert LED assignment */
#define AssertIndication()                      LED_Off(FRONT_LED)

/* ########################################################################## */
/* #    U A R T  I N T E R F A C E                                          # */
/* ########################################################################## */
/** UART configuration */
/* Definition for USARTx clock resources */
#define DBG_UART_BAUD                           921600

/* Note: On Nucleo Board USART1 (PA2, PA3) is connected to STLINK for debug output via USB */
#define DBG_IF_UART                             USART1
#define DBG_UART_CLK_ENABLE()                   __HAL_RCC_USART1_CLK_ENABLE();
#define DBG_UART_RX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define DBG_UART_TX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOC_CLK_ENABLE()
#define DMAx_CLK_ENABLE()                       __HAL_RCC_DMA1_CLK_ENABLE(); \
                                                __HAL_RCC_DMAMUX1_CLK_ENABLE()
//#define DBG_UART_CLK_SELECTION                  RCC_PERIPHCLK_USART2
//#define DBG_UART_CLK_SOURCE                     RCC_USART3CLKSOURCE_SYSCLK

#define DBG_UART_FORCE_RESET()                  __HAL_RCC_USART1_FORCE_RESET()
#define DBG_UART_RELEASE_RESET()                __HAL_RCC_USART1_RELEASE_RESET()

/* Definition for USARTx Pins */
#define DBG_UART_TX_PIN                         GPIO_PIN_4
#define DBG_UART_TX_GPIO_PORT                   GPIOC
#define DBG_UART_TX_AF                          GPIO_AF7_USART1
#define DBG_UART_RX_PIN                         GPIO_PIN_5
#define DBG_UART_RX_GPIO_PORT                   GPIOC
#define DBG_UART_RX_AF                          GPIO_AF7_USART1

/* Definition for USARTx's DMA */
#define DBG_UART_TX_DMA_REQUEST                 DMA_REQUEST_USART1_TX
#define DBG_UART_RX_DMA_REQUEST                 DMA_REQUEST_USART1_RX

#define DBG_UART_TX_DMA_CHANNEL                 DMA1_Channel1   //Ch. assignment is not related to peripheral
//#define DBG_UART_RX_DMA_CHANNEL                 DMA1_Channel2

/* Definition for USARTx's NVIC */
#define DBG_UART_DMA_TX_IRQn                    DMA1_Channel1_IRQn
//#define DBG_UART_DMA_RX_IRQn                    DMA1_Channel2_IRQn
#define DBG_UART_DMA_TX_IRQHandler              DMA1_Channel1_IRQHandler
//#define DBG_UART_DMA_RX_IRQHandler              DMA1_Channel2_IRQHandler

/* Definition for USARTx's NVIC */
#define DBG_UART_IRQn                           USART1_IRQn
#define DBG_UART_IRQHandler                     USART1_IRQHandler

/* Flush macro used in assert */
#ifdef UART_DMA_ENABLE
# define FlushUart()                            HAL_UART_DMAStop(gDbgUartPort.hUart)
#else
# define FlushUart()
#endif

/* ########################################################################## */
/* #    S P I   I N T E R F A C E                                           # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I 2 C   I N T E R F A C E                                           # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I N T E R R U P T  A S S I G N M E N T S                            # */
/* ########################################################################## */
/** Interrupt Channels, Group & Priority assignments:
 Note that lower number = higher priority (subpriority). All priorities are in relationship to
 each other & the priority group so changing one may affect others.
 */
#define DBG_UART_DMA_INT_PREEMPT_PRIORITY       9   // Uart DMA channel preemption prio. group
#define DBG_UART_TX_DMA_INT_SUB_PRIORITY        0   // Uart DMA channel subpriority within group

#define DBG_UART_INT_PREEMPT_PRIORITY           10  // Lowest group Preemption priority
#define DBG_UART_INT_SUB_PRIORITY               0   // Lowest Priority within group

/* ########################################################################## */
/* #    M I S C E L L A N E O U S                                           # */
/* ########################################################################## */
/* Device Unique ID register for STM32G4 series devices */
#define DEV_UID_OFFSET                          0x1FFF7590
#define DBG_MCU_IDCODE_OFFSET                   0xE0042000

/* ########################################################################## */
/* #    U S B  I N T E R F A C E                                            # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
typedef DMA_HandleTypeDef   DMAhandle_t;
typedef UART_HandleTypeDef  UARThandle_t;

typedef union DeviceUidTag
{
    uint32_t uidWords[3];
    uint8_t  uidBytes[12];
} DeviceUid_t;


typedef struct _GPIO_info {
    uint32_t        rccPeriph;
    GPIO_TypeDef*   grp;
    uint16_t        pin;
} GPioPinInfo_t;


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern DeviceUid_t *gDevUniqueId;
extern GPioPinInfo_t DiagLEDs[NUM_LEDS];

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
void SystemClock_Config( void );
void SystemGPIOConfig( void );
void SystemInterruptConfig( void );
void DebugUARTConfig( uint32_t baud, uint32_t dataLen, uint32_t stopBits, uint32_t parity );
void LED_Init( void );


#endif /* HW_SETUP_NUCLEO_G4_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

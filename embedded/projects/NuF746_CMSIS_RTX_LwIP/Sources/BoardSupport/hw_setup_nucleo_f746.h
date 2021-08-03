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
#if !defined (HW_SETUP_NUCLEO_F746_H)
#define   HW_SETUP_NUCLEO_F746_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f7xx.h"
#include "stm32f7xx_hal.h"
#include "osp-types.h"
#include <stddef.h>

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/* ########################################################################## */
/* #    T I M I N G S                                                       # */
/* ########################################################################## */
/** System clock & tick configuration */
#define SYSTEM_CLOCK_FREQ                       216000000
#define USEC_PER_TICK                           1000       ///< (in uS) = 1KHz
#define TICS_PER_SEC                            ((uint32_t)(1000000/USEC_PER_TICK))
#define MSEC_PER_TICK                           ((uint32_t)(USEC_PER_TICK/1000))

/** RTC Configuration */
/* Note Timer-2 is used as free running counter instead of RTC */
#define US_PER_RTC_TICK                         25  //micro-seconds
#define RTC_PRESCALAR                           (uint32_t)((((uint64_t)SystemCoreClock * US_PER_RTC_TICK) / 1000000) - 1)
#define RTC_TIMER                               TIM2
#define RTC_TIMER_CLK_EN()                      __HAL_RCC_TIM2_CLK_ENABLE()

/* Tick conversion macros */
#define TICS_TO_SEC(T)                          ((uint32_t)(((T) + (TICS_PER_SEC/2))/TICS_PER_SEC))
#define SEC_TO_TICS(S)                          ((uint32_t)((S) * TICS_PER_SEC))
#define MSEC_TO_TICS(M)                         ((uint32_t)(((M) + (MSEC_PER_TICK-1))/MSEC_PER_TICK))


/* ########################################################################## */
/* #    D I A G N O S T I C  (LED/GPIOs) I N T E R F A C E                  # */
/* ########################################################################## */
/* Diagnostic GPIOs */

/* Diagnostic LEDs on this board */
enum _Leds {
    LED_LD1,
    LED_LD2,
    LED_LD3,
    NUM_LEDS
};


/* User Friendly LED designation - unused ones should be assigned 0xFF */
#define FRONT_LED                               LED_GREEN
#define LED_GREEN                               LED_LD1
#define LED_BLUE                                LED_LD2
#define LED_RED                                 LED_LD3
#define HARD_FAULT_LED                          LED_RED

#define LED_On(led)                                     \
    if (led < NUM_LEDS) {                               \
         DiagLEDs[led].grp->BSRR = DiagLEDs[led].pin;   \
    }

#define LED_Off(led)                                    \
    if (led < NUM_LEDS) {                               \
         DiagLEDs[led].grp->BSRR = (uint32_t)DiagLEDs[led].pin << 16;   \
    }

#define LED_Toggle(led)                                 \
    if (led < NUM_LEDS) {                               \
        DiagLEDs[led].grp->ODR ^= DiagLEDs[led].pin;    \
    }

#define RCC_GPIO_CLK_ENABLE(rcc_gpio)                       \
    do {                                                    \
        __IO uint32_t tmpreg = 0x00;                        \
        SET_BIT(RCC->AHB1ENR, rcc_gpio);                    \
        /* Delay after an RCC peripheral clock enabling */  \
        tmpreg = READ_BIT(RCC->AHB1ENR, rcc_gpio);          \
        UNUSED(tmpreg);                                     \
    } while(0)

/* Assert LED assignment */
#define AssertIndication()                      LED_On(LED_BLUE)

/* ########################################################################## */
/* #    U A R T  I N T E R F A C E                                          # */
/* ########################################################################## */
/** UART configuration */
/* Definition for USARTx clock resources */
#define DBG_UART_BAUD                           921600
#define DBG_IF_UART                             USART3
#define DBG_UART_CLK_ENABLE()                   __HAL_RCC_USART3_CLK_ENABLE();
#define DBG_UART_RX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DBG_UART_TX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DMAx_CLK_ENABLE()                       __HAL_RCC_DMA1_CLK_ENABLE()
#define DBG_UART_CLK_SELECTION                  RCC_PERIPHCLK_USART3
#define DBG_UART_CLK_SOURCE                     RCC_USART3CLKSOURCE_SYSCLK

#define DBG_UART_FORCE_RESET()                  __HAL_RCC_USART3_FORCE_RESET()
#define DBG_UART_RELEASE_RESET()                __HAL_RCC_USART3_RELEASE_RESET()

/* Definition for USARTx Pins */
#define DBG_UART_TX_PIN                         GPIO_PIN_8
#define DBG_UART_TX_GPIO_PORT                   GPIOD
#define DBG_UART_TX_AF                          GPIO_AF7_USART3
#define DBG_UART_RX_PIN                         GPIO_PIN_9
#define DBG_UART_RX_GPIO_PORT                   GPIOD
#define DBG_UART_RX_AF                          GPIO_AF7_USART3

/* Definition for USARTx's DMA (see DM00124865.pdf) */
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
/* #    S P I   I N T E R F A C E                                           # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I 2 C   I N T E R F A C E                                           # */
/* ########################################################################## */
/* Definition for I2Cx clock resources */
#define I2Cx                            I2C1
#define RCC_PERIPHCLK_I2Cx              RCC_PERIPHCLK_I2C1
#define RCC_I2CxCLKSOURCE_SYSCLK        RCC_I2C1CLKSOURCE_PCLK1
#define I2Cx_CLK_ENABLE()               __HAL_RCC_I2C1_CLK_ENABLE()
#define I2Cx_SDA_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()
#define I2Cx_SCL_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOB_CLK_ENABLE()

#define I2Cx_FORCE_RESET()              __HAL_RCC_I2C1_FORCE_RESET()
#define I2Cx_RELEASE_RESET()            __HAL_RCC_I2C1_RELEASE_RESET()

#define I2C_ADDRESS                     0x3E

/* Definition for I2Cx Pins */
#define I2Cx_SCL_PIN                    GPIO_PIN_8
#define I2Cx_SCL_GPIO_PORT              GPIOB
#define I2Cx_SDA_PIN                    GPIO_PIN_9
#define I2Cx_SDA_GPIO_PORT              GPIOB
#define I2Cx_SCL_SDA_AF                 GPIO_AF4_I2C1

/* Definition for I2Cx's NVIC */
#define I2Cx_EV_IRQn                    I2C1_EV_IRQn
#define I2Cx_ER_IRQn                    I2C1_ER_IRQn

#ifdef MASTER_BOARD
#define I2C_Driver_ISR_Handler          I2C1_EV_IRQHandler
#define I2C_Driver_ERR_ISR_Handler      I2C1_ER_IRQHandler
#else
#define I2Cx_EV_IRQHandler              I2C1_EV_IRQHandler
#define I2Cx_ER_IRQHandler              I2C1_ER_IRQHandler
#endif

/* I2C TIMING Register define when I2C clock source is APB1 (SYSCLK/4) */
/* I2C TIMING is calculated in case of the I2C Clock source is the APB1CLK = 50 MHz */
/* This example use TIMING to 0x40912732 to reach 100 kHz speed (Rise time = 700 ns, Fall time = 100 ns) */
#define I2C_TIMING                      0x40912732  //TODO - Check settings. SHould be 100KHz


/* ########################################################################## */
/* #    I N T E R R U P T  A S S I G N M E N T S                            # */
/* ########################################################################## */
/** Interrupt Channels, Group & Priority assignments:
 Note that lower number = higher priority (subpriority). All priorities are in relationship to
 each other & the priority group so changing one may affect others.
 */
#define DBG_UART_DMA_INT_PREEMPT_PRIORITY       9   // Uart DMA channel preemption prio. group
#define DBG_UART_TX_DMA_INT_SUB_PRIORITY        0   // Uart DMA channel subprio within group

#define DBG_UART_INT_PREEMPT_PRIORITY           10  // Lowest group Preemption priority
#define DBG_UART_INT_SUB_PRIORITY               0   // Lowest Priority within group

#define I2C_EVINT_PREEMPT_PRIORITY              3
#define I2C_EVINT_SUB_PRIORITY                  0
#define I2C_ERRINT_PREEMPT_PRIORITY             2
#define I2C_ERRINT_SUB_PRIORITY                 0

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
typedef DMA_HandleTypeDef   DMAhandle_t;
typedef UART_HandleTypeDef  UARThandle_t;

typedef enum MsgContextTag
{
    CTX_THREAD,     ///< Message sent from within a thread context
    CTX_ISR         ///< Message sent from ISR
} MsgContext;

typedef struct _Leds_info {
    uint32_t        rccPeriph;
    GPIO_TypeDef*   grp;
    uint16_t        pin;
} LedsInfo_t;

typedef struct _Interrupt_info {
    uint32_t        rccPeriph;
    GPIO_TypeDef*   grp;
    uint16_t        pin;
    uint8_t         extPortSource;
    uint8_t         extPinSource;
    uint32_t        extInterruptLine;
    IRQn_Type       irqChannel;
} InterruptInfo_t;

typedef struct _SPICSPin_info {
    uint32_t        rccPeriph;
    GPIO_TypeDef*   grp;
    uint16_t        chipSelect;
} SPICSPinInfo_t;


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern LedsInfo_t DiagLEDs[NUM_LEDS];

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
void MPU_Config( void );
void CPU_CACHE_Enable( void );
void LED_Init( void );


#endif /* HW_SETUP_NUCLEO_F746_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

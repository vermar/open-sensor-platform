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
#if !defined (HW_SETUP_DISCO_F4_H)
#define   HW_SETUP_DISCO_F4_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include <stdint.h>
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "osp-types.h"
#include <stddef.h>

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/* ########################################################################## */
/* #    T I M I N G S                                                       # */
/* ########################################################################## */
/** System clock & tick configuration */
#define SYSTEM_CLOCK_FREQ                       168000000  //Make sure this matches the settings in system_stm32f10x.c
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

#define ASF_Delay(mSec)                         os_dly_wait(MSEC_TO_TICS(mSec))

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
    LED_LD3,    //Orange
    LED_LD4,    //Green
    LED_LD5,    //Red
    LED_LD6,    //Blue
    NUM_LEDS
};


/* User Friendly LED designation - unused ones should be assigned 0xFF */
#define FRONT_LED                               LED_GREEN
#define LED_GREEN                               LED_LD4
#define LED_BLUE                                LED_LD6
#define LED_RED                                 LED_LD5
#define LED_ORANGE                              LED_LD3
#define HARD_FAULT_LED                          LED_BLUE

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
#define AssertIndication()                      LED_Off(FRONT_LED)

/* ########################################################################## */
/* #    A U D I O  I N T E R F A C E                                        # */
/* ########################################################################## */
/* Audio Reset Pin definition */
#define AUDIO_RESET_GPIO_CLK_ENABLE()           __GPIOD_CLK_ENABLE()
#define AUDIO_RESET_PIN                         GPIO_PIN_4
#define AUDIO_RESET_GPIO                        GPIOD

    /* The 7 bits Codec address (sent through I2C interface) */
#define CODEC_ADDRESS                           0x94  /* b00100111 */
#define CODEC_I2C_7BIT_ADDR                     (CODEC_ADDRESS >> 1)
#define AUDIO_I2C_ADDRESS                       CODEC_I2C_7BIT_ADDR

/* I2S peripheral configuration defines */
#define CODEC_I2S                               SPI3
#define CODEC_I2S_CLK                           RCC_APB1Periph_SPI3
#define CODEC_I2S_ADDRESS                       0x40003C0C
#define CODEC_I2S_GPIO_AF                       GPIO_AF_SPI3
#define CODEC_I2S_IRQ                           SPI3_IRQn
#define CODEC_I2S_GPIO_CLOCK                    (RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOA)
#define CODEC_I2S_WS_PIN                        GPIO_Pin_4
#define CODEC_I2S_SCK_PIN                       GPIO_Pin_10
#define CODEC_I2S_SD_PIN                        GPIO_Pin_12
#define CODEC_I2S_MCK_PIN                       GPIO_Pin_7
#define CODEC_I2S_WS_PINSRC                     GPIO_PinSource4
#define CODEC_I2S_SCK_PINSRC                    GPIO_PinSource10
#define CODEC_I2S_SD_PINSRC                     GPIO_PinSource12
#define CODEC_I2S_MCK_PINSRC                    GPIO_PinSource7
#define CODEC_I2S_GPIO                          GPIOC
#define CODEC_I2S_WS_GPIO                       GPIOA
#define CODEC_I2S_MCK_GPIO                      GPIOC
#define Audio_I2S_IRQHandler                    SPI3_IRQHandler

/* I2S DMA Stream definitions */
#define AUDIO_I2S_DMA_CLOCK                     RCC_AHB1Periph_DMA1
#define AUDIO_I2S_DMA_STREAM                    DMA1_Stream7
#define AUDIO_I2S_DMA_DREG                      CODEC_I2S_ADDRESS
#define AUDIO_I2S_DMA_CHANNEL                   DMA_Channel_0
#define AUDIO_I2S_DMA_IRQ                       DMA1_Stream7_IRQn
#define AUDIO_I2S_DMA_FLAG_TC                   DMA_FLAG_TCIF7
#define AUDIO_I2S_DMA_FLAG_HT                   DMA_FLAG_HTIF7
#define AUDIO_I2S_DMA_FLAG_FE                   DMA_FLAG_FEIF7
#define AUDIO_I2S_DMA_FLAG_TE                   DMA_FLAG_TEIF7
#define AUDIO_I2S_DMA_FLAG_DME                  DMA_FLAG_DMEIF7

/* I2C peripheral configuration defines (control interface of the audio codec) */
//#define CODEC_I2C                               I2C1
//#define CODEC_I2C_CLK                           RCC_APB1Periph_I2C1
//#define CODEC_I2C_GPIO_CLOCK                    RCC_AHB1Periph_GPIOB
//#define CODEC_I2C_GPIO_AF                       GPIO_AF_I2C1
//#define CODEC_I2C_GPIO                          GPIOB
//#define CODEC_I2C_SCL_PIN                       GPIO_Pin_6
//#define CODEC_I2C_SDA_PIN                       GPIO_Pin_9
//#define CODEC_I2S_SCL_PINSRC                    GPIO_PinSource6
//#define CODEC_I2S_SDA_PINSRC                    GPIO_PinSource9

/* Mask for the bit EN of the I2S CFGR register */
#define I2S_ENABLE_MASK                         0x0400


#define Audio_MAL_I2S_IRQHandler                DMA1_Stream7_IRQHandler

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
/* #    S P I   I N T E R F A C E                                           # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    I 2 C   I N T E R F A C E                                           # */
/* ########################################################################## */
/* ########################################################################## */
/* #    I 2 C   I N T E R F A C E                                           # */
/* ########################################################################## */
/* I2C Master interface defines for I2C Driver */
#define I2C_IF_BUS                              (I2C1)
#define I2C_IF_BUS_BASE                         ((U32)I2C1_BASE)
#define I2C_IF_BUS_CLOCK                        400000  //Fast mode
#define RCC_Periph_I2C_IF_BUS                   RCC_APB1Periph_I2C1
#define I2C_IF_CLK_ENABLE()                     __I2C1_CLK_ENABLE()
#define I2C_IF_FORCE_RESET()                    __I2C1_FORCE_RESET()
#define I2C_IF_RELEASE_RESET()                  __I2C1_RELEASE_RESET()

/* BUS-IO Pins */
#define I2C_IF_BUS_GPIO_GRP                     GPIOB
#define RCC_Periph_I2C_IF_BUS_GPIO              RCC_AHB1ENR_GPIOBEN
/* SCL */
#define I2C_IF_BUS_CLK_PIN                      GPIO_PIN_6
/* SDA */
#define I2C_IF_BUS_SDA_PIN                      GPIO_PIN_9
#define I2C_IF_SCL_SDA_AF                       GPIO_AF4_I2C1

/* DMA & interrupt Channel assignments */
#define I2C_IF_BUS_EVENT_IRQ_CH                 I2C1_EV_IRQn
#define I2C_IF_BUS_ERROR_IRQ_CH                 I2C1_ER_IRQn

#define I2C_IF_BUS_EVENT_IRQHandler             I2C1_EV_IRQHandler
#define I2C_IF_BUS_ERROR_IRQHandler             I2C1_ER_IRQHandler

/* Priorities for I2C Bus interrupts */
#define I2C_IF_BUS_INT_PREEMPT_PRIORITY         0   ///< I2C IRQ, I2C TX DMA and I2C RX DMA priority
#define I2C_IF_BUS_EVENT_INT_SUB_PRIORITY       0   ///< I2C EV IRQ subpriority !!!! IMPORTANT - THIS NEEDS TO BE HIGHEST PRIORITY !!!!
#define I2C_IF_BUS_ERROR_INT_SUB_PRIORITY       1   ///< I2C ER IRQ subpriority

/* I2C Master interface for Audio Codec */
#define I2C_AUDIO_BUS                           (I2C1)


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

/* Priorities for I2C Bus interrupts */
//#define I2C_AUDIO_BUS_INT_PREEMPT_PRIORITY     0   ///< I2C IRQ, I2C TX DMA and I2C RX DMA priority
//#define I2C_AUDIO_BUS_EVENT_INT_SUB_PRIORITY   0   ///< I2C EV IRQ subpriority !!!! IMPORTANT - THIS NEEDS TO BE HIGHEST PRIORITY !!!!
//#define I2C_AUDIO_BUS_ERROR_INT_SUB_PRIORITY   1   ///< I2C ER IRQ subpriority

#define EVAL_AUDIO_IRQ_PREPRIO                 1   /* Select the preemption priority level(0 is the highest) */
#define EVAL_AUDIO_IRQ_SUBRIO                  0   /* Select the sub-priority level (0 is the highest) */

#define NVIC_CH_ENABLE(IRQCh)                   NVIC_EnableIRQ(IRQCh)
#define NVIC_CH_DISABLE(IRQCh)                  NVIC_DisableIRQ(IRQCh)

/* ########################################################################## */
/* #    M I S C E L L A N E O U S                                           # */
/* ########################################################################## */
/* Device Unique ID register for STM32F4 series devices */
#define DEV_UID_OFFSET                          0x1FFF7A10
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

typedef enum MsgContextTag
{
    CTX_THREAD,     ///< Message sent from within a thread context
    CTX_ISR         ///< Message sent from ISR
} MsgContext;

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

/****************************************************************************************************
 * @fn      GetContext
 *          Identifies if we are currently in ISR or Thread context and returns the corresponding
 *          enum value for it. The logic is based on the fact that ISR uses system stack and Thread
 *          use their allocated stack. We only need to read the current value of SP to figure out
 *          whether we are in a Thread or ISR. (Maybe there is a better way... but this is good
 *          enough for now)
 *
 * @param   none
 *
 * @return  CTX_THREAD or CTX_ISR
 *
 ***************************************************************************************************/
static __inline MsgContext GetContext( void )
{
    extern uint32_t gStackMem;

    return (__current_sp() < (uint32_t)&gStackMem)? CTX_THREAD : CTX_ISR;
}



#endif /* HW_SETUP_DISCO_F4_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

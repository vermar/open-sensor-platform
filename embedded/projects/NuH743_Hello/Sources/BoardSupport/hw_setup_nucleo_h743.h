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
#if !defined (HW_SETUP_NUCLEO_H743_H)
#define   HW_SETUP_NUCLEO_H743_H

//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include <stdint.h>
#include <stddef.h>
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_exti.h"
#include "stm32h7xx_ll_system.h"
#include "stm32h7xx_ll_bus.h"
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
#define SYSTEM_CLOCK_FREQ                       480000000
#define USEC_PER_TICK                           1000       ///< (in uS) = 1KHz
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

/* PLL Parameters */
/* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M                                   1
#define PLL_N                                   120

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P                                   2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q                                   15
#define PLL_R                                   2

/* ########################################################################## */
/* #    F L A S H  S T O R A G E                                            # */
/* ########################################################################## */

/* **** NOT PRESENT **** */

/* ########################################################################## */
/* #    G P I O  I N T E R F A C E S / A S S I G N M E N T S                # */
/* ########################################################################## */
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
    GPIO_IN_CN9_IO1,         //Left IO marked signal on CN9
    NUM_GPIO_INPUTS
};

/* Diagnostic LEDs on this board */
enum _Leds {
    LED_GREEN,
    LED_RED,
    LED_YELLOW,
    NUM_LEDS
};

/* Input mode muxing to support analog input option with "pull-mode" */
#define GPIO_IN_MODE_ANALOG                     ((GPIO_MODE_ANALOG << 8) | GPIO_NOPULL)
#define GPIO_IN_PULLMODE_MASK                   0xFF
#define M_CheckAnalogMode(pm)                   ((((pm) >> 8) & 0xFF) == GPIO_MODE_ANALOG)

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

#define LED_Toggle(led)                              \
    if (led < NUM_LEDS)                              \
    {                                                \
        DiagLEDs[led].grp->ODR ^= DiagLEDs[led].pin; \
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
#define GPIO_GetInputState(gpioIn)                          \
    HAL_GPIO_ReadPin(InputGPIOs[gpioIn].grp, InputGPIOs[gpioIn].pin)

#define GPIO_GetOutputState(gpioIn)                          \
    HAL_GPIO_ReadPin(OutputGPIOs[gpioIn].grp, OutputGPIOs[gpioIn].pin)

/* Assert LED assignment */
#define AssertIndication()                      LED_On(LED_RED)

/* ########################################################################## */
/* #    U A R T  I N T E R F A C E S                                        # */
/* ########################################################################## */
/** Debug Console UART configuration */
#define DBG_UART_BAUD                           115200
#define DBG_IF_UART                             USART3      //Connected to STLink's VCP
#define DBG_UART_CLK_ENABLE()                   __HAL_RCC_USART3_CLK_ENABLE();
#define DBG_UART_DMAx_CLK_ENABLE()              __HAL_RCC_DMA2_CLK_ENABLE()
#define DBG_UART_CLK_SELECTION                  RCC_PERIPHCLK_USART3
#define DBG_UART_CLK_SOURCE                     RCC_USART234578CLKSOURCE_PCLK1
#define DBG_UART_RCC_SELECTION                  rccInit.Usart234578ClockSelection

#define DBG_UART_FORCE_RESET()                  __HAL_RCC_USART3_FORCE_RESET()
#define DBG_UART_RELEASE_RESET()                __HAL_RCC_USART3_RELEASE_RESET()

/* Definition for USARTx Pins */
#define DBG_UART_TX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DBG_UART_TX_PIN                         GPIO_PIN_8
#define DBG_UART_TX_GPIO_PORT                   GPIOD
#define DBG_UART_TX_AF                          GPIO_AF7_USART3
#define DBG_UART_RX_GPIO_CLK_ENABLE()           __HAL_RCC_GPIOD_CLK_ENABLE()
#define DBG_UART_RX_PIN                         GPIO_PIN_9
#define DBG_UART_RX_GPIO_PORT                   GPIOD
#define DBG_UART_RX_AF                          GPIO_AF7_USART3

/* Definition for USARTx's DMA (see RM0410 Rev 4) */
#define DBG_UART_TX_DMA_STREAM                  DMA2_Stream7
#define DBG_UART_RX_DMA_STREAM                  DMA2_Stream5

#define DBG_UART_TX_DMA_REQUEST                 DMA_REQUEST_USART3_TX
#define DBG_UART_RX_DMA_REQUEST                 DMA_REQUEST_USART3_RX

/* Definition for USARTx's NVIC */
#define DBG_UART_DMA_TX_IRQn                    DMA2_Stream7_IRQn
#define DBG_UART_DMA_RX_IRQn                    DMA2_Stream5_IRQn
#define DBG_UART_DMA_TX_IRQHandler              DMA2_Stream7_IRQHandler
#define DBG_UART_DMA_RX_IRQHandler              DMA2_Stream5_IRQHandler

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
#define NUM_I2C_INTERFACES                      2   //Number of I2C interfaces used on the board
#define NUM_I2C_CLIENTS                         2   //Total clients for all I2C interfaces
#define I2C_BUS_CLOCK_400K                      400000  //Fast mode
#define I2C_BUS_CLOCK_100K                      100000  //Standard mode

/* I2C Master interface defines for I2C Driver */
/* Channel X interface */
#define I2C_IF1_BUS                             I2C1
#define RCC_Periph_I2C_IF1_BUS                  RCC_PERIPHCLK_I2C1
#define RCC_I2C_IF1_CLKSOURCE                   RCC_I2C123CLKSOURCE_D2PCLK1
#define I2C_IF1_RCC_SELECTION                   rccInit.I2c123ClockSelection
#define I2C_IF1_CLK_ENABLE()                    __HAL_RCC_I2C1_CLK_ENABLE()
#define I2C_IF1_FORCE_RESET()                   __HAL_RCC_I2C1_FORCE_RESET()
#define I2C_IF1_RELEASE_RESET()                 __HAL_RCC_I2C1_RELEASE_RESET()

/* I2C TIMING Register define when I2C clock source is PCLK1 */
/* I2C TIMING is calculated in case of the I2C Clock source is the PCLK1 = 120 MHz, I2C baud 400K */
/* 400K, 100ns Rise, 100ns Fall */
#define I2C_IF1_TIMING_400K                     0x10B21F61  //Note: As provided by CubeMX/CubeIDE tool
/* 100K, 100ns Rise, 100ns Fall */
#define I2C_IF1_TIMING_100K                     0x30A175AB  //Note: As provided by CubeMX/CubeIDE tool

/* BUS-IO Pins */
#define I2C_IF1_BUS_GPIO_GRP                    GPIOB
#define I2C_IF1_BUS_CLK_PIN                     GPIO_PIN_8
#define I2C_IF1_BUS_SDA_PIN                     GPIO_PIN_9
#define I2C_IF1_SCL_SDA_AF                      GPIO_AF4_I2C1
#define I2C_IF1_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOB_CLK_ENABLE()

/* Interrupt Channel assignments */
#define I2C_IF1_BUS_EVENT_IRQ_CH                I2C1_EV_IRQn
#define I2C_IF1_BUS_ERROR_IRQ_CH                I2C1_ER_IRQn

#define I2C_IF1_ISR_Handler                     I2C1_EV_IRQHandler
#define I2C_IF1_ERR_ISR_Handler                 I2C1_ER_IRQHandler

/* Second I2C interface */
/* Channel Y interface */
#define I2C_IF2_BUS                             I2C2
#define RCC_Periph_I2C_IF2_BUS                  RCC_PERIPHCLK_I2C2
#define RCC_I2C_IF2_CLKSOURCE                   RCC_I2C123CLKSOURCE_D2PCLK1
#define I2C_IF2_RCC_SELECTION                   rccInit.I2c123ClockSelection
#define I2C_IF2_CLK_ENABLE()                    __HAL_RCC_I2C2_CLK_ENABLE()
#define I2C_IF2_FORCE_RESET()                   __HAL_RCC_I2C2_FORCE_RESET()
#define I2C_IF2_RELEASE_RESET()                 __HAL_RCC_I2C2_RELEASE_RESET()

/* BUS-IO Pins */
#define I2C_IF2_BUS_GPIO_GRP                    GPIOF
#define I2C_IF2_BUS_CLK_PIN                     GPIO_PIN_1
#define I2C_IF2_BUS_SDA_PIN                     GPIO_PIN_0
#define I2C_IF2_SCL_SDA_AF                      GPIO_AF4_I2C2
#define I2C_IF2_GPIO_CLK_ENABLE()               __HAL_RCC_GPIOF_CLK_ENABLE()

/* Interrupt Channel assignments */
#define I2C_IF2_BUS_EVENT_IRQ_CH                I2C2_EV_IRQn
#define I2C_IF2_BUS_ERROR_IRQ_CH                I2C2_ER_IRQn

#define I2C_IF2_ISR_Handler                     I2C2_EV_IRQHandler
#define I2C_IF2_ERR_ISR_Handler                 I2C2_ER_IRQHandler

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

 /* I2C Driver interrupt priorities (same for all I2C interfaces) */
 /* !!!! Note - In the past this needed to be the highest priority due to some "bug" in STM32 implementation
  * but hopefully that's been fixed since. If I2C issues are seen (hung bus) the try making these priorities highest (0)
  */
#define I2C_IF_BUS_INT_PREEMPT_PRIORITY         2   // I2C IRQ, I2C TX DMA and I2C RX DMA priority
#define I2C_IF_BUS_EVENT_INT_SUB_PRIORITY       0   // I2C EV IRQ subpriority
#define I2C_IF_BUS_ERROR_INT_SUB_PRIORITY       0   // I2C ER IRQ subpriority

/* ########################################################################## */
/* #    M I S C E L L A N E O U S                                           # */
/* ########################################################################## */
/* Device Unique ID register for STM32H7 series devices */
#define DEV_UID_OFFSET                          0x1FF0F420
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
    GPIO_STATE_HIGH,
    GPIO_INVALID
} GpioState_t;

typedef struct _GPIO_OutputInfo
{
    const char*     schRef;     //Schematic reference name
    GPIO_TypeDef*   grp;
    uint16_t        pin;
    GpioState_t     initState; //LOW/HIGH
    uint16_t        hwCompat;  //Compatibility mask
} GPioOutputInfo_t;

typedef struct _GPIO_InputInfo
{
    const char*     schRef;     //Schematic reference name
    GPIO_TypeDef*   grp;
    uint16_t        pin;
    uint32_t        pullMode;  //Pull-up/Pull-down/No-pull
    uint16_t        hwCompat;  //Compatibility mask
} GPioInputInfo_t;

/* Interrupt configuration */
typedef enum _ExtInterruptId
{
    USER_BUTTON_PRESS = GPIO_IN_USR_BTN,
    DBG_TEST_INPUT = GPIO_IN_CN9_IO1,   //Left edge IO signal on CN9 connector
} ExtIntId_t;

typedef enum _IntTriggerType
{
    INT_EDGE_NOT_CONFIGURABLE,
    INT_TRIGGER_FALLING_EDGE,
    INT_TRIGGER_RISING_EDGE
} IntTrigger_t;

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
extern DeviceUid_t *gDevUniqueId;
extern const GPioOutputInfo_t DiagLEDs[NUM_LEDS];
extern const GPioOutputInfo_t OutputGPIOs[NUM_GPIO_OUTPUTS];
extern const GPioInputInfo_t InputGPIOs[NUM_GPIO_INPUTS];

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
void MPU_Config( void );
void CPU_CACHE_Enable( void );
void DumpGpioInputStatusAll(void);
void DumpGpioOutputStatusAll(void);
GpioState_t GetGpioStateByName(const char* schRef);
void SetGpioStateByName(const char* schRef, GpioState_t state);
void I2C_IF1_HardwareSetup(void);
void I2C_IF2_HardwareSetup(void);

#endif /* HW_SETUP_NUCLEO_H743_H */
//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

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
#if !defined (MAIN_H)
#define   MAIN_H
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "stm32f4xx.h"              /* STM32F4xx Library Definitions     */

#ifdef __cplusplus
extern "C"
{
#endif

//==================================================================================================
//    C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define THIS_BOARD                      "Nucleo-F413??"

/* Compiler Version Macros */
#ifdef __ICCARM__
# define _1M_                           1000000
# define _ICC_MAJOR_                    (__VER__/_1M_)
# define _ICC_MINOR_                    ((__VER__%_1M_)/1000)
# define _ICC_PATCH_                    ((__VER__%_1M_)%1000)
#endif

#if defined (__CC_ARM) || defined (__ARMCC_VERSION)
# define _1M_                           1000000
# define _ARMCC_MAJOR_                  (__ARMCC_VERSION / _1M_)
# define _ARMCC_MINOR_                  ((__ARMCC_VERSION % _1M_) / 10000)
# define _ARMCC_BUILD_                  ((__ARMCC_VERSION % _1M_) % 10000)
#endif

#ifdef DEBUG_BUILD
# ifndef DEBUG_OUTPUT
# define DEBUG_OUTPUT
# endif
#endif

//#define RESET_ON_ASSERT

#ifdef RESET_ON_ASSERT
# define SysRESET()                     NVIC_SystemReset()
#else
# define SysRESET()                     while(1)
#endif


#define MAX_SYSTEM_MESSAGES             25     // Max number of queued messages in the system
#define MAX_OS_TIMERS                   20
/* NOTE: Timer references are specific to tasks that create the timer and can be defined in the same
 * module as the task unless (an unlikely scenario) a task creates a timer for another task, in which
 * case the timer reference must be known by the (expiry message) receiving task */

/* Printf and assert support for debugging */
#ifdef DEBUG_OUTPUT
# define DEBUG_LVL                      1
#else
# ifndef DEBUG_LVL
#  define DEBUG_LVL                     0
# endif
#endif

#if (DEBUG_LVL > 0)
# ifdef UART_DMA_ENABLE
#  define MAX_DPRINTF_MESSAGES                  60  ///< Max printf messages allowed at a given time
# else
#  error Unsupported UART debug mode
# endif

/* Receive buffer size. Note: Actual receive capacity is 1 less than the size specified here */
# define RX_BUFFER_SIZE                 200
# define DPRINTF_BUFF_SIZE              120   // Print messages above this size will be truncated

#else //DEBUG_LVL = 0

# ifndef UART_DMA_ENABLE
#  error Unsupported UART debug mode
# endif
# define RX_BUFFER_SIZE                 200
# define MAX_DPRINTF_MESSAGES           10   // Max printf messages buffered at a given time
# define DPRINTF_BUFF_SIZE              90

#endif

/* ANSI escape sequences for debug messages */
#define G_NORM                          "\x1B[0m"       //Reset Color
/* 16 Colors (Foreground) */
#define G_BLACK                         "\x1B[30m"
#define G_BLACK_BOLD                    "\x1B[1;30m"
#define G_RED                           "\x1B[31m"
#define G_RED_BOLD                      "\x1B[1;31m"
#define G_GREEN                         "\x1B[32m"
#define G_GREEN_BOLD                    "\x1B[1;32m"
#define G_YELLOW                        "\x1B[33m"
#define G_YELLOW_BOLD                   "\x1B[1;33m"
#define G_BLUE                          "\x1B[34m"
#define G_BLUE_BOLD                     "\x1B[1;34m"
#define G_MAGENTA                       "\x1B[35m"
#define G_MAGENTA_BOLD                  "\x1B[1;35m"
#define G_CYAN                          "\x1B[36m"
#define G_CYAN_BOLD                     "\x1B[1;36m"
#define G_WHITE                         "\x1B[37m"
#define G_WHITE_BOLD                    "\x1B[1;37m"
#define G_BOLD                          "\x1B[1m"

/* Background Colors */
#define BG_BLACK                        "\x1B[40m"
#define BG_RED                          "\x1B[41m"
#define BG_GREEN                        "\x1B[42m"
#define BG_YELLOW                       "\x1B[43m"
#define BG_BLUE                         "\x1B[44m"
#define BG_MAGENTA                      "\x1B[45m"
#define BG_CYAN                         "\x1B[46m"
#define BG_WHITE                        "\x1B[47m"

/* Cursor Controls */
#define CURSOR_UP1                      "\x1B[1A"
#define CURSOR_UP                       "\x1B[A"
#define CURSOR_DN                       "\x1B[B"
#define CURSOR_UP2                      "\x1B[2A"
#define CURSOR_SAVE_POS                 "\x1B[s"
#define CURSOR_RESTORE_POS              "\x1B[u"
#define ERASE_LINE                      "\x1B[K"

/* Defines for command handler task only. Event flag scope is limited to the task and each task
 * can have its own set of 16 flag-bits */
#define COMMAND_LINE_SIZE               32
#define EVT_FLAG_CURSOR_UP              0x0001
#define EVT_FLAG_CURSOR_DN              0x0002

/* Application Event flags */
#define EVT_FLAG_ANY_EVENT                      0

/* Common application macros */
#define M_KillTimerIfStarted(pTimer)    \
    if (ASFTimerStarted(pTimer))        \
    {                                   \
        ASFKillTimer(pTimer);           \
    }

//==================================================================================================
//    T Y P E   D E F I N I T I O N S
//==================================================================================================
/* Serial command parser tokens */
enum ParserTokensTag
{
    TOKEN_NULL = 0,
    TOKEN_STATS = '\r',
    TOKEN_BS = 0x08,    //Backspace
};

/* RTC clock */
typedef struct RtcClockTag
{
    uint16_t hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t msec;
} RtcClock_t;

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
int _dprintf(uint8_t dbgLvl, const char* fmt, ...);

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
extern uint8_t gHardwareRev;

//==================================================================================================
//    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================
uint32_t RTC_GetCounter( void );
void HexDump(const void* memory, uint32_t bytes);

#ifdef __cplusplus
} /*extern "C" */
#endif
#endif /* MAIN_H */
//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

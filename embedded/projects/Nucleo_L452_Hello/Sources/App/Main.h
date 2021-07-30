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
#if !defined (MAIN_H)
#define   MAIN_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include "stm32l4xx.h"              /* STM32L4xx Library Definitions     */

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#ifdef DEBUG_BUILD
# ifndef DEBUG_OUTPUT
# define DEBUG_OUTPUT
# endif
#endif

//#define RESET_ON_ASSERT

#ifdef RESET_ON_ASSERT
# define SysRESET()                             NVIC_SystemReset()
#else
# define SysRESET()                             while(1)
#endif


#define MAX_SYSTEM_MESSAGES                     15     ///< Max number of queued messages in the system

/* All timer references (arbitrary unique identifiers for each timer)*/
#define TIMER_REF_LEDB_ON                       0x55A0
#define TIMER_REF_LEDB_OFF                      0x55A1
#define TIMER_REF_RTC_UPDATE                    0x55A5
#define TIMER_REF_SENSOR_READ                   0x55B0

/* Printf and assert support for debugging */
#ifdef DEBUG_OUTPUT
# define DEBUG_LVL      1
#else
# ifndef DEBUG_LVL
#  define DEBUG_LVL     0
# endif
#endif

#if (DEBUG_LVL > 0)
# ifdef UART_DMA_ENABLE
#  define MAX_DPRINTF_MESSAGES                  30  ///< Max printf messages allowed at a given time
# else
#  define TX_BUFFER_SIZE                        512
#  define MAX_DPRINTF_MESSAGES                  30   ///< Max printf messages allowed at a given time
# endif

# define RX_BUFFER_SIZE                         32
# define DPRINTF_BUFF_SIZE                      200

#else //DEBUG_LVL = 0

# ifndef UART_DMA_ENABLE
#  define TX_BUFFER_SIZE                        512
# endif
# define RX_BUFFER_SIZE                         200
# define MAX_DPRINTF_MESSAGES                   10   ///< Max printf messages allowed at a given time
# define DPRINTF_BUFF_SIZE                      100

#endif

/* ANSI escape sequences for debug messages */
#define G_NORM                                  "\x1B[0m"       //Reset Color
/* 16 Colors (Foreground) */
#define G_BLACK                                 "\x1B[30m"
#define G_BLACK_BOLD                            "\x1B[1;30m"
#define G_RED                                   "\x1B[31m"
#define G_RED_BOLD                              "\x1B[1;31m"
#define G_GREEN                                 "\x1B[32m"
#define G_GREEN_BOLD                            "\x1B[1;32m"
#define G_YELLOW                                "\x1B[33m"
#define G_YELLOW_BOLD                           "\x1B[1;33m"
#define G_BLUE                                  "\x1B[34m"
#define G_BLUE_BOLD                             "\x1B[1;34m"
#define G_MAGENTA                               "\x1B[35m"
#define G_MAGENTA_BOLD                          "\x1B[1;35m"
#define G_CYAN                                  "\x1B[36m"
#define G_CYAN_BOLD                             "\x1B[1;36m"
#define G_WHITE                                 "\x1B[37m"
#define G_WHITE_BOLD                            "\x1B[1;37m"
#define G_BOLD                                  "\x1B[1m"

/* Background Colors */
#define BG_BLACK                                "\x1B[40m"
#define BG_RED                                  "\x1B[41m"
#define BG_GREEN                                "\x1B[42m"
#define BG_YELLOW                               "\x1B[43m"
#define BG_BLUE                                 "\x1B[44m"
#define BG_MAGENTA                              "\x1B[45m"
#define BG_CYAN                                 "\x1B[46m"
#define BG_WHITE                                "\x1B[47m"

/* Cursor Controls */
#define CURSOR_UP1                              "\x1B[1A"
#define CURSOR_UP                               "\x1B[A"
#define CURSOR_DN                               "\x1B[B"
#define CURSOR_UP2                              "\x1B[2A"
#define CURSOR_SAVE_POS                         "\x1B[s"
#define CURSOR_RESTORE_POS                      "\x1B[u"
#define ERASE_LINE                              "\x1B[K"

/* Defines for command handler */
#define COMMAND_LINE_SIZE                       32
//#define CONSOLE_PROMPT                          "\r\n> "
#define EVT_FLAG_CURSOR_UP                      0x0001
#define EVT_FLAG_CURSOR_DN                      0x0002

/* Application Event flags */
#define EVT_FLAG_ANY_EVENT                      0

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Serial command parser tokens */
enum ParserTokensTag {
    TOKEN_NULL = 0,
    TOKEN_1 = 'c',
    TOKEN_2 = 'm',
    TOKEN_3 = 'd',
    TOKEN_4 = '=',
    TOKEN_STATS = '\r',
    TOKEN_BS = 0x08,    //Backspace
    TOKEN_PARAM = 0xAA
};

/* RTC clock */
typedef struct RtcClockTag
{
    uint16_t hours;
    uint8_t  minutes;
    uint8_t  seconds;
    uint16_t msec;
} RtcClock_t;


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* RTC Counter (using TIM2 instead of RTC) */
uint32_t RTC_GetCounter( void );

#endif /* MAIN_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

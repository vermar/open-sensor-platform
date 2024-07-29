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
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "common.h"
#include "hw_setup.h"
#include <ctype.h>

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
ASF_TASK void InstrManagerTask( ASF_TASK_ARG );
extern uint32_t gStackMemTop;
extern uint32_t gHeapStart;
extern uint32_t gHeapSize;
extern AsfTaskHandle asfTaskHandleTable[];

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================

//==================================================================================================
//    P R I V A T E   T Y P E   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
#ifdef DEBUG_BUILD
  char _errBuff[ERR_LOG_MSG_SZ];
#endif
uint8_t gHardwareRev = 0;

//==================================================================================================
//    P R I V A T E     F U N C T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief In CMSIS-RTOS framework, main() is the first application thread that is created by the
**        kernel's internal initialization along side Timer thread (if enabled). This thread has
**        the responsibility of spawning other system threads and system initialization. In ASF,
**        this was being done in Instrumentation Manager Task so we just call the entry function
**        for Instrumentation Manager here.
**
** @param None
**
** @return 0 always
*/
int main( void )
{
    InstrManagerTask( NULL );

    /* we don't expect to return but just to shut up the compiler... */
    return 0;
}

/***************************************************************************************************
** @brief Platform initialization was done originally in main() (non-CMSIS scheme). Now its the
**        first function called by the Instrumentation Manager task to initialize platform specific
**        hardware and debug interfaces. Note that some clock setup is already done at this point
**        via the SystemInit() call made from the startup file.
**
** @param None
**
** @return None
*/
void PlatformInitialize( void )
{
    /* Configure the MPU attributes as Write Through */
    MPU_Config();

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32H7xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Global MSP (MCU Support Package) initialization
       - Set NVIC Priority Grouping (default NVIC_PRIORITYGROUP_4 - 4-bits for pre-emption priority,
         0 for sub-priority)
    */
    HAL_Init();

    /* NVIC configuration */
    SystemInterruptConfig();

    // Configure the System clock to have a frequency of 216 MHz
    SystemClock_Config();
    SystemCoreClockUpdate();

    /* Configure RTC */
    RTC_Configuration(); //Configuring RTC after SystemCoreClockUpdate to support HAL_GetTick & HAL_Delay calls

    /* Configure the GPIO ports (non module specific) */
    SystemGPIOConfig();

    /* Set startup state of LEDs */
    LED_Init(HW_REV_COMMON);    /* Initialize Debug LEDs */
    LED_On(FRONT_LED);          //Visual indication that we powered up

    /* Configure debug UART port - we do it here to enable assert messages early in the system */
    DebugPortInit();
    DebugUARTConfig( DBG_UART_BAUD, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE );

    /* Print version number */
    printf("\r\nCMSIS-RTX Hello World ASF Example for %s, %s - %s ###\r\n",
        THIS_BOARD, __DATE__, __TIME__);

    /* Print compiler identification & version */
#if defined (__ICCARM__)
    D0_printf(G_BLUE"\tCompiler: IAR C/C++ Compiler for Arm\r\n");
    D0_printf("\tVersion: %u.%u.%u\r\n"G_NORM, _ICC_MAJOR_, _ICC_MINOR_, _ICC_PATCH_);
    D0_printf("\nStack Top: %lX\r\n", gStackMemTop);
    D0_printf("Heap Start: %lX, Size: %ld\r\n", gHeapStart, gHeapSize);
#else /* Keil or GCC compilers */
# if defined (__GNUC__) && !defined (__clang__)
    D0_printf(G_MAGENTA"\tCompiler: GNU Embedded C/C++ Compiler for Arm\r\n");
    D0_printf("\tVersion: %u.%u.%u\r\n"G_NORM, __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
# endif
# if defined (__CC_ARM) || defined (__ARMCC_VERSION)
#  if (_ARMCC_MAJOR_ >= 6)
    D0_printf(G_CYAN"\tCompiler: Arm Clang for Keil MDK\r\n");
#  else
    D0_printf(G_CYAN"\tCompiler: Keil C Compiler for Arm\r\n");
#  endif
    D0_printf("\tVersion: %u.%02u Build: %u\r\n"G_NORM, _ARMCC_MAJOR_, _ARMCC_MINOR_, _ARMCC_BUILD_);
# endif
    /* Heap/Stack display for GCC or Keil */
    D0_printf("\nStack Top: 0x%lX\r\n", (uint32_t)&gStackMemTop);
    D0_printf("Heap Start: 0x%lX, Size: %ld\r\n", (uint32_t)&gHeapStart, (uint32_t)&gHeapSize);
#endif

    /* Display System memory & clock information */
    D0_printf("MSP: 0x%lX\r\n", __get_MSP());
    D0_printf("PSP: 0x%lX\r\n\n", __get_PSP());

    D0_printf("System Clocks:\r\n");
    D0_printf("\tCore Clk - %ld Hz\r\n", SystemCoreClock);
    D0_printf("\tSYSCLK   - %ld Hz\r\n", HAL_RCC_GetSysClockFreq());
    D0_printf("\tHCLK     - %ld Hz\r\n", HAL_RCC_GetHCLKFreq());
    D0_printf("\tPCLK1    - %ld Hz\r\n", HAL_RCC_GetPCLK1Freq());
    D0_printf("\tPCLK2    - %ld Hz\r\n", HAL_RCC_GetPCLK2Freq());

    D0_printf("STM32 MCU 96-bits UID: ");
    D0_printf("%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\r\n",
        gDevUniqueId->uidBytes[11], gDevUniqueId->uidBytes[10], gDevUniqueId->uidBytes[9],
        gDevUniqueId->uidBytes[8], gDevUniqueId->uidBytes[7], gDevUniqueId->uidBytes[6],
        gDevUniqueId->uidBytes[5], gDevUniqueId->uidBytes[4], gDevUniqueId->uidBytes[3],
        gDevUniqueId->uidBytes[2], gDevUniqueId->uidBytes[1], gDevUniqueId->uidBytes[0]);
}

/***************************************************************************************************
** @brief Helper function for HexDump (adapted from: http://www.iso-9899.info/wiki/Code_snippets)
**
** @note: Choose Windows-Arabic character set (or ISO8859-11) in console program
**
** @param memory: Pointer to the memory buffer to dump
** @param numBytes: Number of bytes to dump
**
** @return None
*/
void HexDump(const void* memory, uint32_t numBytes)
{
#define LINE_SZ_MAX     80
#define CHARS_PER_LINE  16
    const uint8_t* p, * q;
    uint32_t i, n;
    char buffer[LINE_SZ_MAX];

    p = memory;
    while (numBytes)
    {
        q = p;
        n = 0;
        for (i = 0; i < CHARS_PER_LINE && numBytes; ++i)
        {
            n += snprintf(&buffer[n], (LINE_SZ_MAX - n), "%02X ", *p);
            ++p;
            --numBytes;
        }
        numBytes += i;
        while (i < CHARS_PER_LINE)
        {
            n += snprintf(&buffer[n], (LINE_SZ_MAX - n), "XX ");
            ++i;
        }
        n += snprintf(&buffer[n], (LINE_SZ_MAX - n), "| ");
        p = q;
        for (i = 0; i < CHARS_PER_LINE && numBytes; ++i)
        {
            /* Note: Choose Windows-Arabic character set in console program */
            if (isprint(*p))
            {
                n += snprintf(&buffer[n], (LINE_SZ_MAX - n), "%c", *p);
            }
            else
            {
                n += snprintf(&buffer[n], (LINE_SZ_MAX - n), "\x95"); //Non-printable characters are shown as large
                                                                      //dot in the center
            }
            ++p;
            --numBytes;
        }
        while (i < CHARS_PER_LINE)
        {
            n += snprintf(&buffer[n], (LINE_SZ_MAX - n), " ");
            ++i;
        }
        D0_printf(G_BLACK_BOLD"\t%s |\r\n"G_NORM, buffer);
    }
}

/***************************************************************************************************
** @brief This example task turns On the LEDs on the board in sequential order & signals the LED Off
**        task
**
**  Input:
** (see definition for ASF_TASK_ARG)
**
** @return (see definition for ASF_TASK - typically void)
*/
ASF_TASK void LED_On_Task( void )
{
    uint8_t ledID = 0;
    for (;;)
    {
        LED_On(ledID);                      /* Turn LED On                   */
        osSignalSet( asfTaskHandleTable[LED_OFF_TASK_ID].handle, 0x0001 );
        ASFTaskSleep(200);
        //D0_printf("Timer Count: %lu\r\n", RTC_GetCounter());
        ledID = (ledID + 1) % NUM_LEDS;
    }
}

/***************************************************************************************************
** @brief This example task waits for signal from LED On task and turns Off the LEDs on the board in
**        sequential order
**
**  Input:
** (see definition for ASF_TASK_ARG)
**
** @return (see definition for ASF_TASK - typically void)
*/
ASF_TASK void LED_Off_Task( void )
{
    uint8_t ledID = 0;
    osEvent  ret;
    uint16_t evtFlags = 0;

    for (;;)
    {
        evtFlags = 0;
        ret = osSignalWait( 0, osWaitForever ); //0 => Any signal will resume thread
        if (ret.status == osEventSignal)
        {
            evtFlags = ret.value.signals; //provided for ref. not used here!
            (void)evtFlags;               //avoid compiler warning
        }
        ASFTaskSleep(15);                   /* delay 20ms                        */
        LED_Off(ledID);                     /* Turn LED Off                      */
        ledID = (ledID + 1) % NUM_LEDS;
    }
}

#ifdef INCLUDE_TEST_TASK
/***************************************************************************************************
** @brief This example task is used for testing the system tick timing & RTC timing
**
**  Input:
** (see definition for ASF_TASK_ARG)
**
** @return (see definition for ASF_TASK - typically void)
*/
ASF_TASK void LED_Test_Task( void )
{
    uint32_t counter = 0;
    uint32_t timeElap = 0;

    while(1)
    {
        D0_printf("This is test task - %u\r\n", counter++);
        timeElap = RTC_GetCounter();
        ASFTaskSleep(1000);
        timeElap = ((RTC_GetCounter() - timeElap) * US_PER_RTC_TICK )/1000;
        D0_printf(G_BLUE "\tRTC Delta = %ld ms\r\n" G_NORM, timeElap);
    }
}
#endif

//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

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
/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include "common.h"
#include "hw_setup.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
#ifdef ASF_PROFILING
  extern uint32_t gStackMem;
  extern uint32_t gStackSize;
  extern const char C_gStackPattern[8];
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
#ifdef DEBUG_BUILD
  char _errBuff[ERR_LOG_MSG_SZ];
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      main
 *          Main entry point to the application firmware
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
int main( void )
{
#ifdef ASF_PROFILING
    register uint32_t *pStack = (uint32_t *)&gStackMem;
    register uint32_t stkSize = (uint32_t)&gStackSize;
    register uint32_t idx;

    /* main() is using the same stack that we are trying to initialize so we leave the last 32 bytes */
    for ( idx = 0; idx < ((stkSize-32)/sizeof(C_gStackPattern)); idx++)
    {
        *pStack++ = *((uint32_t *)C_gStackPattern);
        *pStack++ = *((uint32_t *)(C_gStackPattern+4));
    }
#endif
    /* STM32F4xx HAL library initialization:
       - Configure the Flash prefetch, instruction and Data caches
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Global MSP (MCU Support Package) initialization
    */
    HAL_Init();

    // Configure the System clock to have a frequency of 168 MHz
    SystemClock_Config();
    SystemCoreClockUpdate();

    /* Configure RTC */
    RTC_Configuration();

    /* Configure the GPIO ports (non module specific) */
    SystemGPIOConfig();

    /* Set startup state of LEDs */
    LED_Init();                   /* Initialize Debug LEDs */
    LED_On(FRONT_LED); //Visual indication that we powered up

    /* Configure debug UART port - we do it here to enable assert messages early in the system */
    DebugPortInit();
    DebugUARTConfig( DBG_UART_BAUD, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE );

    /* Print version number */
    printf("\r\n### RTX Hello World Application Example for Discovery-F4 board: Date: %s - %s ###\r\n",
        __DATE__, __TIME__);

    /* Display System clock information */
    D0_printf("System Clocks:\r\n");
    D0_printf("\tCore Clk - %ld\r\n", SystemCoreClock);
    D0_printf("\tSYSCLK   - %ld\r\n", HAL_RCC_GetSysClockFreq());
    D0_printf("\tHCLK     - %ld\r\n", HAL_RCC_GetHCLKFreq());
    D0_printf("\tPCLK1    - %ld\r\n", HAL_RCC_GetPCLK1Freq());
    D0_printf("\tPCLK2    - %ld\r\n", HAL_RCC_GetPCLK2Freq());
    //D0_printf("RTC Prescalar: %ld\r\n", PrescalerValue);

    D0_printf("Device SNo.: %08X-%08X-%08X\r\n", gDevUniqueId->uidWords[2],
        gDevUniqueId->uidWords[1], gDevUniqueId->uidWords[0]);
    D0_printf("\t%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\r\n",
        gDevUniqueId->uidBytes[11], gDevUniqueId->uidBytes[10], gDevUniqueId->uidBytes[9],
        gDevUniqueId->uidBytes[8], gDevUniqueId->uidBytes[7], gDevUniqueId->uidBytes[6],
        gDevUniqueId->uidBytes[5], gDevUniqueId->uidBytes[4], gDevUniqueId->uidBytes[3],
        gDevUniqueId->uidBytes[2], gDevUniqueId->uidBytes[1], gDevUniqueId->uidBytes[0]);

    /* Get the OS going - This must be the last call */
    AsfInitialiseTasks();

    /* If it got here something bad happened */
    ASF_assert_fatal(false);
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

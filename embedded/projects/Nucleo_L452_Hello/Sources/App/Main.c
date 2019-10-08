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
ASF_TASK void InstrManagerTask( ASF_TASK_ARG );

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
 *          In CMSIS-RTOS framework, main() is the first application thread that is created by the
 *          kernel's internal initialization along side Timer thread (if enabled). This thread has
 *          the responsibility of spawning other system threads and system initialization. In ASF,
 *          this was being done in Instrumentation Manager Task so we just call the entry function
 *          for Instrumentation Manager here.
 *
 * @param   none
 *
 * @return  0 always.
 *
 ***************************************************************************************************/
int main( void )
{
    InstrManagerTask( NULL );

    /* we don't expect to return but just to shut up the compiler... */
    return 0;
}



/****************************************************************************************************
 * @fn      PlatformInitialize
 *          This was done originally in main() (non-CMSIS scheme). Now its the first function called
 *          by the Instrumentation Manager task to initialize platform specific hardware and debug
 *          interfaces. Note that some clock setup is already done at this point via the SystemInit()
 *          call made from the startup file.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void PlatformInitialize( void )

{

    /* NVIC configuration */
    SystemInterruptConfig();


    /* STM32L4xx HAL library initialization:
       - Configure the Flash prefetch and Buffer caches
       - Systick timer is configured by default as source of time base, but user
             can eventually implement his proper time base source (a general purpose
             timer for example or other time source), keeping in mind that Time base
             duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and
             handled in milliseconds basis.
       - Low Level Initialization
    */
    HAL_Init();

    /* Configure the System clock to have a frequency of 80MHz */
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
    printf("\r\n### CMSIS-RTX Hello World ASF Example for Nucleo-L452RE board: Date: %s - %s ###\r\n",
        __DATE__, __TIME__);

    /* Display System clock information */
    D0_printf("System Clocks:\r\n");
    D0_printf("\tCore Clk - %ld\r\n", SystemCoreClock);
    D0_printf("\tSYSCLK   - %ld\r\n", HAL_RCC_GetSysClockFreq());
    D0_printf("\tHCLK     - %ld\r\n", HAL_RCC_GetHCLKFreq());
    D0_printf("\tPCLK1    - %ld\r\n", HAL_RCC_GetPCLK1Freq());
    D0_printf("\tPCLK2    - %ld\r\n", HAL_RCC_GetPCLK2Freq());

    D0_printf("Device SNo.: %08X-%08X-%08X\r\n", gDevUniqueId->uidWords[2],
        gDevUniqueId->uidWords[1], gDevUniqueId->uidWords[0]);
    D0_printf("\t%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\r\n",
        gDevUniqueId->uidBytes[11], gDevUniqueId->uidBytes[10], gDevUniqueId->uidBytes[9],
        gDevUniqueId->uidBytes[8], gDevUniqueId->uidBytes[7], gDevUniqueId->uidBytes[6],
        gDevUniqueId->uidBytes[5], gDevUniqueId->uidBytes[4], gDevUniqueId->uidBytes[3],
        gDevUniqueId->uidBytes[2], gDevUniqueId->uidBytes[1], gDevUniqueId->uidBytes[0]);
}


extern AsfTaskHandle asfTaskHandleTable[];
/*----------------------------------------------------------------------------
  Task 1 'ledOn': switches the LED on
 *---------------------------------------------------------------------------*/
ASF_TASK void LED_On_Task( void )
{
    uint8_t ledID = 0;
    for (;;)
    {
        LED_On(ledID);                      /* Turn LED On                   */
        osSignalSet( asfTaskHandleTable[LED_OFF_TASK_ID].handle, 0x0001 );
        ASFTaskSleep(100);
        //D0_printf("Timer Count: %lu\r\n", RTC_GetCounter());
        ledID = (ledID + 1) % NUM_LEDS;
    }
}

/*----------------------------------------------------------------------------
  Task 2 'ledOff': switches the LED off
 *---------------------------------------------------------------------------*/
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
        ASFTaskSleep(20);                   /* delay 20ms                        */
        LED_Off(ledID);                     /* Turn LED Off                      */
        ledID = (ledID + 1) % NUM_LEDS;
    }
}

#ifdef INCLUDE_TEST_TASK
ASF_TASK void LED_Test_Task( void )
{
    uint32_t counter = 0;

    while(1)
    {
        D0_printf("This is test task - %u\r\n", counter++);
        ASFTaskSleep(1000);
    }
}
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

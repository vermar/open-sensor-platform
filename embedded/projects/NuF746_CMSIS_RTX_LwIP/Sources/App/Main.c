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
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "ethernetif.h"
#include "hw_setup.h"
#include "httpserver-netconn.h"
#include "app_ethernet.h"

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
#define configMINIMAL_STACK_SIZE        128

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
struct netif gnetif; /* network interface structure */

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
static void Netif_Config(void);

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
    /* Configure the MPU attributes as Write Through */
    MPU_Config();

    /* Enable the CPU Cache */
    CPU_CACHE_Enable();

    /* STM32F7xx HAL library initialization:
       - Configure the Flash ART accelerator on ITCM interface
       - Configure the Systick to generate an interrupt each 1 msec
       - Set NVIC Group Priority to 4
       - Low Level Initialization
    */
    HAL_Init();

    // Configure the System clock to have a frequency of 216 MHz
    SystemClock_Config();
    SystemCoreClockUpdate();

    /* Configure RTC */
    RTC_Configuration(); //Configuring RTC after SystemCoreClockUpdate to support HAL_GetTick & HAL_Delay calls

    /* Configure the GPIO ports (non module specific) */
    SystemGPIOConfig();

    /* Set startup state of LEDs */
    LED_Init();                   /* Initialize Debug LEDs */
    LED_On(FRONT_LED); //Visual indication that we powered up

    /* Configure debug UART port - we do it here to enable assert messages early in the system */
    DebugPortInit();
    DebugUARTConfig( DBG_UART_BAUD, UART_WORDLENGTH_8B, UART_STOPBITS_1, UART_PARITY_NONE );

    /* Print version number */
    printf("\r\n### CMSIS-RTX Hello World Application Example for Nucleo-F746 board: Date: %s - %s ###\r\n",
        __DATE__, __TIME__);

    /* Display System clock information */
    D0_printf("System Clocks:\r\n");
    D0_printf("\tCore Clk - %ld\r\n", SystemCoreClock);
    D0_printf("\tSYSCLK   - %ld\r\n", HAL_RCC_GetSysClockFreq());
    D0_printf("\tHCLK     - %ld\r\n", HAL_RCC_GetHCLKFreq());
    D0_printf("\tPCLK1    - %ld\r\n", HAL_RCC_GetPCLK1Freq());
    D0_printf("\tPCLK2    - %ld\r\n", HAL_RCC_GetPCLK2Freq());
    D0_printf("RTC Prescalar: %ld\r\n", RTC_PRESCALAR);
#if 0
    D0_printf("Device SNo.: %08X-%08X-%08X\r\n", gDevUniqueId->uidWords[2],
        gDevUniqueId->uidWords[1], gDevUniqueId->uidWords[0]);
    D0_printf("\t%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X.%02X\r\n",
        gDevUniqueId->uidBytes[11], gDevUniqueId->uidBytes[10], gDevUniqueId->uidBytes[9],
        gDevUniqueId->uidBytes[8], gDevUniqueId->uidBytes[7], gDevUniqueId->uidBytes[6],
        gDevUniqueId->uidBytes[5], gDevUniqueId->uidBytes[4], gDevUniqueId->uidBytes[3],
        gDevUniqueId->uidBytes[2], gDevUniqueId->uidBytes[1], gDevUniqueId->uidBytes[0]);
#endif

  /* Create tcp_ip stack thread */
  tcpip_init(NULL, NULL);

  /* Initialize the LwIP stack */
  Netif_Config();

  /* Initialize webserver demo */
  http_server_netconn_init();

  /* Notify user about the network interface config */
  User_notification(&gnetif);

#if 0 //def USE_DHCP1
  /* Start DHCPClient */
#if defined(__GNUC__)
  osThreadDef(DHCP, DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 5);
#else
  osThreadDef(DHCP_thread, osPriorityBelowNormal, 0, configMINIMAL_STACK_SIZE * 2);
#endif

  osThreadCreate (osThread(DHCP_thread), &gnetif);
#endif
}


/**
  * @brief  Initializes the lwIP stack
  * @param  None
  * @retval None
  */
static void Netif_Config(void)
{
  struct ip_addr ipaddr;
  struct ip_addr netmask;
  struct ip_addr gw;

  /* IP address setting */
  IP4_ADDR(&ipaddr, IP_ADDR0, IP_ADDR1, IP_ADDR2, IP_ADDR3);
  IP4_ADDR(&netmask, NETMASK_ADDR0, NETMASK_ADDR1 , NETMASK_ADDR2, NETMASK_ADDR3);
  IP4_ADDR(&gw, GW_ADDR0, GW_ADDR1, GW_ADDR2, GW_ADDR3);

  /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
  struct ip_addr *netmask, struct ip_addr *gw,
  void *state, err_t (* init)(struct netif *netif),
  err_t (* input)(struct pbuf *p, struct netif *netif))

  Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your ethernet netif interface. The following code illustrates it's use.*/

  netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input);

  /*  Registers the default network interface. */
  netif_set_default(&gnetif);

  if (netif_is_link_up(&gnetif))
  {
    /* When the netif is fully configured this function must be called.*/
    netif_set_up(&gnetif);
  }
  else
  {
    /* When the netif link is down this function must be called */
    netif_set_down(&gnetif);
  }
}

/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

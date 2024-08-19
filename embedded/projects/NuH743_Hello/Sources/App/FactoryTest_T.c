/*==================================================================================================
** Copyright (C) 2024 Rajiv Verma
**
** Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except
** in compliance with the License. You may obtain a copy of the License at:
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**==================================================================================================
**  @file FactoryTest_T.c
**  @brief Implements handling for CLI based testing and configuration requests.
**
*/
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "common.h"
#include <string.h>
#include "FWVersion.h"
#include "I2C_CmnDriver.h"

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define THIS_TASK_ID                    FACTORY_TEST_TASK_ID

//==================================================================================================
//    P R I V A T E   T Y P E   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
static I2CDriverHandle_t s_hI2C_ChX_100K;
static I2CDriverHandle_t s_hI2C_ChY_100K;

//==================================================================================================
//    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    P R I V A T E     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Initialize the I2C Interface for channel X (I2C IF1)
**
** @param  None
**
** @return None
*/
static void initI2CInterfaceChX(void)
{
    /* Init IF HW IO pins */
    I2C_IF1_HardwareSetup();

    /* Init I2C Master interface */
    s_hI2C_ChX_100K = I2C_Master_Initialise(I2C_IF1_BUS, I2C_BUS_CLOCK_400K, THIS_TASK_ID);
    ASF_assert(s_hI2C_ChX_100K != NULL);
}

/***************************************************************************************************
** @brief Initialize the I2C Interface for channel Y (I2C IF2)
**
** @param  None
**
** @return None
*/
static void initI2CInterfaceChY(void)
{
    /* Init IF HW IO pins */
    I2C_IF2_HardwareSetup();

    /* Init I2C Master interface */
    s_hI2C_ChY_100K = I2C_Master_Initialise(I2C_IF2_BUS, I2C_BUS_CLOCK_400K, THIS_TASK_ID);
    ASF_assert(s_hI2C_ChY_100K != NULL);
}

//==================================================================================================
//    P U B L I C     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief This task handles Factory mode Test, Configuration and Calibration requests.
**
**  Input:
** (see definition for ASF_TASK_ARG)
**
** @return (see definition for ASF_TASK - typically void)
*/
ASF_TASK void FactoryModeTask(ASF_TASK_ARG)
{
    MessageBuffer* rcvMsg = NULLP;

    initI2CInterfaceChX();
    initI2CInterfaceChY();

    D0_printf("### %s Running ###\r\n", __MODULE__);

    while (1)
    {
        ASFReceiveMessage(THIS_TASK_ID, &rcvMsg);

        switch (rcvMsg->msgId)
        {
        case MSG_CLI_CMD:
            switch (rcvMsg->msg.msgCliCmd.cmd)
            {
            case 'g': /* GPIO state set/query using schematics name */
            {
                char szBuffer[17] = { 0 };
                char opt;
                int32_t numSc;
                uint32_t val = 0;

                D0_printf(G_BLUE"GPIO Control: <'G'|'S'>,<'ALL'|name>, {0|1}\r\n"G_NORM);
                numSc = sscanf(rcvMsg->msg.msgCliCmd.u.cmdStr, "%c,%16[^,],%lu", &opt, szBuffer, &val);
                if (numSc >= 2)
                {
                    switch (opt)
                    {
                    case 'G':
                        if (strcmp(szBuffer, "ALL") == 0)
                        {
                            DumpGpioInputStatusAll();
                            DumpGpioOutputStatusAll();
                        }
                        else
                        {
                            GpioState_t state = GetGpioStateByName(szBuffer);
                            if (state != GPIO_INVALID)
                            {
                                D0_printf("\t%16s : %u\r\n", szBuffer, state);
                            }
                            else
                            {
                                D0_printf(G_RED "\tInvalid GPIO name\r\n" G_NORM);
                            }
                        }
                        break;

                    case 'S':
                        if (strcmp(szBuffer, "ALL") == 0)
                        {
                            D0_printf(G_RED "\tInvalid 'Set' option. Cannot be 'ALL'\r\n" G_NORM);
                        }
                        else if (numSc == 3)
                        {
                            SetGpioStateByName(szBuffer, (GpioState_t)val);
                        }
                        else
                        {
                            D0_printf(G_RED "\tInvalid params (%c), (%s), (%d)\r\n" G_NORM, opt, szBuffer, val);
                        }
                        break;

                    default:
                        D0_printf(G_RED "\tInvalid option. Expected 'G' for Get & 'S' for Set\r\n" G_NORM);
                        break;
                    }
                }
                break;
            }

            case 'i': /* I2C Scan */
                if ((rcvMsg->msg.msgCliCmd.u.value[1] <= 0x7F) && (rcvMsg->msg.msgCliCmd.u.value[2] <= 0x7F))
                {
                    I2CDriverHandle_t* phI2C;
                    if (rcvMsg->msg.msgCliCmd.u.value[0] == 0)
                    {
                        phI2C = &s_hI2C_ChX_100K;
                        D0_printf(G_BLUE "Scanning Ch-X I2C Addresses: 0x%02X through 0x%02X\r\n" G_NORM, rcvMsg->msg.msgCliCmd.u.value[1], rcvMsg->msg.msgCliCmd.u.value[2]);
                    }
                    else if (rcvMsg->msg.msgCliCmd.u.value[0] == 1)
                    {
                        phI2C = &s_hI2C_ChY_100K;
                        D0_printf(G_BLUE "Scanning Ch-Y I2C Addresses: 0x%02X through 0x%02X\r\n" G_NORM, rcvMsg->msg.msgCliCmd.u.value[1], rcvMsg->msg.msgCliCmd.u.value[2]);
                    }
                    else
                    {
                        D0_printf(G_RED "Wrong I2C interface value. Try again!\r\n" G_NORM);
                        break;
                    }

                    #define CHARS_PER_LINE  DPRINTF_BUFF_SIZE
                    uint8_t devCnt = 0;
                    char buffer[CHARS_PER_LINE];
                    uint32_t n = 0;

                    for (uint8_t addr = rcvMsg->msg.msgCliCmd.u.value[1]; addr <= rcvMsg->msg.msgCliCmd.u.value[2]; addr++)
                    {
                        if (I2C_Ping_Device(*phI2C, addr))
                        {
                            n += snprintf(&buffer[n], (CHARS_PER_LINE - n), G_MAGENTA_BOLD "%02X " G_NORM, addr);
                            //D0_printf(G_BLUE_BOLD "%02X - " G_NORM, addr);
                        }
                        else
                        {
                            n += snprintf(&buffer[n], (CHARS_PER_LINE - n), "xx ");
                            //D0_printf(G_CYAN "XX - " G_NORM);
                        }

                        if (++devCnt % 16 == 0)
                        {
                            D0_printf("%s\r\n", buffer);
                            n = 0;
                        }
                    }
                    D0_printf("%s\r\n", buffer);
                    D0_printf(G_GREEN ":Done:\r\n" G_NORM);
                }
                else
                {
                    D0_printf(G_BLUE "I2C Address Scan: <I2C I/F {0|1}>,<Start-7bit-Addr>,<End-7bit-Addr>\r\n" G_NORM);
                }
                break;

            case 'R':   //HW Revision Check
                D0_printf(G_BLUE"%s HW REV %u Detected\r\n"G_NORM, THIS_BOARD, gHardwareRev);
#if defined (__ICCARM__)
                D0_printf(G_BLUE"\tCompiler: IAR C/C++ Compiler for Arm\r\n");
                D0_printf("\tVersion: %u.%u.%u\r\n"G_NORM, _ICC_MAJOR_, _ICC_MINOR_, _ICC_PATCH_);
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
#endif
                D0_printf(G_BLUE"FW Version:\r\n"G_NORM);
                D0_printf("\tMajor - %u\r\n", FW_VER_MAJOR);
                D0_printf("\tMinor - %u\r\n", FW_VER_MINOR);
                D0_printf("\tPatch - %u\r\n", FW_VER_PATCH);
                D0_printf("\tBuild - %04X\r\n", FW_VER_BUILD);
                D0_printf("\tRelease - %lu\r\n\n", FW_VER_RELEASE);
                break;

            case 'Z':   //Soft reset the board
                NVIC_SystemReset();
                break;

            default:
                D0_printf(G_RED"Warning! Command Not Recognized!\r\n"G_NORM);
                break;
            }
            break;

        default:
            D0_printf("%s: Unhandled Message ID: %u\r\n", __MODULE__, rcvMsg->msgId);
            break;
        }
    }
}

//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

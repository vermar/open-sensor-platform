/*==================================================================================================
** Copyright (C) 2024 Rajiv Verma
**
** Adapted from https://github.com/vermar/open-sensor-platform
** Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except
** in compliance with the License. You may obtain a copy of the License at:
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**==================================================================================================
**  @file <File-Name>
**  @brief Implements ABC functionality for the system
**
**  This para is optional and can contain detailed description of the file contents, notes or other
**  useful information.
**
**  @author <Developer's email address/name>
**  @author <Add if more than one authors>
*/
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "common.h"

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define THIS_TASK_ID                    <TASKNAME>_TASK_ID

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

//==================================================================================================
//    P R I V A T E     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Local/Static functions may use this short header
**
** Optional details about the function
**
**  Input:
** @param None
**
** @return None
*/
static void staticFunc1(void)
{
    /* Code */
}

//==================================================================================================
//    P U B L I C     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Parse incoming control session messages and invoke handlers for the same
**
** <Optional details about the function>
**
**  Input:
** @param pPktBuff:     Pointer to received packet data
** @param bufLen:       Length of the received data packet.
**
**  Output:
** @param pParsed:      Malloc-ed buffer containing the parsed message.
**
** @return TRUE:        Message was parsed
** @return FALSE:       If error occurred
*/
osp_bool_t ControlMsgParse(uint8_t *pPktBuff, uint16_t bufLen, uint8_t *pParsed)
{
    /* Code */
}

/***************************************************************************************************
** @brief Interface task for...
**
** <Optional details about the function>
**
**  Input:
** (see definition for ASF_TASK_ARG)
**
** @return (see definition for ASF_TASK - typically void)
*/
ASF_TASK void <TaskName>Task(ASF_TASK_ARG)
{
    MessageBuffer* rcvMsg = NULLP;

    D0_printf("### %s Running ###\r\n", __MODULE__);

    /* Initialize any HW resource controlled by this task */

    /* (Optional) Start any periodic timer */
    //ASFTimerStart(THIS_TASK_ID, <YOUR>_TIMER_REF, MSEC_TO_TICS(200), &testTimer);

    while (1)
    {
        ASFReceiveMessage(THIS_TASK_ID, &rcvMsg);

        switch (rcvMsg->msgId)
        {
        case MSG_TO_BE_HANDLED:
            break;

            /* Handle timer expiry for all timers created by this task */
        case MSG_TIMER_EXPIRY:
            switch (rcvMsg->msg.msgTimerExpiry.userValue)
            {
            case <YOUR>_TIMER_REF:
                break;

            default:
                D0_printf("%s: Unknown Timer: %X\r\n", __MODULE__, rcvMsg->msg.msgTimerExpiry.userValue);
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

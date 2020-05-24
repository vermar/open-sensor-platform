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
#include <string.h>


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern PortInfo gDbgUartPort;
extern void CmdParse_User( int8_t *pBuffer, uint16_t size, uint16_t event );

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
typedef enum _ReadStatus
{
    READ_OK,
    READ_CURSOR_UP = EVT_FLAG_CURSOR_UP,
    READ_CURSOR_DN = EVT_FLAG_CURSOR_DN,
    READ_ERR = 0xF0
} ReadStatus_t;

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static int8_t inputBuffer[COMMAND_LINE_SIZE];

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      SerialRead
 *          This function reads 'length' bytes and copies the data into memory pointed to by pBuff.
 *          The function returns the number of bytes actually read in pBytesRead. If an error has
 *          occurred, the function returns immediately; otherwise the
 *          function does not return until the specified number of bytes has been received and copied
 *          into the buffer. If not enough received data bytes are available in the driver's internal
 *          storage area, the calling task is suspended (blocked) until the required number of bytes
 *          is available.
 *
 * @param   byte received byte
 *
 * @return  APP_OK Data received properly
 *
 ***************************************************************************************************/
static ReadStatus_t SerialRead( PortInfo *pPort, int8_t *pBuff, uint16_t length, uint16_t *pBytesRead )
{
    uint16_t readIdx, writeIdx;
    uint16_t bytesRead = 0, remaining;
    ReadStatus_t  retVal = READ_OK;
    uint16_t evtFlags = 0;
    osEvent  ret;
    OS_SETUP_CRITICAL();

    if ((pBuff == NULL) || (length == 0) || (length > RX_BUFFER_SIZE))
    {
        return READ_ERR;
    }

    while (bytesRead < length)
    {
        /* Wait here for any ISR event */
        evtFlags = 0;
        ret = osSignalWait(0, osWaitForever); //0 => Any signal will resume thread
        if (ret.status == osEventSignal)
        {
            evtFlags = ret.value.signals;
        }

        if (evtFlags & EVT_FLAG_CURSOR_DN)
        {
            return READ_CURSOR_DN;
        }
        if (evtFlags & EVT_FLAG_CURSOR_UP)
        {
            return READ_CURSOR_UP;
        }

        /* Snapshot the read/write index of the receive buffer for local use */
        OS_ENTER_CRITICAL();
        readIdx = pPort->rxReadIdx;
        writeIdx = pPort->rxWriteIdx;
        OS_LEAVE_CRITICAL();

        /* If the write index is 1 in front of read, the buffer is now empty. */
        if (writeIdx == ((readIdx + 1) % RX_BUFFER_SIZE))
        {
            continue;
        }
        else
        {
            remaining = readIdx > writeIdx ? (writeIdx + RX_BUFFER_SIZE - (readIdx + 1)) : writeIdx - (readIdx + 1);
            /* Copy the full command string to given buffer */
            while (remaining)
            {
                readIdx = (readIdx + 1) % RX_BUFFER_SIZE;
                pBuff[bytesRead++] = pPort->rxBuffer[readIdx];
                remaining--;
#if 0 /* Following (untested) maybe useful if commands are being sent by some script and you may end up receiving more
       * than one command or a command and a half! */
                /* If we have a complete command in the buffer then just exit for now */
                if (pBuff[bytesRead - 1] == '\r' || pBuff[bytesRead - 1] == '\n')
                {
                    break;
                }
#endif
            }
            /* Update the read index in port structure */
            OS_ENTER_CRITICAL();
            pPort->rxReadIdx = readIdx;
            OS_LEAVE_CRITICAL();

            if (evtFlags & UART_CRLF_RECEIVE)
            {
                break;
            }
        }
    }

    if (pBytesRead)
    {
        *pBytesRead = bytesRead;
    }

    return retVal;
}


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      CmdHandlerTask
 *          This task is responsible for command parsing and setting configuration values read from
 *          serial port.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
ASF_TASK void CmdHandlerTask( ASF_TASK_ARG )
{
    ReadStatus_t retVal;
    uint16_t bytesRead;

    while(1)
    {
        retVal = SerialRead( &gDbgUartPort, inputBuffer, COMMAND_LINE_SIZE-1, &bytesRead );
        if (retVal != READ_ERR)
        {
            CmdParse_User( inputBuffer, bytesRead, (uint16_t)retVal ); //Implemented in application code
        }
    }
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

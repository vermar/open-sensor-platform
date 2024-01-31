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
#include <string.h>

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define CMD_HISTORY_SZ                  10
#define CMD_EXTRA_OPTIONS_START         6

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static uint8_t CmdHistory[CMD_HISTORY_SZ][COMMAND_LINE_SIZE] = {0};

static uint8_t _newCmdIdx = 0;
static uint8_t _totalCmds = 0;

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

#if defined ON_DEMAND_PROFILING && defined ASF_PROFILING
/****************************************************************************************************
 * @fn      SendProfilingReq
 *          Sends message to the instrumentation task that causes profiling data to be displayed
 *
 ***************************************************************************************************/
static void SendProfilingReq( void )
{
    MessageBuffer *pSendMsg = NULLP;
    AsfResult_t result;
    result = ASFCreateMessage( MSG_PROFILING_REQ, sizeof(MsgNoData), &pSendMsg );
    ASF_assert(result == ASF_OK);
    result = ASFSendMessage( INSTR_MANAGER_TASK_ID, pSendMsg );
    ASF_assert(result == ASF_OK);
}
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      CmdParse_User
 *          Called by command handler task to parse input serial string
 *
 * @param   pBuffer: pointer to received buffer
 * @param   size: number of bytes received (always < COMMAND_LINE_SIZE)
 * @param   event: contains Cursor-Up or Cursor-Down event when received on console, else 0
 *
 * @return  none
 *
 ***************************************************************************************************/
void CmdParse_User( int8_t *pBuffer, uint16_t size, uint16_t event )
{
    //MessageBuffer* pSendMsg = NULLP;
    //AsfResult_t result;
    //int32_t numScanned;
    //MsgCliCmd_t cliCmd = { 0 };
    static uint8_t *pHistCmd = NULL;
    static int8_t histReadIdx = -1;
    static uint8_t histCnt = 0;

    /* NOTE: The following logic implements a simple command history scheme where it buffers
     * the last N (=CMD_HISTORY_SZ) commands in an array indexed as circular buffer. The behavior
     * is as follows:
     * - Up-Arrow key will start from the last command entered upto the number of commands in the
     *   buffer (max defined by CMD_HISTORY_SZ) and then stop (does not roll-over)
     * - Commands from history buffer CANNOT BE EDITED
     * - History commands when executed do not create new entry for the history
     * - After execution of command (whether from history buffer or live entry) the history index
     *   is reset so that the Up-arrow will again start from the last live-command entry
     * - Down-Arrow key will similarly cycle down till the most recent entry and clear the line after
     *   that.
     * - Command repetition is not checked so it possible to have multiple identical commands in the
     *   history if entered as such.
     */
    if (event == EVT_FLAG_CURSOR_UP)
    {
        if (_totalCmds > 0)
        {
            if (histReadIdx < 0)
            {
                histReadIdx = (_newCmdIdx + CMD_HISTORY_SZ - 1) % CMD_HISTORY_SZ;
                histCnt++;
            }
            else if (histCnt < _totalCmds)
            {
                histReadIdx = (histReadIdx + CMD_HISTORY_SZ - 1) % CMD_HISTORY_SZ;
                histCnt++;
            }
            pHistCmd = CmdHistory[histReadIdx];
            D0_printf("\r" CURSOR_DN ERASE_LINE "%s", pHistCmd);
        }
        return;
    }

    if (event == EVT_FLAG_CURSOR_DN)
    {
        if (histCnt > 0)
        {
            histCnt--;
            histReadIdx = (histReadIdx + 1) % CMD_HISTORY_SZ;
            pHistCmd = (histReadIdx != _newCmdIdx)? CmdHistory[histReadIdx] : NULL;
            D0_printf("\r" ERASE_LINE "%s", pHistCmd ? (char*)pHistCmd : "");
        }
        return;
    }

    if (!event)
    {
#if defined ON_DEMAND_PROFILING && defined ASF_PROFILING
        if ((pHistCmd == NULL) && (pBuffer[0] == TOKEN_STATS))
        {
            SendProfilingReq();
            return;
        }
#endif
        if (pHistCmd)
        {
            pBuffer = (int8_t*)pHistCmd;
            size = strlen((char*)pHistCmd);
            D0_printf("\r\nHistory Command[%d]: \"%s\"\r\n", histReadIdx, pHistCmd);
        }
        else
        {
            pBuffer[size - 1] = '\0'; //Remove 'CR' from the command string and terminate it
            /* char *strncpy(char *dest, const char *src, size_t n);
             * If there is no null byte among the first n bytes of src, the string placed in dest
             * will not be null-terminated.
             * If the length of src is less than n, strncpy() writes additional null bytes to dest
             * to ensure that a total of n bytes are written*/
            strncpy((char*)&CmdHistory[_newCmdIdx][0], (char*)pBuffer, size);
            _newCmdIdx = (_newCmdIdx + 1) % CMD_HISTORY_SZ;
            if (_totalCmds < CMD_HISTORY_SZ)
            {
                _totalCmds++;
            }
            D0_printf("\r\nCommand: \"%s\"\r\n", pBuffer);
        }
        histCnt = 0;
        histReadIdx = -1;
        pHistCmd = NULL;
#if 0
        numScanned = sscanf((char*)pBuffer, "cmd=%c,%ld,%ld,%ld", &cliCmd.cmd, &cliCmd.value, &cliCmd.value2,
            &cliCmd.value3);
        if ((numScanned > 0) && (numScanned <= 4))
        {
            result = ASFCreateMessage(MSG_CLI_CMD, sizeof(MsgCliCmd_t), &pSendMsg);
            ASF_assert(result == ASF_OK);
            pSendMsg->msg.msgCliCmd = cliCmd;
            result = ASFSendMessage(MP3_APP_TASK_ID, pSendMsg);
            ASF_assert(result == ASF_OK);
        }
#endif
    }
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

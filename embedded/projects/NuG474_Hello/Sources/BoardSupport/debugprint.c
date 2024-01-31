/* Open Sensor Platform Project
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
#include "debugprint.h"
#include "asf_taskstruct.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
#ifdef __GNUC__
int __io_putchar(int ch);
#else
int32_t ser_putchar (int32_t c);
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define DEL_CHAR_W_ECHO             "\x08 \x08"
#define DEL_CHAR_NO_ECHO            " \x08"
#define ESC_CHAR                    0x1B

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Support for ANSI escape-sequence handling */
typedef enum _EscSeq
{
    ES_NONE,
    ES_IN_ESC_SEQ1,
    ES_IN_ESC_SEQ2,
    ES_CURSOR_UP,
    ES_CURSOR_DN,
    ES_SKIP,
} EscSeq_t;

/* The following structure is only used for defining memory blocks size and not used in the code. In essence
* it replicates the first two members of PktBuff_t (common.h) and adds the actual buffer instead
* of pointer */
typedef struct _TxDmaBuff
{
    uint32_t *pNext;    //link to next buffer
    uint32_t  bufLen;   //Length of data in this buffer
    uint8_t   buff[DPRINTF_BUFF_SIZE];   //data buffer
} TxDmaBuff_t;


/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* UART handler declaration */
static UART_HandleTypeDef s_uartHandle;
static DMA_HandleTypeDef s_txDmaHandle;
static osp_bool_t s_enEcho = false;
static osPoolId s_bufPoolId;

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
PortInfo gDbgUartPort;      //Debug information port

osPoolDef(gDbgTxDmaPool, MAX_DPRINTF_MESSAGES, TxDmaBuff_t); //Declare memory pool for transmit buffers
const osPoolDef_t* gDbgPrintPool = osPool(gDbgTxDmaPool);

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      CheckAnsiEsc
 *          Checks for start of Escape sequence and identifies some common ones used
 *
 ***************************************************************************************************/
static EscSeq_t CheckAnsiEsc(uint8_t inByte)
{
    static EscSeq_t esState = ES_NONE;
    EscSeq_t ret = ES_SKIP;

    switch (inByte)
    {
    case ESC_CHAR:
        if ((esState == ES_NONE) || (esState == ES_CURSOR_UP) || (esState == ES_CURSOR_DN))
        {
            esState = ES_IN_ESC_SEQ1;
        }
        else
        {
            esState = ret = ES_NONE;
        }
        break;

    case '[':
        if (esState == ES_IN_ESC_SEQ1)
        {
            esState = ES_IN_ESC_SEQ2;
        }
        else
        {
            esState = ret = ES_NONE;
        }
        break;

    case 'A':
        if (esState == ES_IN_ESC_SEQ2)
        {
            esState = ret = ES_CURSOR_UP;
        }
        else
        {
            esState = ret = ES_NONE;
        }
        break;

    case 'B':
        if (esState == ES_IN_ESC_SEQ2)
        {
            esState = ret = ES_CURSOR_DN;
        }
        else
        {
            esState = ret = ES_NONE;
        }
        break;

    default:
        esState = ret = ES_NONE;
        break;
    }
    return ret;
}

#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      RemoveFromList
 *          Removes head object (in FIFO order) from list and returns pointer to the list object
 *          Note that this function is always called from ISR context.
 *
 ***************************************************************************************************/
static void *RemoveFromList( PortInfo *pPort )
{
    /* At this point the DMA has consumed this buffer. It will be removed from the list and the
       returned buffer pointer will be used to free the printf buffer. The DMA completion handler
       should call this function, free the memory and then use the buffer pointed by the Head pointer */
    void *pTemp;

    if (pPort->pHead == NULL)
    {
        return NULL;
    }

    pTemp = pPort->pHead;
    pPort->pHead = (void *)M_NextBlock(pPort->pHead); //If this is the last element then spHead will now be NULL
    /* Here we should check if spHead is NULL and correspondingly set spTail to NULL but since
       we probably won't check for spTail == NULL, we skip that step here. */

    return pTemp;
}
#endif

#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      AddToList
 *          Adds the buffer to list and starts DMA transfer if this was the first buffer
 *
 ***************************************************************************************************/
static void AddToList( PortInfo *pPort, void *pPBuff, uint16_t length )
{
    /* New printf buffers will be added to the list of buffers that will eventually be used for
       DMAing out the data */
    OS_SETUP_CRITICAL();
    Address *pTemp;
    Address *pObj = M_GetBuffBlock(pPBuff);
    M_SetBuffLen(pObj, length);

    OS_ENTER_CRITICAL();
    if (pPort->pHead == NULL)
    {
        pPort->pTail = pPort->pHead = pObj; //First DWORD is reserved for list management
        M_NextBlock(pObj) = NULL;
        OS_LEAVE_CRITICAL();

        /* Start the first DMA transfer */
        UartTxDMAStart( pPort, pPBuff, length );
    }
    else
    {
        pTemp = pPort->pTail;
        pPort->pTail = pObj;
        M_NextBlock(pObj) = NULL;
        M_NextBlock(pTemp) = pObj;
        OS_LEAVE_CRITICAL();
    }
}
#endif

#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      GetNextBuffer
 *          Called from DMA TC ISR to request next buffer to process
 *
 ***************************************************************************************************/
static void *GetNextBuffer( PortInfo *pPort )
{
    osStatus status;

    void *pFreeBuff = RemoveFromList( pPort );
    if (pFreeBuff != NULL)
    {
        //Free the current consumed buffer
        status = osPoolFree(pPort->pBuffPool, pFreeBuff);
        ASF_assert(status == osOK);
    }
    return pPort->pHead; //Return the current head of the list
}
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      DebugPortInit
 *          Initializes the Debug Uart port data structure.
 *
 ***************************************************************************************************/
void DebugPortInit( void )
{
    s_bufPoolId = osPoolCreate(gDbgPrintPool);
    ASF_assert( s_bufPoolId != NULL );

    gDbgUartPort.pBuffPool = s_bufPoolId;
    gDbgUartPort.rxWriteIdx = 1;
    gDbgUartPort.rxReadIdx  = 0;
    gDbgUartPort.rcvTask    = CMD_HNDLR_TASK_ID;
#ifndef UART_DMA_ENABLE
    gDbgUartPort.txReadIdx  = 0;
    gDbgUartPort.txWriteIdx = 1;
#else
    gDbgUartPort.pHead      = NULL;
    gDbgUartPort.pTail      = NULL;
    gDbgUartPort.hUart      = &s_uartHandle;
    gDbgUartPort.hDMA       = &s_txDmaHandle;
    gDbgUartPort.UartBaseAddress = 0;   //Not used
    gDbgUartPort.ValidateInput = NULL;
    /* Note functions can be empty but not NULL (coz we dont' check for null) */
    gDbgUartPort.EnableDMAChannel = EnableDbgUartDMAChannel;
    gDbgUartPort.EnableDMATxRequest = EnableDbgUartDMATxRequest;
    gDbgUartPort.EnableDMAxferCompleteInt = EnableDbgUartDMAxferCompleteInt;
#endif
}

/****************************************************************************************************
 * @fn      RxBytesToBuff
 *          This function receives bytes into the RX buffer.  It is called from the receive ISR
 *
 * @param   pPort UART port data structure pointer
 * @param   byte received byte
 *
 * @return  none
 *
 ***************************************************************************************************/
void RxBytesToBuff( PortInfo *pPort, uint8_t byte )
{
    int32_t  remaining;
    uint16_t  readIdx, writeIdx;
    EscSeq_t esState;

    /* Snapshot the two index values for local use. */
    readIdx = pPort->rxReadIdx;
    writeIdx = pPort->rxWriteIdx;

    /* Check if enough room in the buffer to store the new data. */
    remaining = readIdx - writeIdx;
    if(readIdx < writeIdx)
    {
        remaining += RX_BUFFER_SIZE;
    } /* Here, remaining should be correct (between 0 and RX_BUFFER_SIZE-1). */

    if (byte == TOKEN_BS)
    {
        if (remaining < (RX_BUFFER_SIZE-1)) //at least 1 char in the buffer
        {
            if (s_enEcho)
            {
                D0_printf(DEL_CHAR_W_ECHO);
            }
            else
            {
                D0_printf(DEL_CHAR_NO_ECHO);
            }
            pPort->rxWriteIdx = (pPort->rxWriteIdx + RX_BUFFER_SIZE - 1) % RX_BUFFER_SIZE;
        }
        return;
    }
    else if (s_enEcho)
    {
        //Echo back
#ifdef __GNUC__
        __io_putchar(byte);
#else
        ser_putchar(byte);
#endif
    }

    /* Check for ANSI escape sequence */
    esState = CheckAnsiEsc(byte);

    if ((remaining > 0) && (esState != ES_SKIP))
    {
        if (esState == ES_NONE)
        {
            pPort->rxBuffer[writeIdx] = byte;
            writeIdx = (writeIdx + 1) % RX_BUFFER_SIZE;

            /* Check if a task is waiting for it. */
            if (pPort->rcvTask != UNKNOWN_TASK_ID)
            {
                if (byte == '\r' || byte == '\n')
                {
                    osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, UART_CRLF_RECEIVE);
                }
#if 0 //Not needed for commands that end with CR/LF. Also avoids waking up task for each char received
                else
                {
                    /* Wake up the task. */
                    osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, UART_CMD_RECEIVE);
                }
#endif
            }
            /* Update the port control block values */
            pPort->rxWriteIdx = writeIdx;
        }
        else if ((esState == ES_CURSOR_UP) && (pPort->rcvTask != UNKNOWN_TASK_ID))
        {
            osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, EVT_FLAG_CURSOR_UP);
        }
        else if ((esState == ES_CURSOR_DN) && (pPort->rcvTask != UNKNOWN_TASK_ID))
        {
            osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, EVT_FLAG_CURSOR_DN);
        }
    }
    /* If the buffer gets full we still want to wakeup task if user presses 'Enter' */
    else if ((remaining == 0) && ((byte == '\r') || (byte == '\n')) && (pPort->rcvTask != UNKNOWN_TASK_ID))
    {
        osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, UART_CRLF_RECEIVE);
    }
}


/****************************************************************************************************
 * @fn      _dprintf
 *          Helper function that replaces printf functionality to dump the printf messages on UART
 *
 * @param   dbgLvl Debug level for the console print output
 * @param   fmt printf style variable length parameters
 *
 * @return  length of the string printed.
 *
 ***************************************************************************************************/
int _dprintf( uint8_t dbgLvl, const char *fmt, ... )
{
    va_list args;
    const PortInfo *pPort = &gDbgUartPort;
#ifdef UART_DMA_ENABLE
    uint16_t len = 0;
    int8_t *pNewBuff, *pPrintBuff;
#endif

    switch( dbgLvl )
    {
#if (DEBUG_LVL < 1)
        case 1:
            return 0;
#endif
#if (DEBUG_LVL < 2)
        case 2:
            return 0;
#endif

        default:
            va_start( args, fmt );

#ifdef UART_DMA_ENABLE
            pNewBuff = osPoolAlloc(pPort->pBuffPool);
# ifdef PRINTF_POOL_EMPTY_ASSERT
            ASF_assert( pNewBuff != NULL );
# endif
            if (pNewBuff != NULL)
            {
                pPrintBuff = M_GetBuffStart(pNewBuff);
                /* Note: Output will be truncated to allowed max size */
                len = vsnprintf((char*)pPrintBuff, DPRINTF_BUFF_SIZE, fmt, args);

                ASF_assert( len > 0 );

                /* Add the print buffer to list */
                AddToList( (PortInfo*)pPort, pPrintBuff, len );
                return len;
            }
            return 0;
#else
# error Non DMA printf handling not implemented!
            return 0;
#endif
    }
}


/****************************************************************************************************
 * @fn      DbgUartTxCompleteCallback
 *          Called from HAL_UART_TxCpltCallback() for handling Tx-DMA completion to allow queuing the
 *          next print buffer.
 *
 * @param   huart Handle to the USART peripheral
 *
 * @return  none
 *
 ***************************************************************************************************/
void DbgUartTxCompleteCallback(UART_HandleTypeDef *huart)
{
    void *pNewBuf;
    uint8_t *pPrintBuf;
#ifdef CONSOLE_PROMPT
    const char* prompt = CONSOLE_PROMPT;
    static osp_bool_t wasPrompt = false;
#endif // CONSOLE_PROMPT

    /* Disable Transfer Complete interrupt */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TC);

    /* Tx process is ended, restore HAL State to Ready */
    gDbgUartPort.hUart->gState = HAL_UART_STATE_READY;

    /*Get the next print buffer */
    pNewBuf = GetNextBuffer(&gDbgUartPort);
    if (pNewBuf != NULL)
    {
        pPrintBuf = M_GetBuffStart(pNewBuf);
        /* Get the next buffer going */
        UartTxDMAStart(&gDbgUartPort, pPrintBuf, M_GetBuffLen(pNewBuf));
    }
#ifdef CONSOLE_PROMPT
    else
    {
        if (!wasPrompt)
        {
            UartTxDMAStart(&gDbgUartPort, (uint8_t*)prompt, 4);
            wasPrompt = true;
        }
        else
        {
            wasPrompt = false;
        }
    }
#endif
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

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
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
PortInfo gDbgUartPort;      //Debug information port

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
/**
 * Declare separate pool for debug printf messages
 */
#ifdef UART_DMA_ENABLE
# define DPRINTF_MPOOL_SIZE         (DPRINTF_BUFF_SIZE + 8)

uint32_t gMemPoolDprintf[3+((DPRINTF_MPOOL_SIZE+3)/4)*(MAX_DPRINTF_MESSAGES)];
const osPoolDef_t PrintBufPool = { MAX_DPRINTF_MESSAGES, DPRINTF_MPOOL_SIZE, gMemPoolDprintf };
static osPoolId _BufPoolId;
#endif

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

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* UART handler declaration */
static UART_HandleTypeDef _UartHandle;
static DMA_HandleTypeDef _TxDmaHandle;
static osp_bool_t _enEcho = false;

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

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

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      AddToList
 *          Adds the buffer to list and starts DMA transfer if this was the first buffer
 *
 ***************************************************************************************************/
void AddToList( PortInfo *pPort, void *pPBuff, uint16_t length )
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

        /* Start the first DMA transfer */
        UartTxDMAStart( pPort, pPBuff, length );
    }
    else
    {
        pTemp = pPort->pTail;
        pPort->pTail = pObj;
        M_NextBlock(pObj) = NULL;
        M_NextBlock(pTemp) = pObj;
    }
    OS_LEAVE_CRITICAL();
}


/****************************************************************************************************
 * @fn      RemoveFromList
 *          Removes head object (in FIFO order) from list and returns pointer to the list object
 *
 ***************************************************************************************************/
void *RemoveFromList( PortInfo *pPort )
{
    /* At this point the DMA has consumed this buffer. It will be removed from the list and the
       returned buffer pointer will be used to free the printf buffer. The DMA completion handler
       should call this function, free the memory and then use the buffer pointed by the Head pointer */
    void *pTemp;
    OS_SETUP_CRITICAL();

    //ASF_assert(pPort->pHead != NULL);
    if (pPort->pHead == NULL)
    {
        return NULL;
    }
    OS_ENTER_CRITICAL();
    pTemp = pPort->pHead;
    pPort->pHead = (void *)M_NextBlock(pPort->pHead); //If this is the last element then spHead will now be NULL
    /* Here we should check if spHead is NULL and correspondingly set spTail to NULL but since
       we probably won't check for spTail == NULL, we skip that step here. */
    OS_LEAVE_CRITICAL();
    return pTemp;
}
#endif


/****************************************************************************************************
 * @fn      DebugPortInit
 *          Initializes the Debug Uart port data structure.
 *
 ***************************************************************************************************/
void DebugPortInit( void )
{
    _BufPoolId = osPoolCreate( &PrintBufPool );
    ASF_assert( _BufPoolId != NULL );

    gDbgUartPort.pBuffPool = _BufPoolId;
    gDbgUartPort.rxWriteIdx = 1;
    gDbgUartPort.rxReadIdx  = 0;
    gDbgUartPort.rcvTask    = CMD_HNDLR_TASK_ID;
#ifndef UART_DMA_ENABLE
    gDbgUartPort.txReadIdx  = 0;
    gDbgUartPort.txWriteIdx = 1;
#else
    gDbgUartPort.pHead      = NULL;
    gDbgUartPort.pTail      = NULL;
    gDbgUartPort.hUart      = &_UartHandle;
    gDbgUartPort.hDMA       = &_TxDmaHandle;
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
    int32_t  left;
    uint16_t  readIdx, writeIdx;
    EscSeq_t esState;

    /* Snapshot the two index values for local use. */
    readIdx = pPort->rxReadIdx;
    writeIdx = pPort->rxWriteIdx;

    /* Check if enough room in the buffer to store the new data. */
    left = readIdx - writeIdx;
    if(readIdx < writeIdx)
    {
        left += RX_BUFFER_SIZE + 1;
    } /* Here, left should be correct (between 0 and RX_BUFFER_SIZE). */
    
    if (byte == TOKEN_BS)
    {
        if (left < RX_BUFFER_SIZE) //at least 1 char in the buffer
        {
            if (_enEcho)
            {
                //ser_putchar(TOKEN_BS);
                D0_printf(DEL_CHAR_W_ECHO);
            }
            else
            {
                D0_printf(DEL_CHAR_NO_ECHO);
            }
            //ser_putchar(' ');
            //ser_putchar(TOKEN_BS);
            //backtrack one byte
            pPort->rxWriteIdx--;
        }
        return;
    }
    else if (_enEcho)
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

    if ((left > 0) && (esState != ES_SKIP))
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
        }
        else if (esState == ES_CURSOR_UP)
        {
            osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, EVT_FLAG_CURSOR_UP);
            return;
        }
        else if (esState == ES_CURSOR_DN)
        {
            osSignalSet(asfTaskHandleTable[pPort->rcvTask].handle, EVT_FLAG_CURSOR_DN);
            return;
        }
    }

    /* Update the port control block values */
    pPort->rxWriteIdx = writeIdx;
}


#ifdef UART_DMA_ENABLE
/****************************************************************************************************
 * @fn      GetNextBuffer
 *          Called from DMA TC ISR to request next buffer to process
 *
 ***************************************************************************************************/
void *GetNextBuffer( PortInfo *pPort )
{
    void *pFreeBuff = RemoveFromList( pPort );
    if (pFreeBuff != NULL)
    {
        ASF_assert(osPoolFree(pPort->pBuffPool, pFreeBuff) == osOK); //Free the current consumed buffer
    }
    return pPort->pHead; //Return the current head of the list
}
#endif

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
 * @fn      HAL_UART_TxCpltCallback
 *          Callback function defined 'weak' in HAL and can be overridden by user. This is invoked by
 *          HAL_UART_IRQHandler(). For DMA based UART transmit, this function is invoked because the
 *          USART Transmit Complete Interrupt is enabled as part of DMA transfer complete interrupt
 *          handling. Compared to the Std.Periph. Driver implementation this implementation has this
 *          extra interrupt invocation and handling as opposed to just having the DMA TC interrupt
 *          take care of queuing the next print buffer.
 *
 * @param   huart Handle to the USART peripheral
 *
 * @return  none
 *
 ***************************************************************************************************/
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    void *pNewBuf;
    uint8_t *pPrintBuf;
#ifdef CONSOLE_PROMPT
    const char* prompt = CONSOLE_PROMPT;
    static osp_bool_t wasPrompt = false;
#endif // CONSOLE_PROMPT

    /* Disable Transfer Complete interrupt */
    __HAL_UART_DISABLE_IT(huart, UART_IT_TC);

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

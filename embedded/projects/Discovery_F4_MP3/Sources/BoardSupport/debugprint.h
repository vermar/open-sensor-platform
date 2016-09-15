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
#if !defined (DEBUGPRINT_H)
#define   DEBUGPRINT_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include "common.h"

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Platform/Device dependent macros */
void UartTxDMAStart( PortInfo *pPort, uint8_t *pTxBuffer, uint16_t txBufferSize );

#ifndef UART_DMA_ENABLE
bool_t GetNextByteToTx( uint8_t* pucByte );
#else
void *GetNextBuffer( PortInfo *pPort );
#endif

/* Support functions for DEBUG Uart */
static __inline void DisableDbgUartInterrupt( void ) {
    NVIC_DisableIRQ(DBG_UART_IRQn);
}

static __inline void EnableDbgUartInterrupt( void ) {
    NVIC_EnableIRQ(DBG_UART_IRQn);
}

#ifdef UART_DMA_ENABLE
static __inline void EnableDbgUartDMAxferCompleteInt( void ) {
}

static __inline void EnableDbgUartDMAChannel( void ) {
}

static __inline void DisableDbgUartDMAChannel( void ) {
}

static __inline void EnableDbgUartDMATxRequest( void ) {
}
#endif

static __inline void DbgUartSendByte( uint8_t byte ) {
    gDbgUartPort.hUart->Instance->DR = (byte & 0xFF);
}

static __inline uint8_t DbgUartReadByte( void ) {
    return ((uint8_t)(gDbgUartPort.hUart->Instance->DR & (uint8_t)0xFF));
}

static __inline osp_bool_t DbgUartTransmitBufferEmpty( void ) {
    if (__HAL_UART_GET_FLAG(gDbgUartPort.hUart, UART_FLAG_TXE) == TRUE) {
        return true;
    }
    return false;
}

static __inline osp_bool_t DbgUartReceiveBufferFull( void ) {
    if (__HAL_UART_GET_FLAG(gDbgUartPort.hUart, UART_FLAG_RXNE) == TRUE) {
        return true;
    }
    return false;
}

static __inline void EnableDbgUartTxBufferEmptyInterrupt( void ) {
    __HAL_UART_ENABLE_IT(gDbgUartPort.hUart, UART_IT_TXE);
}

static __inline void DisableDbgUartTxBufferEmptyInterrupt( void ) {
    __HAL_UART_DISABLE_IT(gDbgUartPort.hUart, UART_IT_TXE);
}



#endif /* DEBUGPRINT_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

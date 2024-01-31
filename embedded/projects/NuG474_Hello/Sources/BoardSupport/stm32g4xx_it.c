/**
  ******************************************************************************
  * @file    stm32g4xx_it.c
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "debugprint.h"


/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
    printf("!!HARD FAULT!!");
    while (1)
    {
        __IO uint32_t i;
        //Fast blink the indicator LED
        LED_On( HARD_FAULT_LED );
        for (i = 0; i < 2000000; i++);
        LED_Off( HARD_FAULT_LED );
        for (i = 0; i < 2000000; i++);
    }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
    /* Go to infinite loop when Usage Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Debug Monitor exception.
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/* STM32G4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32g4xx.s).                    */
/******************************************************************************/


/**
  * @brief  This function handles DMA TX interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is defined in "hw_setup_<board>.h" and related to DMA channel
  *         used for USART data reception
  */
void DBG_UART_DMA_TX_IRQHandler(void)
{
    HAL_DMA_IRQHandler(gDbgUartPort.hUart->hdmatx);
}

/**
  * @brief  This function handles USARTx interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is defined in "hw_setup_<board>.h" and related 
  *         USART data interrupts
  */
void DBG_UART_IRQHandler(void)
{
    uint8_t nextByte;

    if (__HAL_UART_GET_FLAG(gDbgUartPort.hUart, UART_FLAG_RXNE) != RESET)
    {
        /* Read one byte from the receive data register */
        nextByte = DbgUartReadByte();
        RxBytesToBuff( &gDbgUartPort, nextByte );
    }

    /* This HAL handler will handle TX Complete interrupt among other and invoke the
     * (Legacy Weak) callback HAL_UART_TxCpltCallback() from inside the HAL layer
     * This callback is defined in debugprint.c file
     */
    HAL_UART_IRQHandler(gDbgUartPort.hUart);
}


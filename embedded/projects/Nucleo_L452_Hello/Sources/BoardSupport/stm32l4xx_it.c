/**
  ******************************************************************************
  * @file    stm32l4xx_it.c 
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "debugprint.h"
#include "stm32l4xx_it.h"


/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief   This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
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
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
    /* Go to infinite loop when Memory Manage exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
    /* Go to infinite loop when Bus Fault exception occurs */
    while (1)
    {
    }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
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
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/******************************************************************************/
/*                 STM32L4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32l452xx.s).                                             */
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

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    19-September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides all exceptions handler and peripherals interrupt
  *          service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "common.h"
#include "debugprint.h"
#include "stm32f4xx_it.h"
#include "stm32f4_discovery_audio.h"

void RxBytesToBuff( PortInfo *pPort, uint8_t byte );
void I2C_Driver_ERR_ISR_Handler(void);
void I2C_Driver_ISR_Handler(void);

extern HCD_HandleTypeDef hhcd;
extern I2S_HandleTypeDef       hAudioOutI2s;

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
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/


/**
  * @brief  This function handles DMA TX interrupt request.
  * @param  None
  * @retval None
  * @Note   This function is redefined in "main.h" and related to DMA stream
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
  * @Note   This function is redefined in "main.h" and related to DMA
  *         used for USART data transmission
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
    HAL_UART_IRQHandler(gDbgUartPort.hUart);
}

/**
  * @brief  This function handles USB-On-The-Go FS global interrupt requests.
  * @param  None
  * @retval None
  */
void OTG_FS_IRQHandler(void)
{
  HAL_HCD_IRQHandler(&hhcd);
}

/**
  * @brief  This function handles main I2S interrupt.
  * @param  None
  * @retval None
  */
void I2S3_IRQHandler(void)
{
  HAL_DMA_IRQHandler(hAudioOutI2s.hdmatx);
}

/**
  * @brief  This function handles master I2C event interrupt.
  * @param  None
  * @retval None
  */
void I2C_IF_BUS_EVENT_IRQHandler(void)
{
  I2C_Driver_ISR_Handler();
}

/**
  * @brief  This function handles master I2C error interrupt.
  * @param  None
  * @retval None
  */
void I2C_IF_BUS_ERROR_IRQHandler(void)
{
  I2C_Driver_ERR_ISR_Handler();
}

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/


/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

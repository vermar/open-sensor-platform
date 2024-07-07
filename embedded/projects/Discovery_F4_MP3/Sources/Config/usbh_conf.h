/**
  ******************************************************************************
  * @file    FatFs/FatFs_USBDisk/Inc/usbh_conf.h
  * @author  MCD Application Team
  * @brief   General low level driver configuration
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USBH_CONF__H__
#define __USBH_CONF__H__

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Exported types ------------------------------------------------------------*/
#define USBH_MAX_NUM_ENDPOINTS                2
#define USBH_MAX_NUM_INTERFACES               2
#define USBH_MAX_NUM_CONFIGURATION            1
#define USBH_MAX_NUM_SUPPORTED_CLASS          1
#define USBH_KEEP_CFG_DESCRIPTOR              0
#define USBH_MAX_SIZE_CONFIGURATION           0x200
#define USBH_MAX_DATA_BUFFER                  0x200
#define USBH_DEBUG_LEVEL                      2
#define USBH_USE_OS                           1

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
/* CMSIS OS macros */
#if (USBH_USE_OS == 1)
# ifndef __CMSIS_RTOS
#  include "common.h"
# else
#  include "common.h"
# define   USBH_PROCESS_PRIO          osPriorityNormal
# define   USBH_PROCESS_STACK_SIZE    (8 * 128)
# endif
#endif

/* Memory management macros */
#define USBH_malloc               malloc
#define USBH_free(x)              free(x)
#define USBH_memset               memset
#define USBH_memcpy               memcpy

 /* DEBUG macros */
#if (USBH_DEBUG_LEVEL > 0)
#define  USBH_UsrLog(...)   D1_printf(__VA_ARGS__);\
                            D1_printf("\r\n");
#else
#define USBH_UsrLog(...)
#endif


#if (USBH_DEBUG_LEVEL > 1)

#define  USBH_ErrLog(...)   D1_printf("ERROR: ") ;\
                            D1_printf(__VA_ARGS__);\
                            D1_printf("\r\n");
#else
#define USBH_ErrLog(...)
#endif

#if (USBH_DEBUG_LEVEL > 2)
#define  USBH_DbgLog(...)   D1_printf("DEBUG : ") ;\
                            D1_printf(__VA_ARGS__);\
                            D1_printf("\r\n");
#else
#define USBH_DbgLog(...)
#endif


#if (USBH_MAX_NUM_CONFIGURATION > 1)
#error This USB Host Library version Supports only 1 configuration!
#endif

/* Exported functions ------------------------------------------------------- */
void ASFMessagePut( TaskId tid, uint32_t info, uint32_t timeout );

#endif /* __USBH_CONF__H__ */

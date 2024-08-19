/*==================================================================================================
** OSP Hello World Project: https://github.com/vermar/open-sensor-platform
** Copyright (C) 2024 Rajiv Verma
**
** Licensed under the Apache License, Version 2.0 (the "License"). You may not use this file except
** in compliance with the License. You may obtain a copy of the License at:
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**==================================================================================================
**  @file I2C_CmnDriver.h
**  @brief API definitions for I2C interface driver for STM32 MCUs compatible with ASF/CMSIS-RTOS
**      application
**
*/
#if !defined (I2C_CMNDRIVER_H)
#define   I2C_CMNDRIVER_H

//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include <stdint.h>

//==================================================================================================
//    C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define I2C_ERR_OK                              0
#define I2C_ERR_BUSY                            1
#define I2C_ERR_REQ                             2
#define I2C_ERR_FAIL                            3
#define I2C_ERR_TIMEOUT                         4
#define I2C_ERR_ADDR_NAK                        5

//==================================================================================================
//    T Y P E   D E F I N I T I O N S
//==================================================================================================
typedef enum _SendModeTag {
    I2C_MASTER_SIMPLE_WRITE,    // Simple write transfer without Re-Start
    I2C_MASTER_SIMPLE_READ,     // Simple read transfer without Re-Start
    I2C_MASTER_REG_WRITE,       // Write to specified register of the device
    I2C_MASTER_REG_READ,        // Read from specified register of the device (requires re-start)
    I2C_SLAVE_TX,               // Slave mode transmit
    I2C_SLAVE_RX,               // Slave mode receive
    NUM_I2C_MODES
} I2C_SendMode_t;

typedef void* I2CDriverHandle_t;

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================
I2CDriverHandle_t I2C_Master_Initialise(I2C_TypeDef* busId, uint32_t busSpeed, TaskId taskId);
uint8_t I2C_Driver_Busy(I2CDriverHandle_t handle);
uint8_t I2C_Start_Transfer(I2CDriverHandle_t handle, uint8_t slaveAddr, uint16_t regAddr, uint8_t* pData,
    uint16_t dataSize, I2C_SendMode_t sendMode);
uint8_t I2C_Wait_Completion(I2CDriverHandle_t handle);
osp_bool_t I2C_Ping_Device(I2CDriverHandle_t hDev, uint8_t addr);

#endif /* I2C_CMNDRIVER_H */
//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

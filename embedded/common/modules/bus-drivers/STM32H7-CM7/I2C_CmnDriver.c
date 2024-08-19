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
**  @file I2C_CmnDriver.c
**  @brief Implements I2C interface driver for STM32H7xx devices, compatible with ASF/CMSIS-RTOS
**      application. This driver was originally implemented for STM32L4xx & STM32F7xx devices and
**      supports multiple I2C interfaces. The I2C peripheral on F7xx devices and H7xx devices is
**      functionally similar.
**
*/
//==================================================================================================
//    I N C L U D E   F I L E S
//==================================================================================================
#include "common.h"
#include "I2C_CmnDriver.h"
#include "stm32h7xx_ll_i2c.h"

#ifndef __CMSIS_RTOS
#error This driver requires CMSIS-RTOS v1
#endif

//==================================================================================================
//    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
//==================================================================================================
extern AsfTaskHandle asfTaskHandleTable[];

//==================================================================================================
//    P R I V A T E   C O N S T A N T S   &   M A C R O S
//==================================================================================================
#define I2C_WAIT_TIMEOUT_ms             20 //ms
#define I2C_TXRX_STATUS_ACTIVE          0
#define I2C_TXRX_STATUS_PASSED          1
#define I2C_TXRX_STATUS_FAILED          2
#define I2C_TXRX_STATUS_ADDRNAK         4

#define I2C_MAX_NBYTE_SIZE              255

/* Interrupt and Status register I2C_ISR register flags */
#define I2C_STATUS_BIT_TXE              M_Bit(0)  //Transmit Data Reg empty
#define I2C_STATUS_BIT_TXIS             M_Bit(1)  //Transmit interrupt status
#define I2C_STATUS_BIT_RXNE             M_Bit(2)  //Receive Data Reg not empty
#define I2C_STATUS_BIT_ADDR             M_Bit(3)  //Address Matched (Slave Mode)
#define I2C_STATUS_BIT_NACKF            M_Bit(4)  //Nack received flag
#define I2C_STATUS_BIT_STOPF            M_Bit(5)  //Stop condition detected (initiated by self or externally)
#define I2C_STATUS_BIT_TC               M_Bit(6)  //Transfer Complete (Master Mode)
#define I2C_STATUS_BIT_TCR              M_Bit(7)  //Transfer Complete Reload (for RELOAD=1; NBYTES > 255)
/* Error bits */
#define I2C_STATUS_BIT_BERR             M_Bit(8)  //Bus Error (Reset by PE=0 or BERRCF=1)
#define I2C_STATUS_BIT_ARLO             M_Bit(9)  //Arbitration Lost
#define I2C_STATUS_BIT_OVR              M_Bit(10) //Overrun/Under-run (Slave Mode)
#define I2C_STATUS_BIT_BUSY             M_Bit(15) //Bus Busy (Cleared when Stop detected or PE=0)

#define I2C_MASTER_MODE_EVT_FLAGS       (I2C_IT_ERRI | I2C_IT_TCI | I2C_IT_STOPI | I2C_IT_NACKI | \
                                         I2C_IT_RXI | I2C_IT_TXI)
#define I2C_MASTER_MODE_ERR_FLAGS       (I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR)
                                        /* NOTE: AF (Acknowledge Failure) = NACKF */

/* Some useful Macros for setting/clearing control bits & data */
#define M_GenerateSTART(pInst)          LL_I2C_GenerateStartCondition(pInst)
#define M_GenerateSTOP(pInst)           LL_I2C_GenerateStopCondition(pInst)
#define M_SetWriteTransfer(pInst)       LL_I2C_SetTransferRequest(pInst, LL_I2C_REQUEST_WRITE)
#define M_SetReadTransfer(pInst)        LL_I2C_SetTransferRequest(pInst, LL_I2C_REQUEST_READ)
#define M_SetSlaveAddr(pInst, sa)       LL_I2C_SetSlaveAddr(pInst, sa)
#define M_TxData(pInst, byte)           LL_I2C_TransmitData8(pInst, byte)
#define M_RxData(pInst)                 LL_I2C_ReceiveData8(pInst)
#define M_EnAutoEnd(pInst)              LL_I2C_EnableAutoEndMode(pInst)
#define M_DisAutoEnd(pInst)             LL_I2C_DisableAutoEndMode(pInst)

//==================================================================================================
//    P R I V A T E   T Y P E   D E F I N I T I O N S
//==================================================================================================
/* I2C asynchronous transfer descriptor.*/
typedef struct _TransferDescr
{
    /* Device handle */
    I2C_HandleTypeDef devHandle;
    /* I2C Bus identifier */
    I2C_TypeDef*      busId;
    /* Link to last active client handle */
    I2CDriverHandle_t lastActive;
    /* Send Mode */
    I2C_SendMode_t    sendMode;
    /* Slave Addr */
    uint32_t          slaveAddr;
    /* Reg addr */
    uint32_t          slaveReg;
    /* Asynchronous transfer status. */
    uint8_t           txrxStatus;
    /* Pointer to the data buffer.*/
    uint8_t          *pData;
    /* Total number of bytes to transfer.*/
    uint32_t          num;
    /* Index of current receive/transmit buffer.*/
    uint32_t          byte_index;
    /* Task ID of the handler task */
    TaskId            tID;
    /* Flag to mark if the interface was initialized */
    osp_bool_t        initDone;
} XferDescr_t;

/* I2C Client info structure */
typedef struct _I2cClientInfo
{
    I2C_HandleTypeDef* phI2cIf;     /* Pointer to the device handle */
    uint32_t           i2cSpeed;    /* I2C Clock speed */
    uint32_t           i2cTiming;   /* Timing value corresponding to the speed setting */
    XferDescr_t*       pIfDesc;     /* Pointer to interface descriptor */
    TaskId             tID;         /* Task ID of the handler task */
    /* Flag to mark if the client structure has been allocated to a client */
    osp_bool_t         allocated;
} I2cClientInfo_t;

//==================================================================================================
//    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================
static XferDescr_t s_xferDescriptor[NUM_I2C_INTERFACES];
static I2cClientInfo_t s_clientInfo[NUM_I2C_CLIENTS];

//==================================================================================================
//    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
//==================================================================================================

//==================================================================================================
//    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
//==================================================================================================

//==================================================================================================
//    P R I V A T E     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Provides the handle associated with the given I2C bus
**
** @param pI2cBus: I2C interface identifier
**
** @return pointer to the handle associated with the given I2C bus
*/
static I2C_HandleTypeDef* getI2cBusHandle(I2C_TypeDef* pI2cBus)
{
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        /* Find match corresponding to the given bus reference */
        if (s_xferDescriptor[i].busId == pI2cBus)
        {
            return &s_xferDescriptor[i].devHandle;
        }
    }
    return NULL;
}

/***************************************************************************************************
** @brief Initializes/Updates data structures related to client and associated interface descriptor
**
** @param pI2cBus: I2C interface identifier
** @param hClient: handle for the I2C interface client
**
** @return None
*/
static void updateDescriptor(I2C_TypeDef* pI2cBus, I2CDriverHandle_t hClient)
{
    I2cClientInfo_t* pCInfo = (I2cClientInfo_t*)hClient;
    OS_SETUP_CRITICAL();

    OS_ENTER_CRITICAL();
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        /* Initialize bus reference if this is the first time */
        if (s_xferDescriptor[i].busId == 0)
        {
            s_xferDescriptor[i].busId = pI2cBus;
            s_xferDescriptor[i].lastActive = NULL;
            s_xferDescriptor[i].txrxStatus = I2C_TXRX_STATUS_PASSED;
            pCInfo->pIfDesc = &s_xferDescriptor[i];
            s_xferDescriptor[i].tID = pCInfo->tID;
            s_xferDescriptor[i].lastActive = hClient;
            break;
        }

        /* Find match corresponding to the given bus reference */
        if (s_xferDescriptor[i].busId == pI2cBus)
        {
            if (pCInfo->pIfDesc == NULL)
            {
                pCInfo->pIfDesc = &s_xferDescriptor[i];
            }
            s_xferDescriptor[i].lastActive = hClient;
            break;
        }
    }
    OS_LEAVE_CRITICAL();
}

/***************************************************************************************************
** @brief Checks if the given I2C interface was initialized and marks initialized if not so
**
** @param pI2CBus: I2C interface identifier
**
** @return true if the given I2C bus was already initialized; false otherwise
*/
static osp_bool_t querySetInitState(I2C_TypeDef* pI2cBus)
{
    osp_bool_t wasInitialized = true;
    osp_bool_t busFound = false;
    OS_SETUP_CRITICAL();

    OS_ENTER_CRITICAL();
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        if (s_xferDescriptor[i].busId == pI2cBus)
        {
            busFound = true;
            if (!s_xferDescriptor[i].initDone)
            {
                s_xferDescriptor[i].initDone = true;
                wasInitialized = false;
            }
            break;
        }
    }
    OS_LEAVE_CRITICAL();
    ASF_assert(busFound == true);

    return wasInitialized;
}

/***************************************************************************************************
** @brief Helper function that returns handle to a free data structure for I2C client
**
** @param  none
**
** @return Handle to the allocated Client configuration
*/
static I2CDriverHandle_t allocateClient(void)
{
    uint8_t c;
    OS_SETUP_CRITICAL();

    for (c = 0; c < NUM_I2C_CLIENTS; c++)
    {
        OS_ENTER_CRITICAL();
        if (s_clientInfo[c].allocated == false)
        {
            s_clientInfo[c].allocated = true;
            OS_LEAVE_CRITICAL();
            return &s_clientInfo[c];
        }
        OS_LEAVE_CRITICAL();
    }
    return NULL;
}

/***************************************************************************************************
** @brief Interrupt Service Routine (ISR) handler for I2C transaction events (TX/RX/STOP/ACK etc)
**
** @param handle: Handle to the driver instance
**
** @return None
*/
static void eventIsrHandler(I2CDriverHandle_t handle)
{
    I2cClientInfo_t* pCInfo = (I2cClientInfo_t*)handle;
    XferDescr_t* pDesc = pCInfo->pIfDesc;
    __IO uint32_t statusRegister;
    I2C_TypeDef* pInst = pCInfo->phI2cIf->Instance;

    /* Read the status register */
    statusRegister = pInst->ISR;

    /* NOTE: When Address is ACKed - NBYTES=0 & AUTOEND will generate STOP condition automatically
     * with STOPF set which is used to end this transaction
     * When Slave address is NACKed then NACKF is set. NACK reception also causes STOP condition
     * to be auto generated by HW and so STOPF flag is also set along with NACKF */
    if ((statusRegister & I2C_STATUS_BIT_NACKF) == I2C_STATUS_BIT_NACKF)
    {
        /* Indicate that we are done */
        pDesc->txrxStatus = I2C_TXRX_STATUS_ADDRNAK;
        /* Clear NACKF */
        LL_I2C_ClearFlag_NACK(pInst);
        /* NACKF causes STOP condition to be automatically sent */
    }
    /* Handle Transmit: TXIS is set for every Transmit event */
    else if ((statusRegister & I2C_STATUS_BIT_TXIS) == I2C_STATUS_BIT_TXIS)
    {
        if (pDesc->num == 0)
        {
            /* This case was never seen during testing but leaving in place just in case */
            if (!LL_I2C_IsEnabledAutoEndMode(pInst))
            {
                M_GenerateSTOP(pInst);
            }
            return;
        }
        else /* Start sending transmit data */
        {
            /* If no re-start required just continue sending bytes, starting first with the Register
             * Address if provided */
            if ((pDesc->sendMode == I2C_MASTER_REG_READ) || (pDesc->sendMode == I2C_MASTER_REG_WRITE))
            {
                M_TxData(pInst, pDesc->slaveReg);
                if (pDesc->sendMode == I2C_MASTER_REG_WRITE)
                {
                    /* Switch mode so that we can send data bytes next */
                    pDesc->sendMode = I2C_MASTER_SIMPLE_WRITE;
                }
            }
            else if (pDesc->sendMode == I2C_MASTER_SIMPLE_WRITE)
            {
                /* Send next byte and... */
                M_TxData(pInst, pDesc->pData[pDesc->byte_index++]);
            }
            /* ...decrement counter.
             * NOTE: pDesc->num includes Slave-Register Address byte in the count */
            pDesc->num--;
        }

    }
    /* Handle Receive: RXNE is set for every Receive event */
    else if ((statusRegister & I2C_STATUS_BIT_RXNE) == I2C_STATUS_BIT_RXNE)
    {
        /* Read the data register */
        pDesc->pData[pDesc->byte_index++] = M_RxData(pInst);

        /* Because of AUTOEND we don't need anything else done here! */
    }

    /* NBYTES transferred with AUTOEND=0 - TC is set - ReSTART needed */
    if ((statusRegister & I2C_STATUS_BIT_TC) == I2C_STATUS_BIT_TC)
    {
        /* ReStart is only needed for Register Read */
        if (pDesc->sendMode == I2C_MASTER_REG_READ)
        {
            LL_I2C_HandleTransfer(pDesc->devHandle.Instance, pDesc->slaveAddr, LL_I2C_ADDRSLAVE_7BIT, pDesc->num,
                I2C_AUTOEND_MODE, I2C_GENERATE_START_READ);
        }
        else
        {
            /* This is for completeness... unlikely we will get here in normal operation
             * This can happen only if AUTOEND=0 for non I2C_MASTER_REG_READ transfers */
            M_GenerateSTOP(pInst);
        }
    }

    /* NBYTES transferred with AUTOEND=1 - No TC interrupt, Only STOP interrupt */

    /* STOP Generated. We are done! */
    if ((statusRegister & I2C_STATUS_BIT_STOPF) == I2C_STATUS_BIT_STOPF)
    {
        /* Clear the STOPF Flag */
        LL_I2C_ClearFlag_STOP(pInst);

        /* Disable further interrupts */
        __HAL_I2C_DISABLE_IT(&pDesc->devHandle, I2C_MASTER_MODE_EVT_FLAGS);

        /* Indicate that we are done */
        if (pDesc->txrxStatus == I2C_TXRX_STATUS_ADDRNAK)
        {
            osSignalSet(asfTaskHandleTable[pDesc->tID].handle, I2C_TXRX_STATUS_ADDRNAK);
        }
        else
        {
            pDesc->txrxStatus = I2C_TXRX_STATUS_PASSED;
            osSignalSet(asfTaskHandleTable[pDesc->tID].handle, I2C_TXRX_STATUS_PASSED);
        }
    }
}

/***************************************************************************************************
** @brief Interrupt Service Routine (ISR) handler for I2C error events
**
** @param handle: Handle to the driver instance
**
** @return None
*/
static void errorIsrHandler(I2CDriverHandle_t handle)
{
    I2cClientInfo_t* pCInfo = (I2cClientInfo_t*)handle;
    XferDescr_t* pDesc = pCInfo->pIfDesc;
    __IO uint32_t statusRegister;
    I2C_TypeDef* pInst = pCInfo->phI2cIf->Instance;

    /* Read the status register */
    statusRegister = pInst->ISR;
    pDesc->txrxStatus = I2C_TXRX_STATUS_FAILED;

    /* If ARLO = 1 */
    if ((statusRegister & I2C_STATUS_BIT_ARLO) == I2C_STATUS_BIT_ARLO)
    {
        /* Clear the flag */
        LL_I2C_ClearFlag_ARLO(pInst);
        D0_printf(G_RED"Warning! I2C-ARLO\r\n"G_NORM);
    }
    /* If BERR = 1 */
    if ((statusRegister & I2C_STATUS_BIT_BERR) == I2C_STATUS_BIT_BERR)
    {
        /* Clear the flag */
        LL_I2C_ClearFlag_BERR(pInst);
        D0_printf(G_RED"Warning! I2C-BERR\r\n"G_NORM);
    }
    osSignalSet( asfTaskHandleTable[pDesc->tID].handle, pDesc->txrxStatus );
}

//==================================================================================================
//    P U B L I C     F U N C T I O N S
//==================================================================================================

/***************************************************************************************************
** @brief Initialize given I2C bus in Master Mode
**
** Call this function to set up the I2C master to its initial standby state. Note that HW Setup for
** GPIOs and Interrupt priorities must be called before calling this function.
**
**  Input:
** @param busId: I2C interface ID (HW dependent)
** @param clkSpeed: I2C interface clock speed in Hz. Only standard (100KHz) and fast (400KHz) is
**                  supported
** @param taskId: TaskId of the driver task
**
** @return Handle for the driver instance
*/
I2CDriverHandle_t I2C_Master_Initialise( I2C_TypeDef *busId, uint32_t clkSpeed, TaskId taskId )
{
    HAL_StatusTypeDef ret;
    I2CDriverHandle_t hClient;
    I2cClientInfo_t* pCInfo;

    hClient = allocateClient();

    if (hClient != NULL)
    {
        pCInfo = (I2cClientInfo_t*)hClient;
        pCInfo->i2cSpeed = clkSpeed;
        pCInfo->i2cTiming = (clkSpeed == I2C_BUS_CLOCK_100K) ? I2C_IF1_TIMING_100K : I2C_IF1_TIMING_400K;
        pCInfo->tID = taskId;

        updateDescriptor(busId, hClient);
        pCInfo->phI2cIf = getI2cBusHandle(busId);

        /* Initialize bus interface if this is the first time */
        if (!querySetInitState(busId))
        {
            pCInfo->phI2cIf->Instance = busId;
            pCInfo->phI2cIf->Init.Timing = pCInfo->i2cTiming;
            pCInfo->phI2cIf->Init.OwnAddress1 = 0; //Don't care in master mode
            pCInfo->phI2cIf->Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
            pCInfo->phI2cIf->Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
            pCInfo->phI2cIf->Init.OwnAddress2 = 0;
            pCInfo->phI2cIf->Init.OwnAddress2Masks = I2C_OA2_NOMASK;
            pCInfo->phI2cIf->Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
            pCInfo->phI2cIf->Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

            /* Init the I2C */
            ret = HAL_I2C_Init(pCInfo->phI2cIf);
            ASF_assert(ret == HAL_OK);

            /* Enable the Analog I2C Filter */
            ret = HAL_I2CEx_ConfigAnalogFilter(pCInfo->phI2cIf, I2C_ANALOGFILTER_ENABLE);
            ASF_assert(ret == HAL_OK);
            /* Disable the Digital I2C Filter */
            ret = HAL_I2CEx_ConfigDigitalFilter(pCInfo->phI2cIf, 0);
            ASF_assert(ret == HAL_OK);
        }
        else
        {
            /* This implementation does not support same I2C Bus handling from different tasks */
            ASF_assert(pCInfo->tID == pCInfo->pIfDesc->tID);
            /* Ensure that the bus timing is correctly set for this client */
            if (pCInfo->phI2cIf->Init.Timing != pCInfo->i2cTiming)
            {
                LL_I2C_Disable(busId);
                LL_I2C_SetTiming(busId, pCInfo->i2cTiming);
                pCInfo->phI2cIf->Init.Timing = pCInfo->i2cTiming;
                LL_I2C_Enable(busId);
            }
        }
    }
    return hClient;
    /* Note: I2C bus event and error interrupts are enabled when tx is started */
}

/***************************************************************************************************
** @brief Wait for an I2C transaction to finish.
**  This call will block the calling task for a certain timeout
**
** @param handle: Handle to the driver instance
**
** @return OK or Failed error codes
*/
uint8_t I2C_Wait_Completion( I2CDriverHandle_t handle )
{
    osEvent  ret;
    I2cClientInfo_t* pCInfo;

    ret = osSignalWait( EVT_FLAG_ANY_EVENT, I2C_WAIT_TIMEOUT_ms );
    if (ret.status == osEventTimeout)
    {
        pCInfo = (I2cClientInfo_t*)handle;

        /* Disable interrupts & force Stop condition to release the bus */
        __HAL_I2C_DISABLE_IT(pCInfo->phI2cIf, I2C_MASTER_MODE_EVT_FLAGS);
        M_GenerateSTOP(pCInfo->phI2cIf->Instance);

        pCInfo->pIfDesc->txrxStatus = I2C_TXRX_STATUS_FAILED;
        D0_printf(G_RED"WARNING - Timedout on I2C completion\r\n"G_NORM);
        return I2C_ERR_TIMEOUT;
    }
    else if (ret.value.signals & I2C_TXRX_STATUS_ADDRNAK)
    {
        return I2C_ERR_ADDR_NAK;
    }
    else if (ret.value.signals & I2C_TXRX_STATUS_FAILED)
    {
        return I2C_ERR_FAIL;
    }
    return I2C_ERR_OK;
}

/***************************************************************************************************
** @brief Check the state of the I2C interface
**
** @param handle: Handle to the driver instance
**
** @return I2C_ERR_BUSY: I2C interface is busy with previous transaction
** @return I2C_ERR_OK: I2C interface is idle
*/
uint8_t I2C_Driver_Busy( I2CDriverHandle_t handle )
{
    I2cClientInfo_t* pCInfo = (I2cClientInfo_t*)handle;

    if (pCInfo->pIfDesc->txrxStatus == I2C_TXRX_STATUS_ACTIVE)
    {
        return I2C_ERR_BUSY;
    }
    return I2C_ERR_OK;
}

/***************************************************************************************************
** @brief Start I2C transfer (read or write)
**
**  Call this function to send a prepared data. Also include how many bytes that should be sent/read
**  including the address byte. The function will initiate the transfer and return immediately (or
**  return with error if previous transfer pending). User must wait for transfer to complete by
**  calling I2C_Wait_Completion()
**
** @param handle: Handle to the driver instance
** @param slaveAddr: The slave device's 7-bit address value (not left shifted)
** @param regAddr: When the device has register based access this will contain the register address
**                 from where the read or write is done
** @param pData: Buffer that will contain data for writing or will receive data for reading
** @param dataSize: Data size expected in read and sent in write
** @param sendMode: One of the enum values supported by the driver for I2C transaction
**                  (simple/register based etc)
**
** @return I2C_ERR_BUSY: I2C interface is busy with previous transaction
** @return I2C_ERR_OK: I2C interface is idle
** @return I2C_ERR_REQ: Incorrect mode
*/
uint8_t I2C_Start_Transfer( I2CDriverHandle_t handle, uint8_t slaveAddr, uint16_t regAddr, uint8_t *pData,
    uint16_t dataSize, I2C_SendMode_t sendMode )
{
    I2cClientInfo_t* pCInfo = (I2cClientInfo_t*)handle;
    XferDescr_t* pDesc = pCInfo->pIfDesc;
    uint32_t xferMode = I2C_AUTOEND_MODE;
    uint8_t nByteSz;
    uint32_t reqType = I2C_GENERATE_START_WRITE;

    /* Check that no transfer is already pending*/
    if (pDesc->txrxStatus == I2C_TXRX_STATUS_ACTIVE)
    {
        D0_printf("I2C_Start_Transfer: A transfer is already pending\n\r");
        return I2C_ERR_BUSY;
    }

    /* This driver currently does not support more than 255 bytes (inc. register addr) */
    ASF_assert(dataSize < I2C_MAX_NBYTE_SIZE);

    updateDescriptor(pCInfo->phI2cIf->Instance, handle);
    /* Ensure that the bus timing is correctly set for this client */
    if (pCInfo->phI2cIf->Init.Timing != pCInfo->i2cTiming)
    {
        LL_I2C_Disable(pCInfo->phI2cIf->Instance);
        LL_I2C_SetTiming(pCInfo->phI2cIf->Instance, pCInfo->i2cTiming);
        pCInfo->phI2cIf->Init.Timing = pCInfo->i2cTiming;
        LL_I2C_Enable(pCInfo->phI2cIf->Instance);
    }

    if (sendMode <= I2C_MASTER_REG_READ)
    {
        /* Update the transfer descriptor */
        pDesc->slaveAddr  = (slaveAddr << 1);
        pDesc->slaveReg   = regAddr;  //Don't care for SIMPLE_READ/WRITE
        pDesc->txrxStatus = I2C_TXRX_STATUS_ACTIVE;
        pDesc->pData      = pData;
        /* For register read/write add 1 to the total number of transfer bytes */
        pDesc->num        = sendMode > I2C_MASTER_SIMPLE_READ ? dataSize +1 : dataSize;
        pDesc->byte_index = 0;
        pDesc->sendMode   = sendMode;

        /* Set the NBYTES value */
        nByteSz = pDesc->num;

        /* Clear Configuration Register 2 */
        I2C_RESET_CR2(&pDesc->devHandle);

        /* Clear any lingering flags */
        __HAL_I2C_CLEAR_FLAG( &pDesc->devHandle, I2C_FLAG_OVR | I2C_MASTER_MODE_ERR_FLAGS );

        if (sendMode == I2C_MASTER_REG_READ)
        {
            xferMode = 0; //ReStart needed
            nByteSz = 1;
        }

        if (sendMode == I2C_MASTER_SIMPLE_READ)
        {
            reqType = I2C_GENERATE_START_READ;
        }

        LL_I2C_HandleTransfer(pDesc->devHandle.Instance, pDesc->slaveAddr, LL_I2C_ADDRSLAVE_7BIT, nByteSz,
            xferMode, reqType);

        /* Enable interrupts */
        __HAL_I2C_ENABLE_IT(&pDesc->devHandle, I2C_MASTER_MODE_EVT_FLAGS);
    }
    else
    {
        return I2C_ERR_REQ;
    }
    return I2C_ERR_OK;
}

/***************************************************************************************************
** @brief Checks the presence various devices on the I2C bus based on Slave Address acknowledge
**
** @param hDev: Handle to the driver instance
** @param addr: 7-Bit slave address to ping
**
** @return true: Device with given address was found on the bus
** @return false: Device not found
*/
osp_bool_t I2C_Ping_Device(I2CDriverHandle_t hDev, uint8_t addr)
{
    uint8_t res;

    I2C_Start_Transfer(hDev, addr, 0, NULL, 0, I2C_MASTER_SIMPLE_WRITE);
    res = I2C_Wait_Completion(hDev);
    if (res != I2C_ERR_OK)
    {
        if (res != I2C_ERR_ADDR_NAK)
        {
            D0_printf(G_RED_BOLD"WARNING! I2C Ping Failed [%u] for Addr: %02X\r\n"G_NORM, res, addr);
        }
        return false; //Device NOT FOUND
    }
    return true; //Device FOUND
}

/***************************************************************************************************
** @brief This function handles I2C Interface 1 interrupt request.
**
** @param None
**
** @return None
*/
void I2C_IF1_ISR_Handler(void)
{
#if NUM_I2C_INTERFACES >= 1
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        if (s_xferDescriptor[i].devHandle.Instance == I2C_IF1_BUS)
        {
            eventIsrHandler(s_xferDescriptor[i].lastActive);
            break;
        }
    }
#else
    D0_printf("Unhandled I2C IF1 Interrupt!\r\n");
#endif
}

/***************************************************************************************************
** @brief This function handles I2C Interface 1 error interrupt request.
**
** @param None
**
** @return None
*/
void I2C_IF1_ERR_ISR_Handler(void)
{
#if NUM_I2C_INTERFACES >= 1
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        if (s_xferDescriptor[i].devHandle.Instance == I2C_IF1_BUS)
        {
            errorIsrHandler(s_xferDescriptor[i].lastActive);
            break;
        }
    }
#else
    D0_printf("Unhandled I2C IF1 Error Interrupt!\r\n");
#endif
}

/***************************************************************************************************
** @brief This function handles I2C Interface 2 interrupt request.
**
** @param None
**
** @return None
*/
void I2C_IF2_ISR_Handler(void)
{
#if NUM_I2C_INTERFACES >= 2
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        if (s_xferDescriptor[i].devHandle.Instance == I2C_IF2_BUS)
        {
            eventIsrHandler(s_xferDescriptor[i].lastActive);
            break;
        }
    }
#else
    D0_printf("Unhandled I2C IF2 Interrupt!\r\n");
#endif
}

/***************************************************************************************************
** @brief This function handles I2C Interface 2 error interrupt request.
**
** @param None
**
** @return None
*/
void I2C_IF2_ERR_ISR_Handler(void)
{
#if NUM_I2C_INTERFACES >= 2
    for (uint8_t i = 0; i < NUM_I2C_INTERFACES; i++)
    {
        if (s_xferDescriptor[i].devHandle.Instance == I2C_IF2_BUS)
        {
            errorIsrHandler(s_xferDescriptor[i].lastActive);
            break;
        }
    }
#else
    D0_printf("Unhandled I2C IF2 Error Interrupt!\r\n");
#endif
}

//==================================================================================================
//    E N D   O F   F I L E
//==================================================================================================

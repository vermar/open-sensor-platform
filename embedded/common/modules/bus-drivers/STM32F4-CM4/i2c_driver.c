/* OSP Hello World Project
 * https://github.com/vermar/open-sensor-platform
 *
 * Copyright (C) 2024 Rajiv Verma
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
#include "i2c_driver.h"

#ifdef I2C_DRIVER //defined in Main.h/Common.h
# ifndef I2C_DRIVER_TASK
#  error The application task that handles I2C interface must be designated as I2C_DRIVER_TASK in Main.h
# endif

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern AsfTaskHandle asfTaskHandleTable[];


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define I2C_WAIT_TIMEOUT                      120 //ms
#define I2C_TXRX_STATUS_ACTIVE                0
#define I2C_TXRX_STATUS_PASSED                1
#define I2C_TXRX_STATUS_FAILED                2
#define I2C_TXRX_STATUS_ADDRNAK               4

#define I2C_MASTER_RESTART                    ((I2C_SendMode_t)(NUM_I2C_MODES+1)) //For internal use

/* I2C START mask */
#define CR1_START_Set                         ((uint16_t)0x0100)
#define CR1_START_Reset                       ((uint16_t)0xFEFF)

/* I2C STOP mask */
#define CR1_STOP_Set                          ((uint16_t)0x0200)
#define CR1_STOP_Reset                        ((uint16_t)0xFDFF)

/* I2C ACK mask */
#define CR1_ACK_Set                           ((uint16_t)0x0400)
#define CR1_ACK_Reset                         ((uint16_t)0xFBFF)


/* SR1 register flags */
#define I2C_STATUS_BIT_OVR                    (0x0800)
#define I2C_STATUS_BIT_AF                     (0x0400)
#define I2C_STATUS_BIT_ARLO                   (0x0200)
#define I2C_STATUS_BIT_BERR                   (0x0100)
#define I2C_STATUS_BIT_TXE                    (0x0080)
#define I2C_STATUS_BIT_RXNE                   (0x0040)
#define I2C_STATUS_BIT_BTF                    (0x0004)
#define I2C_STATUS_BIT_ADDR                   (0x0002)
#define I2C_STATUS_BIT_SB                     (0x0001)

/* Masks */
#define I2C_MASK_SB                           (0x0001)
#define I2C_MASK_ADDR                         (0x0002)
#define I2C_MASK_TXE_BTF                      (0x0084)
#define I2C_MASK_RXNE                         (0x0040)
#define I2C_MASK_MSL                          (0x0001)
#define I2C_MASK_BERR                         (0x0100)
#define I2C_MASK_ARLO                         (0x0200)
#define I2C_MASK_AF                           (0x0400)
#define I2C_MASK_OVR                          (0x0800)

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/** \brief I2C asynchronous transfer descriptor.*/
typedef struct _AsyncDesc {
    /* Slave Addr */
    uint32_t i2c_slave_addr;
    /* Reg addr */
    uint32_t i2c_slave_reg;
    /** Asynchronous transfer status. */
    uint8_t i2c_txrx_status;
    /** Asynchronous transfer phase. */
    uint8_t i2c_txrx_phase;
    /** Pointer to the data buffer.*/
    uint8_t *pData;
    /** Total number of bytes to transfer.*/
    uint32_t num;
    /** Index of current receive/transmit buffer.*/
    uint32_t byte_index;

} AsyncDesc_t;

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
static volatile AsyncDesc_t _AsyncXfer;
/* Mode of transfer */
static I2C_SendMode_t _SendMode;
static I2C_HandleTypeDef _I2cHandle;


/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      I2C_HardwareSetup
 *          Configures the GPIOs and h/w interface for the I2C bus
 *
 ***************************************************************************************************/
static void I2C_HardwareSetup( void )
{
    GPIO_InitTypeDef  GPIO_InitStructure;

    /* Enable Clocks GPIOs used */
    I2C_SDA_GPIO_CLK_ENABLE();
    I2C_SCL_GPIO_CLK_ENABLE();

    /* GPIO Configuration for CLK and SDA signals */
    GPIO_InitStructure.Pin = I2C_IF_BUS_SCL_GPIO_PIN;
    GPIO_InitStructure.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStructure.Pull = GPIO_NOPULL;
    GPIO_InitStructure.Speed = GPIO_SPEED_FAST;
    GPIO_InitStructure.Alternate = I2C_IF_SCL_SDA_AF;
    HAL_GPIO_Init(I2C_IF_BUS_SCL_GPIO_GRP, &GPIO_InitStructure);

    GPIO_InitStructure.Pin = I2C_IF_BUS_SDA_GPIO_PIN;
    HAL_GPIO_Init(I2C_IF_BUS_SDA_GPIO_GRP, &GPIO_InitStructure);

    /* Enable I2C Peripheral clock */
    I2C_IF_CLK_ENABLE();

    /* Force the I2C peripheral clock reset */
    I2C_IF_FORCE_RESET();

    /* Release the I2C peripheral clock reset */
    I2C_IF_RELEASE_RESET();

    /* NVIC/Interrupt config */
    /* Enable and set I2Cx Event Interrupt to the highest priority */
    HAL_NVIC_SetPriority(I2C_IF_BUS_EVENT_IRQ_CH, I2C_IF_BUS_INT_PREEMPT_PRIORITY, I2C_IF_BUS_EVENT_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(I2C_IF_BUS_EVENT_IRQ_CH);

    /* Enable and set I2Cx Error Interrupt to the highest priority */
    HAL_NVIC_SetPriority(I2C_IF_BUS_ERROR_IRQ_CH, I2C_IF_BUS_INT_PREEMPT_PRIORITY, I2C_IF_BUS_ERROR_INT_SUB_PRIORITY);
    HAL_NVIC_EnableIRQ(I2C_IF_BUS_ERROR_IRQ_CH);
}


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      I2C_Master_Initialise
 *          Call this function to set up the I2C master to its initial standby state.
 *          Remember to enable interrupts from the main application after initializing the I2C.
 *
 * @param   busId - I2C bus identifier in case multiple buses are supported
 *
 * @return  none
 *
 ***************************************************************************************************/
void I2C_Master_Initialise( I2C_TypeDef *busId )
{
    if(HAL_I2C_GetState(&_I2cHandle) == HAL_I2C_STATE_RESET)
    {
        /* DISCOVERY_I2Cx peripheral configuration */
        _I2cHandle.Init.ClockSpeed = I2C_IF_BUS_CLOCK;
        _I2cHandle.Init.DutyCycle = I2C_DUTYCYCLE_2;
        _I2cHandle.Init.OwnAddress1 = 0x33;
        _I2cHandle.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
        _I2cHandle.Instance = busId;

        /* Init the I2C */
        I2C_HardwareSetup();
        HAL_I2C_Init(&_I2cHandle);
    }

    _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_PASSED;     //initialize last comm status
    /* Note: I2C bus event and error interrupts are enabled when tx is started */
}


/****************************************************************************************************
 * @fn      I2C_Wait_Completion
 *          This function allows application to pend on completion
 *
 * @param   none
 *
 * @return  interrupt enabled status
 *
 ***************************************************************************************************/
uint8_t I2C_Wait_Completion( void )
{
#ifdef __CMSIS_RTOS
    osEvent  ret;

    ret = osSignalWait( EVT_FLAG_ANY_EVENT, I2C_WAIT_TIMEOUT );
    if (ret.status == osEventTimeout)
    {
        D0_printf("### WARNING - Timedout on I2C completion ###\r\n");
        return I2C_ERR_TIMEOUT;
    }
    else if (ret.value.signals & I2C_TXRX_STATUS_FAILED)
    {
        return I2C_ERR_FAIL;
    }
    return I2C_ERR_OK;
#else
    OS_RESULT result;

    result = os_evt_wait_or( I2C_TXRX_STATUS_FAILED | I2C_TXRX_STATUS_PASSED, MSEC_TO_TICS(I2C_WAIT_TIMEOUT));
    if (result == OS_R_TMO)
    {
        D0_printf("### WARNING - Timedout on I2C completion ###\r\n");
        return I2C_ERR_TIMEOUT;
    }
    return I2C_ERR_OK; //TODO Error return
#endif
}


/****************************************************************************************************
 * @fn      I2C_Transceiver_Busy
 *          Call this function to test if the I2C_ISR is busy transmitting.
 *
 * @param   none
 *
 * @return  interrupt enabled or busy status
 *
 ***************************************************************************************************/
uint8_t I2C_Transceiver_Busy( void )
{
    if (_AsyncXfer.i2c_txrx_status == I2C_TXRX_STATUS_ACTIVE)
    {
        return I2C_ERR_BUSY;
    }
    return I2C_ERR_OK;
}


/***************************************************************************************************
** @brief Start I2C transfer (read or write)
**
** Call this function to send a prepared data. Also include how many bytes that should be sent/read
** including the address byte. The function will initiate the transfer and return immediately (or
** return with error if previous transfer pending). User must wait for transfer to complete by
** calling I2C_Wait_Completion()
**
**  Input:
** @param pPktBuff:     Pointer to received packet data
** @param bufLen:       Length of the received data packet.
** @param slaveAddr:    The slave device's 7-bit address value (not left shifted)
** @param regAddr:      When the device has register based access this will contain the register address
**                      from where the read or write is done
** @param pData:        Buffer that will contain data for writing or will receive data for reading
** @param dataSize:     Data size expected in read and sent in write
** @param sendMode:     One of the enum values supported by the driver for I2C transaction (simple/register based etc)
**
** @return I2C_ERR_BUSY:    I2C interface is busy with previous transaction
** @return I2C_ERR_OK:      I2C interface is idle
*/
uint8_t I2C_Start_Transfer( uint8_t slaveAddr, uint16_t regAddr, uint8_t *pData, uint16_t dataSize, I2C_SendMode_t sendMode )
{
    /* Check that no transfer is already pending*/
    if (_AsyncXfer.i2c_txrx_status == I2C_TXRX_STATUS_ACTIVE)
    {

        D0_printf("I2C_Start_Transfer: A transfer is already pending\n\r");
        return I2C_ERR_BUSY;
    }

    if ((sendMode >= I2C_MASTER_SIMPLE_WRITE) && (sendMode <= I2C_MASTER_REG_READ))
    {
        /* Update the transfer descriptor */
        _AsyncXfer.i2c_slave_addr  = (slaveAddr << 1);
        _AsyncXfer.i2c_slave_reg   = regAddr;  //Don't care for SIMPLE_READ/WRITE
        _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_ACTIVE;
        _AsyncXfer.pData           = pData;
        _AsyncXfer.num             = dataSize;
        _AsyncXfer.byte_index      = 0;
        _AsyncXfer.i2c_txrx_phase  = 0;
        _SendMode = sendMode;
        /* Enable interrupts and clear flags */
        __HAL_I2C_ENABLE_IT( &_I2cHandle, I2C_IT_EVT | I2C_IT_BUF | I2C_IT_ERR );
        __HAL_I2C_CLEAR_FLAG( &_I2cHandle, I2C_FLAG_OVR | I2C_FLAG_AF | I2C_FLAG_ARLO | I2C_FLAG_BERR );

        /* Enable ACK as it is disabled in interrupt handler after each transaction */
        _I2cHandle.Instance->CR1 |= CR1_ACK_Set;

        /* Generate Start */
        _I2cHandle.Instance->CR1 |= I2C_CR1_START;
    }
    else
    {
        return I2C_ERR_REQ;
    }
    return I2C_ERR_OK;
}


/****************************************************************************************************
 * @fn      I2C_Get_Data_From_Transceiver
 *          Call this function to read out the requested data from the I2C transceiver buffer.
 *          I.e. first call I2C_Start_Transceiver to send a request for data to the slave. Then Run
 *          this function to collect the data when they have arrived. Include a pointer to where to
 *          place the data and the number of bytes requested (including the address field) in the
 *          function call. The function will hold execution (loop) until the I2C_ISR has completed
 *          with the previous operation, before reading out the data and returning. If there was an
 *          error in the previous transmission the function will return the I2C error code.
 *
 * @param   msg Data buffer for receive
 * @param   msgSize Size of data buffer
 *
 * @return  error status
 *
 ***************************************************************************************************/
uint8_t I2C_Get_Data_From_Transceiver( uint8_t *msg, uint8_t msgSize )
{
    return I2C_ERR_OK;
}


/****************************************************************************************************
 * @fn      I2C_Driver_ISR_Handler
 *          This function is the Interrupt Service Routine (ISR), and called when the I2C interrupt
 *          is triggered; that is whenever a I2C event has occurred.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void I2C_Driver_ISR_Handler(void)
{

    __IO uint32_t SR1Register;
    __IO uint32_t SR2Register = 0;

    /* Read the I2C SR1 and SR2 status registers */
    SR1Register = _I2cHandle.Instance->SR1;
    SR2Register = _I2cHandle.Instance->SR2;

    /* If SB = 1, I2C master sent a START on the bus (EV5) or ReSTART in case of receive */
    if ((SR1Register & I2C_MASK_SB) == I2C_STATUS_BIT_SB)
    {
        /* Send the slave address for transmission or for reception (according to the configured value
            in the master write routine) */
        if ((_SendMode == I2C_MASTER_RESTART) || (_SendMode == I2C_MASTER_SIMPLE_READ))
        {
            _I2cHandle.Instance->DR = _AsyncXfer.i2c_slave_addr | 0x01; //Set read bit
            //After this we just wait for RXNE interrupts to read data
        }
        else //Its a Write transaction (simple or register)
        {
            _I2cHandle.Instance->DR = _AsyncXfer.i2c_slave_addr;
        }
        return;
    }

    /* If ADDR = 1, EV6 */
    if ((SR1Register & I2C_MASK_ADDR) == I2C_STATUS_BIT_ADDR)
    {
        if ((_SendMode == I2C_MASTER_REG_WRITE) || (_SendMode == I2C_MASTER_REG_READ))
        {
            /* Write the device register address */
            _I2cHandle.Instance->DR = _AsyncXfer.i2c_slave_reg;

            /* If this is receive mode then program start bit here so that repeat start will be generated as soon as
            ACK is received */
            if (_SendMode == I2C_MASTER_REG_READ)
            {
                _SendMode = I2C_MASTER_RESTART;
                _I2cHandle.Instance->CR1 |= CR1_START_Set;
            }
        }
        /* Only for Simple READ transaction we need to clear the ACK from master on last byte */
        if (((_SendMode == I2C_MASTER_RESTART) || (_SendMode == I2C_MASTER_SIMPLE_READ)) && (_AsyncXfer.num == 1))
        {
            /* Clear ACK */
            _I2cHandle.Instance->CR1 &= CR1_ACK_Reset;
            /* Program the STOP */
            _I2cHandle.Instance->CR1 |= CR1_STOP_Set;
        }

        if ((_SendMode == I2C_MASTER_SIMPLE_WRITE) && (_AsyncXfer.num > 0))
        {
            /* Write the data in DR register */
            _I2cHandle.Instance->DR = _AsyncXfer.pData[_AsyncXfer.byte_index++];
            /* Decrement the number of data to be written */
            _AsyncXfer.num--;

            /* If no further data to be sent, disable the I2C BUF IT
            in order to not have a TxE  interrupt */
            if (_AsyncXfer.num == 0)
            {
                __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_BUF);
            }
        }
        return;
    }

    /* Master transmits the remaining data: from data2 until the last one.  */
    /* If TXE is set (EV_8) */
    if ((SR1Register & I2C_MASK_TXE_BTF) == I2C_STATUS_BIT_TXE)
    {
        if ((_SendMode == I2C_MASTER_REG_WRITE) || (_SendMode == I2C_MASTER_SIMPLE_WRITE))
        {
            /* If there is still data to write */
            if (_AsyncXfer.num != 0)
            {
                /* Write the data in DR register */
                _I2cHandle.Instance->DR = _AsyncXfer.pData[_AsyncXfer.byte_index++];
                /* Decrement the number of data to be written */
                _AsyncXfer.num--;
                /* If  no data remains to write, disable the BUF IT in order
                to not have again a TxE interrupt. */
                if (_AsyncXfer.num == 0)
                {
                    /* Disable the BUF IT */
                    __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_BUF);
                }
            }

        }

        return;
    }

    /* If BTF and TXE are set (EV8_2), program the STOP */
    if ((SR1Register & I2C_MASK_TXE_BTF) == (I2C_STATUS_BIT_TXE | I2C_STATUS_BIT_BTF))
    {
        if ((_SendMode == I2C_MASTER_REG_WRITE) || (_SendMode == I2C_MASTER_SIMPLE_WRITE))
        {
            /* Program the STOP */
            _I2cHandle.Instance->CR1 |= CR1_STOP_Set;
            /* Disable EVT IT In order to not have again a BTF IT */
            __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_EVT);
            _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_PASSED;
#ifdef __CMSIS_RTOS
            osSignalSet( asfTaskHandleTable[I2C_DRIVER_TASK].handle, I2C_TXRX_STATUS_PASSED );
#else
            isr_evt_set(I2C_TXRX_STATUS_PASSED, asfTaskHandleTable[I2C_DRIVER_TASK].handle );
#endif
        }
        return;
    }
    /* If RXNE is set */
    if ((SR1Register & I2C_MASK_RXNE) == I2C_STATUS_BIT_RXNE)
    {
        if (_AsyncXfer.num == 0) //received all expected data
        {
            /* Disable the BUF IT */
            __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_BUF);
            /* Indicate that we are done receiving */
            _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_PASSED;
#ifdef __CMSIS_RTOS
            osSignalSet( asfTaskHandleTable[I2C_DRIVER_TASK].handle, I2C_TXRX_STATUS_PASSED );
#else
            isr_evt_set(I2C_TXRX_STATUS_PASSED, asfTaskHandleTable[I2C_DRIVER_TASK].handle );
#endif
        }
        else
        {
            /* Read the data register */
            _AsyncXfer.pData[_AsyncXfer.byte_index++] = _I2cHandle.Instance->DR;
            /* Decrement the number of bytes to be read */
            _AsyncXfer.num--;

            /* If it remains only one byte to read, disable ACK and program the STOP (EV7_1) */
            if (_AsyncXfer.num == 1)
            {
                /* Clear ACK */
                _I2cHandle.Instance->CR1 &= CR1_ACK_Reset;
                /* Program the STOP */
                _I2cHandle.Instance->CR1 |= CR1_STOP_Set;
            }

            if (_AsyncXfer.num == 0) //received all expected data
            {
                /* Disable the BUF IT */
                __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_BUF);
                /* Indicate that we are done receiving */
                _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_PASSED;
#ifdef __CMSIS_RTOS
                osSignalSet( asfTaskHandleTable[I2C_DRIVER_TASK].handle, I2C_TXRX_STATUS_PASSED );
#else
                isr_evt_set(I2C_TXRX_STATUS_PASSED, asfTaskHandleTable[I2C_DRIVER_TASK].handle );
#endif
            }
        }

        return;
    }
}


/****************************************************************************************************
 * @fn      I2C_Driver_ERR_ISR_Handler
 *          This function is the I2C error interrupt handler.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void I2C_Driver_ERR_ISR_Handler(void)
{
    __IO uint32_t SR2Register = 0;
    __IO uint32_t SR1Register = 0;
    uint32_t signal = I2C_TXRX_STATUS_FAILED;
    _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_FAILED;

    /* Read the I2C1 status register */
    SR1Register = _I2cHandle.Instance->SR1;
    SR2Register = _I2cHandle.Instance->SR2;

    /* If AF = 1 */
    if ((SR1Register & I2C_MASK_AF) == I2C_STATUS_BIT_AF)
    {
        _I2cHandle.Instance->SR1 &= (~I2C_STATUS_BIT_AF);
        /* Master must generate stop condition on the bus */
        _I2cHandle.Instance->CR1 |= CR1_STOP_Set;
        /* Disable buffer interrupt as well */
        __HAL_I2C_DISABLE_IT(&_I2cHandle, I2C_IT_BUF);
        SR1Register = 0;
        signal |= I2C_TXRX_STATUS_ADDRNAK;
        _AsyncXfer.i2c_txrx_status = I2C_TXRX_STATUS_ADDRNAK;
    }
    /* If ARLO = 1 */
    if ((SR1Register & I2C_MASK_ARLO) == I2C_STATUS_BIT_ARLO)
    {
        _I2cHandle.Instance->SR1 &= (~I2C_STATUS_BIT_ARLO);
    }
    /* If BERR = 1 */
    if ((SR1Register & I2C_MASK_BERR) == I2C_STATUS_BIT_BERR)
    {
        _I2cHandle.Instance->SR1 &= (~I2C_STATUS_BIT_BERR);
    }

    /* If OVR = 1 */
    if ((SR1Register & I2C_MASK_OVR) == I2C_STATUS_BIT_OVR)
    {
        _I2cHandle.Instance->SR1 &= (~I2C_STATUS_BIT_OVR);
    }
#ifdef __CMSIS_RTOS
    osSignalSet( asfTaskHandleTable[I2C_DRIVER_TASK].handle, signal );
#else
    isr_evt_set(signal, asfTaskHandleTable[I2C_DRIVER_TASK].handle );
#endif
}


#endif //I2C_DRIVER
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

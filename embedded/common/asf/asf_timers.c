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
#include "common.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define OS_TIMER_CB_SIZE                (sizeof(uint32_t)*6) //This should match the size set aside in osTimerDef() macro
#define TICS_TO_MSEC(t)                 ((t) * MSEC_PER_TICK)
#define M_GetTCB_Start(p)               (void*)((uint8_t*)p + offsetof(osTimerDef_t, timer) + sizeof(void*))

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Control block for managing ASF timers */
typedef struct _LocalTimerCb
{
    osTimerDef_t *pOsT;
    TimerId       TId;
    AsfTimer     *asfT;
    osp_bool_t    inUse;
} LocalTimer_t;

/* Define a structure to size up the Timer CB space needed */
typedef struct _LocalTimerCbPool
{
    osTimerDef_t OsT;
    uint8_t      Cb[OS_TIMER_CB_SIZE];
} LocalTimerCbPool_t;

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
osPoolDef( tpool, MAX_OS_TIMERS, LocalTimerCbPool_t );

static osPoolId _SysTimerPoolId;

static LocalTimer_t _AsfTimers[MAX_OS_TIMERS];

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      GetFreeTimerIdx
 *          Gets the first index of a timer that is not marked as used
 *
 ***************************************************************************************************/
static int32_t GetFreeTimerIdx( void )
{
    uint32_t i;

    for (i = 0; i < MAX_OS_TIMERS; i++)
    {
        if (_AsfTimers[i].inUse == false)
        {
            return i;
        }
    }
    return -1;
}


/****************************************************************************************************
 * @fn      SendTimerExpiry
 *          Sends the timer expiry message to the owner of the timer
 *
 * @param   pTimer  Pointer to the timer control block
 *
 * @return  none
 *
 ***************************************************************************************************/
static void SendTimerExpiry ( AsfTimer *pTimer )
{
    MessageBuffer *pSendMsg = NULLP;

    ASF_assert( ASFCreateMessage( MSG_TIMER_EXPIRY, sizeof(MsgTimerExpiry), &pSendMsg ) == ASF_OK );
    pSendMsg->msg.msgTimerExpiry.userValue = pTimer->userValue;
    pSendMsg->msg.msgTimerExpiry.timerId   = pTimer->timerId;
    ASF_assert( ASFSendMessage( pTimer->owner, pSendMsg ) == ASF_OK );
}



/****************************************************************************************************
 * @fn      ASFTimerStart
 *          Creates a new timer in the system with the given attributes.
 *
 * @param   pTimer  Pointer to timer control block containing the attributes of the timer to be
 *                  started.
 *
 * @return  none
 *
 * @see     ASFDeleteTimer()
 ***************************************************************************************************/
static void _TimerStart ( AsfTimer *pTimer, char *_file, int _line )
{
    osStatus err;
    int32_t freeIdx = GetFreeTimerIdx();
    ASF_assert(freeIdx >= 0);
    ASF_assert( pTimer != NULLP );
    ASF_assert( pTimer->sysUse == TIMER_NOT_IN_USE ); //In case we are trying to restart a running timer

    _AsfTimers[freeIdx].asfT = pTimer; //Store reference of the application timer
    pTimer->sysUse = freeIdx;
     _AsfTimers[freeIdx].inUse = true;
    err = osTimerStart( _AsfTimers[freeIdx].TId, TICS_TO_MSEC(pTimer->ticks) );
    //pTimer->timerId = os_tmr_create( pTimer->ticks, info );
    ASF_assert( err == osOK );
}


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      ASFTimerInitialize
 *          Initializes the timer function in the system
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void ASFTimerInitialize( void )
{
    uint32_t i;

    /* We will use a block pool to allocate timer control structures */
    _SysTimerPoolId = osPoolCreate( osPool(tpool) );
    ASF_assert( _SysTimerPoolId != NULL );

    for (i = 0; i < MAX_OS_TIMERS; i++)
    {
        /* Create all the timers that the Application will use. They can be started later */
        _AsfTimers[i].pOsT = osPoolAlloc( _SysTimerPoolId );
        ASF_assert( _AsfTimers[i].pOsT != NULL );
        _AsfTimers[i].pOsT->ptimer = ASFTimerExpiry; //All timers will use common expiry callback
        _AsfTimers[i].pOsT->timer = M_GetTCB_Start(_AsfTimers[i].pOsT);
        _AsfTimers[i].TId = osTimerCreate( _AsfTimers[i].pOsT, osTimerOnce, (void*)i );
        _AsfTimers[i].inUse = false;
    }
}


/****************************************************************************************************
 * @fn      ASFTimerStarted
 *          Checks if the timer has already been started
 *
 * @param   pTimer  Pointer to timer handle.
 *
 * @return  true - Timer already started; false otherwise
 *
 * @see     ASFTimerStart()
 ***************************************************************************************************/
osp_bool_t ASFTimerStarted ( AsfTimer *pTimer )
{
    return (pTimer->sysUse != TIMER_NOT_IN_USE? true : false);
}


/****************************************************************************************************
 * @fn      ASFTimerStart
 *          Creates a timer with given reference and tick value assigned to the owner.
 *
 * @param   owner  Task ID of the task that will receive the expiry message
 * @param   ref  Unique reference number for the timer
 * @param   tick  Tick count in OS ticks
 * @param   pTimer  Pointer to timer type
 *
 * @return  none
 *
 * @see     ASFTimerKill()
***************************************************************************************************/
void _ASFTimerStart( TaskId owner, uint16_t ref, uint16_t tick, AsfTimer *pTimer, char *_file, int _line  )
{
    pTimer->owner = owner;
    pTimer->ticks = tick;
    pTimer->userValue = ref;
    _TimerStart( pTimer, _file, _line );
}


/****************************************************************************************************
 * @fn      ASFTimerExpiry
 *          Handles the timer expiry by sending message to the task that created the timer
 *
 * @param   arg  argument set when timer was started
 *
 * @return  none
 *
 * @see     ASFKillTimer()
 ***************************************************************************************************/
void ASFTimerExpiry ( void const *arg )
{
    OS_SETUP_CRITICAL();
    AsfTimer *pTimer;
    uint32_t idx = (uint32_t)arg;
    pTimer = _AsfTimers[idx].asfT;
    //Look for our magic number to be sure we got the right pointer
    ASF_assert_var( pTimer->sysUse != TIMER_NOT_IN_USE,  pTimer->ticks, pTimer->userValue, pTimer->owner);

    OS_ENTER_CRITICAL();
    pTimer->sysUse = TIMER_NOT_IN_USE; //Timer no longer in use
    _AsfTimers[idx].inUse = false;
    OS_LEAVE_CRITICAL();
    /* Note: osMessagePut uses SVC call so the following is outside of critical section */
    SendTimerExpiry( pTimer );
}


/****************************************************************************************************
 * @fn      ASFKillTimer
 *          Kills the timer that was created earlier
 *
 * @param   pTimer  Pointer to timer control block containing the attributes of the timer to be
 *                  created.
 *
 * @return  none
 *
 * @see     ASFTimerStart()
 ***************************************************************************************************/
void _ASFKillTimer ( AsfTimer *pTimer, char *_file, int _line )
{
    OS_SETUP_CRITICAL();
    TimerId tId;
    osStatus err;
    ASF_assert( pTimer != NULLP );

    tId = _AsfTimers[pTimer->sysUse].TId;
    err = osTimerStop( tId );
    ASF_assert( err == osOK );
    OS_ENTER_CRITICAL();
    _AsfTimers[pTimer->sysUse].inUse = false;
    pTimer->sysUse = TIMER_NOT_IN_USE; //Timer no longer in use
    OS_LEAVE_CRITICAL();
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

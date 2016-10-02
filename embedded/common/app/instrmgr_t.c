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
#ifdef ASF_PROFILING
# include "rt_TypeDef.h" //For P_TCB definition
#endif
#include "common.h"
#include "asf_taskstruct.h"


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern const uint16_t C_gIdleStkSize;
extern uint32_t gStackMem;
extern uint32_t gStackSize;

extern const AsfTaskInitDef C_gAsfTaskInitTable[NUMBER_OF_TASKS];
extern uint32_t gSystemRTCRefTime;
extern struct OS_TCB os_idle_TCB; //RTX internal

extern void InitializeTasks( void );
extern uint8_t GetTaskList( uint8_t **pTaskList );
extern void PlatformInitialize( void );

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#if defined ON_DEMAND_PROFILING && defined ASF_PROFILING
# define i_printf           D0_printf
#else
# define i_printf           D1_printf
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
#ifdef ASF_PROFILING
/****************************************************************************************************
 * @fn      HighWaterMarkSearch
 *          Detects stack usage based on binary search. (stack grows down only)
 *
 ***************************************************************************************************/
static uint32_t HighWaterMarkSearch( uint32_t start, uint32_t end )
{
    uint32_t currCheck =start;
    uint32_t currStart = start;
    uint32_t currEnd = end;
    uint32_t tmp;

    /* make sure 32-bit aligned */
    if (start & (sizeof(uint32_t)-1))
        return (end-start);
    if (end & (sizeof(uint32_t)-1))
        return (end-start);

    while(currStart < currEnd)
    {
        currCheck = currStart+((currEnd-currStart)/2);

        /* bias to start, since reading 4 bytes at a time and need to check against the pattern */
        currCheck = currCheck & ~(sizeof(uint32_t)-1);
        tmp = *((uint32_t*)(currCheck));

        /* Check for "FREE" or "STAK" pattern */
        if ((tmp != 0x45455246) && (tmp != 0x4B415453))
        {
            currEnd = currCheck;
        }
        else
        {
            currStart = currCheck;
            /* only case where we wouldn't have made progress, break */
            if ((currStart + sizeof(uint32_t)) == currEnd)
                break;
        }
    }
    return (end - currCheck - sizeof(uint32_t));
}


/****************************************************************************************************
 * @fn      DoProfiling
 *          Calculates and prints CPU/Stack profiling information
 *
 ***************************************************************************************************/
static void DoProfiling( osp_bool_t withStartEnd )
{
    uint8_t  taskCounter, numTasks;
    uint32_t start, end, highWater;
    uint32_t totalElapsedTime;
    osp_float_t taskLoad;
    P_TCB tskPtr;
    uint8_t *pTaskList;
    TaskId tid;


    numTasks = GetTaskList( &pTaskList );
# ifdef STACK_INFO_ONLY
    /* Stack check */
    i_printf("\r\n\t\t\t*** ");
    for (taskCounter = 0; taskCounter < numTasks; taskCounter++)
    {
        tid = (TaskId)pTaskList[taskCounter];
        start = (uint32_t)asfTaskHandleTable[tid].pStack;
        end = start + asfTaskHandleTable[tid].stkSize;
        highWater = HighWaterMarkSearch( start, end );
        i_printf("%02d ", (highWater * 100)/asfTaskHandleTable[tid].stkSize);
    }
    /* Main stack check */
    start = (uint32_t)&gStackMem;
    end = start + (uint32_t)&gStackSize;
    highWater = HighWaterMarkSearch( start, end );
    i_printf("%02d ***\r\n\n", (highWater * 100)/((uint32_t)&gStackSize));
# else
    i_printf("\r\n------------------------------------------------------\r\n");
    totalElapsedTime = RTC_GetCounter() - gSystemRTCRefTime;
    for (taskCounter = 0; taskCounter < numTasks; taskCounter++)
    {
        /* Stack check */
        tid = (TaskId)pTaskList[taskCounter];
        tskPtr = (P_TCB)asfTaskHandleTable[tid].handle;
        start = (uint32_t)tskPtr->stack;
        end = start + C_gAsfTaskInitTable[tid].tDef.stacksize;
        highWater = HighWaterMarkSearch( start, end );

        /* CPU Usage */
        taskLoad = ((osp_float_t)tskPtr->cumulativeRunTime * 100.0f)/(osp_float_t)totalElapsedTime;
        if (withStartEnd) {
            i_printf("%16s: %08x/%08x %04ld/%04ld\t%d%%\t%.2f%%\t%ld\r\n", C_gAsfTaskInitTable[tid].tskName, start, end, highWater,
                C_gAsfTaskInitTable[tid].tDef.stacksize,
                (highWater * 100)/C_gAsfTaskInitTable[tid].tDef.stacksize, taskLoad, tskPtr->runCount);
        } else {
            i_printf("%16s: %04ld/%04ld\t%d%%\t%.2f%%\t%ld\r\n", C_gAsfTaskInitTable[tid].tskName, highWater,
                C_gAsfTaskInitTable[tid].tDef.stacksize,
                (highWater * 100)/C_gAsfTaskInitTable[tid].tDef.stacksize, taskLoad, tskPtr->runCount);
        }
#  ifdef ON_DEMAND_PROFILING
        /* Reset (only for CPU usage) runtime for next profiling period */
        tskPtr->cumulativeRunTime = 0;
#  endif
    }
    /* For IDLE task */
    /* --- Stack check */
    start = (uint32_t)os_idle_TCB.stack;
    end = start + C_gIdleStkSize;
    highWater = HighWaterMarkSearch( start, end );
    /* --- CPU load */
    taskLoad = ((osp_float_t)os_idle_TCB.cumulativeRunTime * 100.0f)/(osp_float_t)totalElapsedTime;
#  ifdef ON_DEMAND_PROFILING
    /* Reset (only for CPU usage) runtime for next profiling period */
    os_idle_TCB.cumulativeRunTime = 0;
    gSystemRTCRefTime = RTC_GetCounter(); //Reset reference time
#  endif
    if (withStartEnd) {
        i_printf("%16s: %08x/%08x %04ld/%04ld\t%d%%\t%.2f%%\t%ld\r\n", "IDLE TASK", start, end, highWater,
            C_gIdleStkSize, (highWater * 100)/C_gIdleStkSize, taskLoad, os_idle_TCB.runCount);
    } else {
        i_printf("%16s: %04ld/%04ld\t%d%%\t%.2f%%\t%ld\r\n", "IDLE TASK", highWater,
            C_gIdleStkSize, (highWater * 100)/C_gIdleStkSize, taskLoad, os_idle_TCB.runCount);
    }
    /* Main stack check */
    start = (uint32_t)&gStackMem;
    end = start + (uint32_t)&gStackSize;
    highWater = HighWaterMarkSearch( start, end );
    if (withStartEnd) {
        i_printf("%16s: %08x/%08x %04ld/%04ld\t%d%%\t -*-\t -*-\r\n", "System Stack", start, end, highWater,
            (uint32_t)&gStackSize, (highWater * 100)/((uint32_t)&gStackSize));
    } else {
        i_printf("%16s: %04ld/%04ld\t%d%%\t -*-\t -*-\r\n", "System Stack", highWater,
            (uint32_t)&gStackSize, (highWater * 100)/((uint32_t)&gStackSize));
    }
    i_printf("------------------------------------------------------\r\n");
# endif

}
#endif //ASF_PROFILING


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      InstrManagerTask
 *          This task is for instrumentation. At startup initializes
 *          other tasks and OS resources in the system. This task is called from main() in
 *          CMSIS-RTOS framework.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
ASF_TASK void InstrManagerTask( ASF_TASK_ARG )
{
    MessageBuffer *rcvMsg = NULLP;
    osp_bool_t msgHandled;

    /* Initialize platform stuff (system hardware resources and debug interface) */
    PlatformInitialize();

    /* Initialize timers in the system */
    ASFTimerInitialize();

    /* Create message queue for Instrumentation task */
    asfTaskHandleTable[INSTR_MANAGER_TASK_ID].handle = osThreadGetId();
    asfTaskHandleTable[INSTR_MANAGER_TASK_ID].QId = osMessageCreate( &C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].queue,
        asfTaskHandleTable[INSTR_MANAGER_TASK_ID].handle );
    ASF_assert( asfTaskHandleTable[INSTR_MANAGER_TASK_ID].QId != NULL );

    /* Create other tasks & OS resources in the system */
    /* Jack up this task's priority temporarily while other threads are created */
    osThreadSetPriority( asfTaskHandleTable[INSTR_MANAGER_TASK_ID].handle, osPriorityRealtime );
    InitializeTasks();

    /* Return back to the set priority */
    osThreadSetPriority( asfTaskHandleTable[INSTR_MANAGER_TASK_ID].handle,
        C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].tDef.tpriority );

    /* Call user init related to instrumentation (for stuff needing to be done before the while loop) */
    InstrManagerUserInit();

    while(1)
    {
        /* User instrumentation such as LED indications, RTC updates, etc. */
        ASFReceiveMessage( INSTR_MANAGER_TASK_ID, &rcvMsg );

        msgHandled = InstrManagerUserHandler( rcvMsg );

        if (!msgHandled)
        {
            switch (rcvMsg->msgId)
            {
#if defined ON_DEMAND_PROFILING && defined ASF_PROFILING
            case MSG_PROFILING_REQ:
                //DoProfiling(true); /* Enable this if Stack addresses are needed */
                DoProfiling(false);
                break;
#endif

            case MSG_TIMER_EXPIRY:
                break;

            default:
                /* Unhandled messages */
                D1_printf("INSTR:!!!UNHANDLED MESSAGE:%d!!!\r\n", rcvMsg->msgId);
                break;

            }
        }
    }
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

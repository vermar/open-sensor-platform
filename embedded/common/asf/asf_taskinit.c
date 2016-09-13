/* Open Sensor Platform Project
 * https://github.com/sensorplatforms/open-sensor-platform
 *
 * Copyright (C) 2013 Sensor Platforms Inc.
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
#include "asf_taskstruct.h"
#include <stdlib.h>


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
void ASFMessagingInit( void );
extern uint8_t GetTaskList( uint8_t **pTaskList );
extern const uint32_t Heap_Size;

#ifdef ASF_PROFILING
 extern const char C_gStackPattern[8];
#endif


#define STACK_INCREASE                  0


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Declare all the thread function prototypes here */
#define ASF_TASK_DEF_TYPE ASF_TASK_DECLARE
#include "asf_taskdeftype.h"
#include "asf_tasks.h"

/**
 * Declare the task stack
 */
#define ASF_TASK_DEF_TYPE ASF_STACK_SETUP
#include "asf_taskdeftype.h"
#include "asf_tasks.h"


/**
 * Declare the queues associated with each task
 */
#define ASF_TASK_DEF_TYPE ASF_QUEUE_SETUP
#include "asf_taskdeftype.h"
#include "asf_tasks.h"


/**
 * This is the task initialization table which details all the information
 * pulled from ASF_TASK_STATIC/ ASF_TASK_DYNAMIC that is required to create
 * the tasks.
 * NOTE: this array is marked as constant so that it is placed in ROM.
 */
#define ASF_TASK_DEF_TYPE ASF_TASK_SETUP
#include "asf_taskdeftype.h"
const AsfTaskInitDef C_gAsfTaskInitTable[NUMBER_OF_TASKS] =
{
#include "asf_tasks.h"
};

/**
 * This table will hold the Task handle (RTX task type)corresponding to
 * the TaskId for each task. This is initialized during AsfInitialiseTasks()
 */
AsfTaskHandle asfTaskHandleTable[NUMBER_OF_TASKS];

#ifdef ASF_PROFILING
 /* Reference start time */
 uint32_t gSystemRTCRefTime;
#endif

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
/* Define  additional heap size here (NewHeap provides area for all task stacks and any
 * additional space needed for regular malloc must be declared as ADD_HEAP_SIZE below)
 */
#define ADD_HEAP_SIZE                   1024    /* In bytes */

/* IMPORTANT: The total stack needed must include stack sizes of all tasks that can be created in
   a given mode. */
#define ASF_TASK_DEF_TYPE ASF_TOTAL_STACK_NEEDED
#include "asf_taskdeftype.h"
const uint32_t TotalStkNeeded =
(
    128 + ADD_HEAP_SIZE /* System overhead + Additional Heap for application */
#include "asf_tasks.h"
);

/* Heap Area defined here */
uint64_t NewHeap[TotalStkNeeded/8] = {0};


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

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      InitializeTasks
 *          Called from initial task to spawn the rest of the tasks in the system
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void InitializeTasks( void )
{
    uint8_t  taskCounter, numTasks;
    TaskId tid;
    uint8_t *pTaskTable;
    uint32_t *pU64Aligned;
#ifdef ASF_PROFILING
    uint32_t *pStack;
    uint32_t i;
#endif

    /* Create tasks  based on the mode we are in */
    numTasks = GetTaskList( &pTaskTable );
    for (taskCounter = 0; taskCounter < numTasks; taskCounter++)
    {
        tid = (TaskId)pTaskTable[taskCounter];

        if (tid != INSTR_MANAGER_TASK_ID)
        {
            /* Allocate task stack from heap */
            /* NOTE: All mallocs are 8-byte aligned as per ARM stack alignment requirements */
            pU64Aligned = malloc( C_gAsfTaskInitTable[tid].stackSize );
            ASF_assert( pU64Aligned != NULL );
            ASF_assert( ((uint32_t)pU64Aligned & 0x7) == 0 ); //Ensure 64-bit aligned

#ifdef ASF_PROFILING
            pStack = pU64Aligned;
            for ( i = 0; i < C_gAsfTaskInitTable[tid].stackSize/sizeof(C_gStackPattern); i++)
            {
                *pStack++ = *((uint32_t *)C_gStackPattern);
                *pStack++ = *((uint32_t *)(C_gStackPattern+4));
            }
#endif
            asfTaskHandleTable[tid].handle  = os_tsk_create_user( C_gAsfTaskInitTable[tid].entryPoint,
                C_gAsfTaskInitTable[tid].priority, pU64Aligned, C_gAsfTaskInitTable[tid].stackSize);
            ASF_assert( asfTaskHandleTable[tid].handle != 0 );
            asfTaskHandleTable[tid].stkSize = C_gAsfTaskInitTable[tid].stackSize;
            asfTaskHandleTable[tid].pStack = pU64Aligned; /* Keep track of our stack pointer */
        }

        /* Initialize the associated queue */
        if (asfTaskHandleTable[tid].handle != 0)
        {
            os_mbx_init( C_gAsfTaskInitTable[tid].queue, C_gAsfTaskInitTable[tid].queueSize );
        }
    }


    /* Initialize the messaging */
    ASFMessagingInit();


#ifdef ASF_PROFILING
    /* Capture the reference time before any other tasks run */
    gSystemRTCRefTime = RTC_GetCounter();
#endif

    /* Switch the priority to be lowest now */
    os_tsk_prio_self( C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].priority );
}


/****************************************************************************************************
 * @fn      AsfInitialiseTasks
 *          This function creates all the Tasks (via the initialTask) as defined in the
 *          C_gAsfTaskInitTable and starts the RTX ticking. This function should be the last to be
 *          called from main() as it will not return.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
void AsfInitialiseTasks ( void )
{
    uint32_t *pU64Aligned;
#ifdef ASF_PROFILING
    uint32_t *pStack, i;
    extern uint64_t mp_stk[];
    extern const uint32_t mp_stk_size;
#endif

    /* NOTE: All mallocs are 8-byte aligned as per ARM stack alignment requirements */
    pU64Aligned = malloc( C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].stackSize );
    ASF_assert( ((uint32_t)pU64Aligned & 0x7) == 0 ); //Ensure 64-bit aligned

#ifdef ASF_PROFILING
    /* Setup the RTX allocated task stack (inc. IDLE task) for profiling purposes */
    pStack = (uint32_t *)mp_stk;
    for ( i = 0; i < mp_stk_size/sizeof(C_gStackPattern); i++)
    {
        *pStack++ = *((uint32_t *)C_gStackPattern);
        *pStack++ = *((uint32_t *)(C_gStackPattern+4));
    }

    /* Profiling for initial task */
    pStack = pU64Aligned;
    for ( i = 0; i < C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].stackSize/sizeof(C_gStackPattern); i++)
    {
        *pStack++ = *((uint32_t *)C_gStackPattern);
        *pStack++ = *((uint32_t *)(C_gStackPattern+4));
    }
#endif

    asfTaskHandleTable[INSTR_MANAGER_TASK_ID].handle  = 1; //Initial task always gets this OS_ID
    asfTaskHandleTable[INSTR_MANAGER_TASK_ID].stkSize = C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].stackSize;
    asfTaskHandleTable[INSTR_MANAGER_TASK_ID].pStack = pU64Aligned;

    /* Initialize RTX and start initialTask */
    os_sys_init_user( InstrManagerTask, 254, pU64Aligned, C_gAsfTaskInitTable[INSTR_MANAGER_TASK_ID].stackSize );
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

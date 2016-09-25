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
#include "asf_taskstruct.h"
#include <stdlib.h>


/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
void ASFMessagingInit( void );
extern uint8_t GetTaskList( uint8_t **pTaskList );

#define STACK_INCREASE                  0


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/* Declare all the thread function prototypes here */
#define ASF_TASK_DEF_TYPE ASF_TASK_DECLARE
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

    /* Create tasks  based on the mode we are in */
    numTasks = GetTaskList( &pTaskTable );
    for (taskCounter = 0; taskCounter < numTasks; taskCounter++)
    {
        tid = (TaskId)pTaskTable[taskCounter];

        if (tid != INSTR_MANAGER_TASK_ID)
        {
            asfTaskHandleTable[tid].handle  = osThreadCreate( &C_gAsfTaskInitTable[tid].tDef, NULL );
            ASF_assert( asfTaskHandleTable[tid].handle != NULL );

            /* Initialize the associated queue */
            if (asfTaskHandleTable[tid].handle != NULL)
            {
                asfTaskHandleTable[tid].QId = osMessageCreate( &C_gAsfTaskInitTable[tid].queue, asfTaskHandleTable[tid].handle );
                ASF_assert( asfTaskHandleTable[tid].QId != NULL )
            }
        }
    }


    /* Initialize the messaging */
    ASFMessagingInit();


#ifdef ASF_PROFILING
    /* Capture the reference time before any other tasks run */
    gSystemRTCRefTime = RTC_GetCounter();
#endif
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
}


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

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
/* This file maybe included more than once */

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
/**
** ASF_TASK_STATIC ( ThreadId, EntryFunction, Priority, StackSize, QueueSize )
**      This macro declares a static thread that will be created (along with an
**      associated queue) automatically by the ASF startup initialization and
**      will persist throughout the lifetime of the application.
**
** Parameters:
**      ThreadId - User defined C-Style identifier for the thread.
**                 E.g. MY_THREAD_ID. This identifier will be used in the
**                 application to access the thread properties and its
**                 associated queue.
**      EntryFunction - Thread entry function name.
**      Priority - Thread priority in the range of 51 through 254. Higher value
**                 implies higher priority. Values 0 through 50 are reserved and
**                 should not be used.
**      StackSize - Thread stack size in bytes.
**      QueueSize - This denotes the maximum number of messages that are allowed
**                 to be queued for the thread. An optimum value should be
**                 chosen for this parameter to minimize memory wastage.
**
**      Example:   ASF_TASK_STATIC ( COMM_THREAD, CommThreadEntry, 55, 1024, 10 )
**
** Entry Function must be declared in the following manner:
**      ASF_TASK MyThreadEntry ( ASF_TASK_ARG )
**      {
**          ...
**      }
**
*/
/* Declare all ASF tasks here */
/* InstrManagerTask creates other tasks and OS resources and must always be first task declared */
/* Note that InstrManagerTask is same as 'main' task in CMSIS-RTOS implementation */
ASF_TASK_STATIC ( INSTR_MANAGER_TASK_ID,  InstrManagerTask,   osPriorityIdle,  0x400,    4 )

/* Handles command input from UART */
ASF_TASK_STATIC ( CMD_HNDLR_TASK_ID,      CmdHandlerTask,     osPriorityLow,   0x400,    4 )

/* Additional tasks specific to application is defined in App_Tasks.h */
#include "app_tasks.h"


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

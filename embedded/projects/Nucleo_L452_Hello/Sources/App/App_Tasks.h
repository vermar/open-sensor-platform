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
/* This file in only included in ASF_Tasks.h and nowhere else */

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
**      Priority - Thread priority as defined in CMSIS-RTOS API definition (see cmsis_os.h).
**      StackSize - Thread stack size in bytes.
**      QueueSize - This denotes the maximum number of messages that are allowed
**                 to be queued for the thread. An optimum value should be
**                 chosen for this parameter to minimize memory wastage.
**
**      Example:   ASF_TASK_STATIC ( COMM_THREAD, CommThreadEntry, osPriorityNormal, 1024, 10 )
**
** Entry Function must be declared in the following manner:
**      ASF_TASK MyThreadEntry ( ASF_TASK_ARG )
**      {
**          ...
**      }
**
*/
/* Declare additional application specific tasks here */
/* NOTE: STACK_INCREASE can be used to increase the stack size of all tasks by a constant amount.
   This value is set in ASF_TaskInit.c file and is normally 0. Use this for Debugging crashes */
ASF_TASK_STATIC (LED_ON_TASK_ID,        LED_On_Task,        osPriorityNormal,  (0x100+STACK_INCREASE),  4)
ASF_TASK_STATIC (LED_OFF_TASK_ID,       LED_Off_Task,       osPriorityNormal,  (0x100+STACK_INCREASE),  4)

#ifdef INCLUDE_TEST_TASK
ASF_TASK_STATIC (TEST_TASK_ID,          LED_Test_Task,      osPriorityNormal,  (0x400+STACK_INCREASE),  4)
#endif



/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/
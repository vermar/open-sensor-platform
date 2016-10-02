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

/*
** ASF_TASK_DEF_TYPE should be defined to be either
**
**     ASF_TASK_IDS
**     ASF_QUEUE_IDS
**     ASF_STACK_SETUP
**     ASF_TASK_SETUP
**     ASF_TASK_DECLARE
**
** by the including file
*/


/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#if !defined (IN_ASF_TASK_DEF)
# error This file can only be included in ASF_TaskDefType.h
#endif

#if ASF_TASK_DEF_TYPE == ASF_TASK_IDS
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize ) ThreadId,
#endif

#if ASF_TASK_DEF_TYPE == ASF_STACK_SETUP
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize )           \
    typedef struct { uint8_t _b[StackSize]; } ThreadId##_StkSize;
#endif

#if ASF_TASK_DEF_TYPE == ASF_QUEUE_SETUP
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize )           \
    uint32_t Q_##ThreadId[ 4 + QueueSize ] = {0};
#endif

#if ASF_TASK_DEF_TYPE == ASF_TASK_DECLARE
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize )           \
    extern void EntryFunction(void const *argument);
#endif

#if ASF_TASK_DEF_TYPE == ASF_TASK_SETUP
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize )           \
    { {EntryFunction, Priority, 1, StackSize}, ThreadId, #EntryFunction, {QueueSize, Q_##ThreadId}, #ThreadId },
#endif

#if ASF_TASK_DEF_TYPE == ASF_TOTAL_STACK_NEEDED
# define ASF_TASK_STATIC( ThreadId, EntryFunction, Priority, StackSize, QueueSize )           \
    struct { uint8_t _b[StackSize]; } _##ThreadId##_StkSize;
#endif


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

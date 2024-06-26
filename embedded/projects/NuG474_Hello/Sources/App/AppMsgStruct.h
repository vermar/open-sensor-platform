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
#if !defined (APPMSGSTRUCT_H)
#define   APPMSGSTRUCT_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/
#include <stdint.h>

/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define NUM_CMD_ARGS                5
#define COMMAND_LINE_SIZE           32  //Match the define in Main.h

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(4)

typedef struct MsgIntEvtIndTag
{
    uint32_t     timeStamp;
    uint32_t     deviceId;
} MsgIntEvtInd;

/* Console command input structure */
#if defined (__CC_ARM)
# pragma anon_unions
#endif
typedef struct _MsgCliCmd
{
    char        cmd;                  /* Command identifier */
    union _CVal
    {
        int32_t value[NUM_CMD_ARGS];  /* Value for the command */
        char    cmdStr[COMMAND_LINE_SIZE]; /* Command String sans cmd part */
    } u;
} MsgCliCmd_t;

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/

#pragma pack(pop)   /* restore original alignment from stack */

#endif /* APPMSGSTRUCT_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

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
#if !defined (HW_SETUP_H)
#define   HW_SETUP_H

/*-------------------------------------------------------------------------------------------------*\
 |    I N C L U D E   F I L E S
\*-------------------------------------------------------------------------------------------------*/

#ifdef DISCOVERY_L1_BOARD
# include "hw_setup_discovery_l1.h"
#endif

#ifdef STEVAL_MKI109V2
# include "hw_setup_steval_mki109v2.h"
#endif

#ifdef XPRESSO_LPC54102_BOARD
# include "hw_setup_xpresso_lpc54102.h"
#endif

#ifdef DISCOVERY_F4_BOARD
# include "hw_setup_discovery_f4.h"
#endif

#ifdef NUCLEO_L452RE_BOARD
# include "hw_setup_nucleo_l452.h"
#endif


/*-------------------------------------------------------------------------------------------------*\
 |    C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
void SystemGPIOConfig( void );
void SystemInterruptConfig( void );
void DebugPortInit( void );
void DebugUARTConfig( uint32_t baud, uint32_t dataLen, uint32_t stopBits, uint32_t parity );
void RTC_Configuration( void );


#endif /* HW_SETUP_H */
/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

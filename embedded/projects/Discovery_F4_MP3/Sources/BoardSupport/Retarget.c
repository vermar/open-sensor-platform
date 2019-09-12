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
#include <stdio.h>
#include "debugprint.h"

#ifdef __CC_ARM
# pragma import(__use_no_semihosting_swi)
#endif

#if defined (__GNUC__)
/* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
set to 'Yes') calls __io_putchar() */
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#elif defined (__CC_ARM)
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

#if defined (__GNUC__) || defined (__CC_ARM)
int ser_putchar (int c);
int ser_getchar (void);

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;


PUTCHAR_PROTOTYPE
{
    return (ser_putchar(ch));
}

int fgetc (FILE *f)         { return (ser_getchar()); }


int ferror(FILE *f) {
    /* Your implementation of ferror */
    return EOF;
}


void _ttywrch(int ch)       { ser_putchar(ch); }


void _sys_exit(int return_code) {
label:  goto label;  /* endless loop */
}

/*----------------------------------------------------------------------------
 Write character to Serial Port (blocking)
 *----------------------------------------------------------------------------*/
int ser_putchar (int c) {

    while(!DbgUartTransmitBufferEmpty());
    DbgUartSendByte((uint8_t)c);
    return (c);
}
#endif

/*----------------------------------------------------------------------------
 Read character from Serial Port   (blocking read)
 *----------------------------------------------------------------------------*/
int ser_getchar (void) {

    while (!DbgUartReceiveBufferFull());
    return ((int)DbgUartReadByte());
}

#if defined (__ICCARM__)

/* Standard file handles (see arm/inc/c/LowLevelIOinterface.h file under IAR compiler installation) */
#define _LLIO_STDIN  0
#define _LLIO_STDOUT 1
#define _LLIO_STDERR 2

/* Return values */
#define _LLIO_ERROR ((size_t)-1) /* For __read and __write. */

size_t __write(int Handle, const unsigned char * Buf, size_t Bufsize);
size_t __read(int handle, unsigned char * buffer, size_t size);
/*************************************************************************
 * Function Name: __write
 * Parameters: Low Level cahracter output
 *
 * Return:
 *
 * Description:
 *
 *************************************************************************/
size_t __write(int Handle, const unsigned char * Buf, size_t Bufsize)
{
    size_t nChars = 0;

    for (/*Empty */; Bufsize > 0; --Bufsize)
    {
        /* Loop until the end of transmission */
        while(!DbgUartTransmitBufferEmpty());
        DbgUartSendByte((uint8_t)*Buf++);
        ++nChars;
    }
    return nChars;
}
/*************************************************************************
 * Function Name: __read
 * Parameters: Low Level cahracter input
 *
 * Return:
 *
 * Description:
 *
 *************************************************************************/
size_t __read(int handle, unsigned char * buffer, size_t size)
{
    int nChars = 0;

    /* This template only reads from "standard in", for all other file
     * handles it returns failure. */
    if (handle != _LLIO_STDIN)
    {
        return _LLIO_ERROR;
    }

    for (/* Empty */; size > 0; --size)
    {
        int c = ser_getchar();
        if (c < 0)
            break;

        *buffer++ = c;
        ++nChars;
    }

    return nChars;
}

#endif

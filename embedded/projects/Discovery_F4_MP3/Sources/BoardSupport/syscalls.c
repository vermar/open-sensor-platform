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
#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>

#include "debugprint.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
#undef errno
extern int errno;

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
/*
 environ
 A pointer to a list of environment variables and their values.
 For a minimal environment, this empty list is adequate:
 */
char *__env[1] = { 0 };
char **environ = __env;


/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
int _write(int file, char *ptr, int len);

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/** Define following __io_putchar() as weak so that user can override in application code if needed */
#pragma weak __io_putchar

/****************************************************************************************************
 * @fn      __io_putchar
 *          Retargets the C library printf function to the USART
 *
 ***************************************************************************************************/
int __io_putchar(int ch)
{
    /* Place your implementation of fputc here */
    /* e.g. write a character to the USART */
    while (!DbgUartTransmitBufferEmpty());
    DbgUartSendByte((uint8_t)ch);

    return ch;
}

/****************************************************************************************************
 * @fn      _exit
 *          Minimal implementation for embedded systems where exit is generally not allowed
 *
 ***************************************************************************************************/
void _exit(int status)
{
    _write(1, "exit", 4);
    while (1)
    {
        ;
    }
}

/****************************************************************************************************
 * @fn      _close
 *          Minimal implementation for system without file-io support
 *
 ***************************************************************************************************/
int _close(int file)
{
    return -1;
}

/****************************************************************************************************
 * @fn      _execve
 *          Transfer control to a new process. Minimal implementation for a system without processes
 *
 ***************************************************************************************************/
int _execve(char *name, char **argv, char **env)
{
    errno = ENOMEM;
    return -1;
}

/****************************************************************************************************
 * @fn      _fork
 *          Create a new process. Minimal implementation for a system without processes.
 *
 ***************************************************************************************************/
int _fork()
{
    errno = EAGAIN;
    return -1;
}

/****************************************************************************************************
 * @fn      _fstat
 *          Status of an open file. For consistency with other minimal implementations in these examples,
 *          all files are regarded as character special devices.
 *
 ***************************************************************************************************/
int _fstat(int file, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

/****************************************************************************************************
 * @fn      _getpid
 *          Get Process-ID. Minimal implementation, for a system without processes
 *
 ***************************************************************************************************/
int _getpid()
{
    return 1;
}

/****************************************************************************************************
 * @fn      _isatty
 *          Query whether output stream is a terminal.
 *
 ***************************************************************************************************/
int _isatty(int file)
{
    switch (file)
    {
    case STDOUT_FILENO:
    case STDERR_FILENO:
    case STDIN_FILENO:
        return 1;

    default:
        errno = EBADF;
        return 0;
    }
}

/****************************************************************************************************
 * @fn      _kill
 *          Send a kill signal to process. Not supported
 *
 ***************************************************************************************************/
int _kill(int pid, int sig)
{
    errno = EINVAL;
    return (-1);
}

/****************************************************************************************************
 * @fn      _link
 *          Establish a new name for an existing file
 *
 ***************************************************************************************************/
int _link(char *old, char *new)
{
    errno = EMLINK;
    return -1;
}

/****************************************************************************************************
 * @fn      _lseek
 *          Set position in a file. Minimal implementation
 *
 ***************************************************************************************************/
int _lseek(int file, int ptr, int dir)
{
    return 0;
}

/****************************************************************************************************
 * @fn      _sbrk
 *          Increase program data space. Malloc and related functions depend on this
 *
 ***************************************************************************************************/
caddr_t _sbrk(int incr)
{
    extern char _ebss; // Defined by the linker
    static char *heap_end;
    char *prev_heap_end;

    if (heap_end == 0)
    {
        heap_end = &_ebss;
    }
    prev_heap_end = heap_end;

    char * stack = (char*)__get_MSP();
    if (heap_end + incr > stack)
    {
        _write(STDERR_FILENO, "Heap and stack collision\r\n", 26);
        errno = ENOMEM;
        return (caddr_t)-1;
    }

    heap_end += incr;
    return (caddr_t)prev_heap_end;

}

/*
 read
 .
 */
/****************************************************************************************************
 * @fn      _read
 *          Read a character to a file. `libc' subroutines will use this system routine for input
 *          from all files, including stdin. Returns -1 on error or blocks until the number of
 *          characters have been read
 *
 ***************************************************************************************************/
int _read(int file, char *ptr, int len)
{
    int n;
    int num = 0;
    switch (file)
    {
    case STDIN_FILENO:
        for (n = 0; n < len; n++)
        {
            while (!DbgUartReceiveBufferFull());
            *ptr++ = (char)DbgUartReadByte();
            num++;
        }
        break;

    default:
        errno = EBADF;
        return -1;
    }
    return num;
}

/****************************************************************************************************
 * @fn      _stat
 *          Status of a file (by name). Minimal implementation
 *
 ***************************************************************************************************/
int _stat(const char *filepath, struct stat *st)
{
    st->st_mode = S_IFCHR;
    return 0;
}

/****************************************************************************************************
 * @fn      _times
 *          Timing information for current process. Not supported
 *
 ***************************************************************************************************/
clock_t _times(struct tms *buf)
{
    return -1;
}

/****************************************************************************************************
 * @fn      _unlink
 *          Remove a file's directory entry. Not supported
 *
 ***************************************************************************************************/
int _unlink(char *name)
{
    errno = ENOENT;
    return -1;
}

/****************************************************************************************************
 * @fn      _wait
 *          Wait for a child process. Not supported
 *
 ***************************************************************************************************/
int _wait(int *status)
{
    errno = ECHILD;
    return -1;
}

/****************************************************************************************************
 * @fn      _write
 *          Write a character to a file. `libc' subroutines will use this system routine for output
 *          to all files, including stdout. Returns -1 on error or number of bytes sent
 *
 ***************************************************************************************************/
int _write(int file, char *ptr, int len)
{
    int n;

    switch (file)
    {
    case STDOUT_FILENO:
        for (n = 0; n < len; n++)
        {
            __io_putchar((*ptr++));
        }
        break;

    case STDERR_FILENO:
        for (n = 0; n < len; n++)
        {
            __io_putchar((*ptr++));
        }
        break;

    default:
        errno = EBADF;
        return -1;
    }

    return len;
}

/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

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
#include "Common.h"
#include <string.h>
/* FatFs includes component */
#include "ff_gen_drv.h"
#include "usbh_diskio.h"

#include "mp3dec.h"
#include "stm32f4_discovery_audio.h"

/*-------------------------------------------------------------------------------------------------*\
 |    E X T E R N A L   V A R I A B L E S   &   F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/
extern AsfTaskHandle asfTaskHandleTable[];

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   C O N S T A N T S   &   M A C R O S
\*-------------------------------------------------------------------------------------------------*/
#define f_tell(fp)                  ((fp)->fptr)
#define ID3_EXT_HDR_FLAG            0x40
#define BUTTON                      (GPIOA->IDR & GPIO_PIN_0)

#define MP3_INBUF_SIZE              8192
#define MP3_DMA_BUFFER_SIZE         (4 * MAX_NCHAN * MAX_NGRAN * MAX_NSAMP)

#define DMA_EVT_HALF_TRANSFER       0x0100
#define DMA_EVT_FULL_TRANSFER       0x0200
#define APP_EVT_STOP_REQUEST        0x0400

#define HP_VOL_LEVEL                80      //% of Full Volume

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E   T Y P E   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
typedef enum
{
  DISCONNECTION_EVENT = 1,  
  CONNECTION_EVENT,
} MSC_ApplicationTypeDef;

/*-------------------------------------------------------------------------------------------------*\
 |    S T A T I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
FATFS USBDISKFatFs;           /* File system object for USB disk logical drive */
FIL MyFile;                   /* File object */
char USBDISKPath[4];          /* USB Host logical drive path */

static uint8_t g_Mp3InBuffer[MP3_INBUF_SIZE];
static uint16_t g_pMp3OutBuffer[MAX_NCHAN * MAX_NGRAN * MAX_NSAMP];
static uint16_t* g_pMp3OutBufferPtr = NULL;
static uint16_t g_pMp3DmaBuffer[MP3_DMA_BUFFER_SIZE];
static uint16_t* g_pMp3DmaBufferPtr = NULL;

/*-------------------------------------------------------------------------------------------------*\
 |    F O R W A R D   F U N C T I O N   D E C L A R A T I O N S
\*-------------------------------------------------------------------------------------------------*/
static void SendAppEvent( uint32_t evtId );
static uint32_t Mp3ReadId3V2Tag(FIL* pInFile);
static void Mp3Play( void );

/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C   V A R I A B L E S   D E F I N I T I O N S
\*-------------------------------------------------------------------------------------------------*/
USBH_HandleTypeDef hUSB_Host; /* USB Host handle */

/*-------------------------------------------------------------------------------------------------*\
 |    P R I V A T E     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/****************************************************************************************************
 * @fn      Error_Handler
 *          Spit out error message and LED indication
 *
 ***************************************************************************************************/
static void Error_Handler(void)
{
    /* Turn LED5 on */
    LED_On(LED_RED);
    D1_printf("Stuck in While LOOP!\r\n");
    while(1)
    {
    }
}


/****************************************************************************************************
 * @fn      get_filename_ext
 *          Helper routine for extracting file extension
 *
 ***************************************************************************************************/
static const char *get_filename_ext(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}


/****************************************************************************************************
 * @fn      Mp3FillReadBuffer
 *          Helper routine for ...
 *
 ***************************************************************************************************/
static UINT Mp3FillReadBuffer(BYTE* pInData, UINT unInDataLeft, FIL* pInFile)
{
    FRESULT fr;

    // move last, small chunk from end of buffer to start, then fill with new data
    memmove(g_Mp3InBuffer, pInData, unInDataLeft);

    UINT unSpaceLeft = MP3_INBUF_SIZE - unInDataLeft;
    UINT unRead = 0;

    LED_On(LED_BLUE);
    fr = f_read(pInFile, g_Mp3InBuffer + unInDataLeft, unSpaceLeft, &unRead);
    LED_Off(LED_BLUE);

    if(fr != FR_OK)
    {
        unRead = 0;
        D1_printf("### File read error [%d]\r\n", fr);
    }
    else if (unRead == 0)
    {
        D1_printf("f_read: [%d] Read [%d]\r\n", unSpaceLeft, unRead);
    }

    if(unRead < unSpaceLeft)
    {
        // zero-pad to avoid finding false sync word after last frame (from old data in readBuf)
        memset(g_Mp3InBuffer + unInDataLeft + unRead, unSpaceLeft - unRead, 0);
    }

    return unRead;
}


/****************************************************************************************************
 * @fn      Mp3ReadId3V2Text
 *          Taken from http://www.mikrocontroller.net/topic/252319
 *
 ***************************************************************************************************/
static uint32_t Mp3ReadId3V2Text(FIL* pInFile, uint32_t unDataLen, char* pszBuffer, uint32_t unBufferSize)
{
    UINT unRead = 0;
    BYTE byEncoding = 0;
    if((f_read(pInFile, &byEncoding, 1, &unRead) == FR_OK) && (unRead == 1))
    {
        unDataLen--;
        if(unDataLen <= (unBufferSize - 1))
        {
            if((f_read(pInFile, pszBuffer, unDataLen, &unRead) == FR_OK) && (unRead == unDataLen))
            {
                if(byEncoding == 0)
                {
                    // ISO-8859-1 multibyte
                    // just add a terminating zero
                    pszBuffer[unDataLen] = 0;
                }
                else if(byEncoding == 1)
                {
                    D1_printf("#### CHECK - Unicode!\r\n");
                    // UTF16LE unicode
                    uint32_t r = 0;
                    uint32_t w = 0;
                    if((unDataLen > 2) && (pszBuffer[0] == 0xFF) && (pszBuffer[1] == 0xFE))
                    {
                        // ignore BOM, assume LE
                        r = 2;
                    }
                    for(; r < unDataLen; r += 2, w += 1)
                    {
                        // should be acceptable for 7 bit ascii
                        pszBuffer[w] = pszBuffer[r];
                    }
                    pszBuffer[w] = 0;
                }
            }
            else
            {
                D1_printf("#### CHECK - f_read Err 1!\r\n");
                return 1;
            }
        }
        else
        {
            // we won't read a partial text
            if(f_lseek(pInFile, f_tell(pInFile) + unDataLen) != FR_OK)
            {
                return 1;
            }
            D1_printf("#### CHECK - Skip %d!\r\n", unDataLen);
        }
    }
    else
    {
        D1_printf("#### CHECK - f_read Err 2!\r\n");
        return 1;
    }
    return 0;
}


/****************************************************************************************************
 * @fn      Mp3ReadId3V2Tag
 *          Taken from http://www.mikrocontroller.net/topic/252319
 *
 ***************************************************************************************************/
static uint32_t Mp3ReadId3V2Tag(FIL* pInFile)
{
    char txtBuffer[200];
    const uint32_t txtBufferSz = sizeof(txtBuffer);

    BYTE id3hd[10];
    UINT unRead = 0;
    uint32_t id3TotFrameSz = 0;

    if((f_read(pInFile, id3hd, 10, &unRead) != FR_OK) || (unRead != 10))
    {
        return 1;
    }
    else
    {
        uint32_t unSkip = 0;
        if((unRead == 10) && (id3hd[0] == 'I') && (id3hd[1] == 'D') && (id3hd[2] == '3'))
        {
            unSkip += 10;
            unSkip = ((id3hd[6] & 0x7f) << 21) | ((id3hd[7] & 0x7f) << 14) | ((id3hd[8] & 0x7f) << 7) | (id3hd[9] & 0x7f);
            //D1_printf("#### CHECK!! Skip Total: %lu [%X]\r\n", unSkip, unSkip);

            // try to get some information from the tag
            // skip the extended header, if present
            uint8_t unVersion = id3hd[3];
            if(id3hd[5] & ID3_EXT_HDR_FLAG)
            {
                BYTE exhd[4]; //First 4 bytes of extended header gives size of the header (excluding itself)
                D1_printf("#### CHECK!! Extended Header\r\n");
                f_read(pInFile, exhd, 4, &unRead);
                size_t unExHdrSkip = ((exhd[0] & 0x7f) << 21) | ((exhd[1] & 0x7f) << 14) | ((exhd[2] & 0x7f) << 7) | (exhd[3] & 0x7f);
                unExHdrSkip -= 4;
                if(f_lseek(pInFile, f_tell(pInFile) + unExHdrSkip) != FR_OK)
                {
                    return 1;
                }
            }

            D1_printf("\n--------------------------------------\r\n");
            while(1)
            {
                char frhd[10];
                if((f_read(pInFile, frhd, 10, &unRead) != FR_OK) || (unRead != 10))
                {
                    return 1;
                }
                /* Add to the total frame size read */
                id3TotFrameSz += 10;

                if((frhd[0] == 0) || (strncmp(frhd, "3DI", 3) == 0))
                {
                    //D1_printf("#### CHECK!! break\r\n");
                    break;
                }

                char szFrameId[5] = {0, 0, 0, 0, 0};
                memcpy(szFrameId, frhd, 4);
                uint32_t unFrameSize = 0;
                uint32_t i = 0;
                for(; i < 4; i++)
                {
                    if(unVersion == 3)
                    {
                        // ID3v2.3
                        unFrameSize <<= 8;
                        unFrameSize |= (frhd[i + 4] & 0xFF);
                    }
                    if(unVersion == 4)
                    {
                        // ID3v2.4
                        unFrameSize <<= 7;
                        unFrameSize |= (frhd[i + 4] & 0x7F);
                        //D1_printf("#### CHECK!! - ID3v2.4\r\n");
                    }
                }
                //D1_printf("#### CHECK!! Frame Sz: %lu [%X]\r\n", unFrameSize, unFrameSize);
                /* Add to the total frame size read */
                id3TotFrameSz += unFrameSize;

                /* Sanity check in case ID3 tag is messed up */
                if (id3TotFrameSz >= unSkip)
                {
                    D1_printf("ID3 parsing done. Skiping further parsing\r\n");
                    break; //Exit the while loop
                }

                if(strcmp(szFrameId, "TPE1") == 0)
                {
                    // artist
                    if(Mp3ReadId3V2Text(pInFile, unFrameSize, txtBuffer, txtBufferSz) != 0)
                    {
                        break;
                    }
                    D1_printf("ARTIST : %s\r\n", txtBuffer);
                }
                else if(strcmp(szFrameId, "TIT2") == 0)
                {
                    // title
                    if(Mp3ReadId3V2Text(pInFile, unFrameSize, txtBuffer, txtBufferSz) != 0)
                    {
                        break;
                    }
                    D1_printf("  SONG : %s\r\n", txtBuffer);
                }
                else if(strcmp(szFrameId, "TALB") == 0)
                {
                    // Album
                    if(Mp3ReadId3V2Text(pInFile, unFrameSize, txtBuffer, txtBufferSz) != 0)
                    {
                        break;
                    }
                    D1_printf(" ALBUM : %s\r\n", txtBuffer);
                }
                else
                {
                    //D1_printf("#### CHECK!! No Match\r\n");
                    if(f_lseek(pInFile, f_tell(pInFile) + unFrameSize) != FR_OK)
                    {
                        return 1;
                    }
                }
            }
            D1_printf("--------------------------------------\r\n\n");

            D1_printf("Mp3Decode: Skipping %u bytes of ID3v2 tag\r\n", unSkip + 1);
        }

        if(f_lseek(pInFile, unSkip) != FR_OK)
        {
            return 1;
        }
    }

    return 0;
}


/****************************************************************************************************
 * @fn      MSC_Application
 *          Main routine for Mass Storage Class.
 *
 ***************************************************************************************************/
static void MSC_Application(void)
{
    /* Register the file system object to the FatFs module */
    D1_printf("Mount FAT Volume...");
    if(f_mount(&USBDISKFatFs, (TCHAR const*)USBDISKPath, 0) != FR_OK)
    {
        /* FatFs Initialization Error */
        D1_printf("FAILED!\r\n");
        Error_Handler();
    }
    else
    {
        D1_printf("SUCCESS\r\n");
        LED_On(LED_GREEN);
        Mp3Play();
    }
}


/****************************************************************************************************
 * @fn      USBH_UserProcess
 *          User process for handling USB Host stack messages.
 *
 ***************************************************************************************************/
static void USBH_UserProcess(USBH_HandleTypeDef *phost, uint8_t id)
{  
    switch(id)
    { 
    case HOST_USER_SELECT_CONFIGURATION:
        break;

    case HOST_USER_DISCONNECTION:
        SendAppEvent( DISCONNECTION_EVENT );
        LED_Off(LED_GREEN); 
        LED_Off(LED_RED);  
        break;

    case HOST_USER_CLASS_ACTIVE:
        SendAppEvent( CONNECTION_EVENT );
        break;

    default:
        break;
    }
}


/****************************************************************************************************
 * @fn      SendAppEvent
 *          Sends events to the Application task
 *
 ***************************************************************************************************/
static void SendAppEvent( uint32_t evtId )
{
    MessageBuffer *pSendMsg = NULLP;
    ASF_assert( ASFCreateMessage( MSG_APP_EVENT, sizeof(MsgGeneric), &pSendMsg ) == ASF_OK );
    pSendMsg->msg.msgAppEvent.U.dword = evtId;
    ASF_assert( ASFSendMessage( MP3_APP_TASK_ID, pSendMsg ) == ASF_OK );
}


/****************************************************************************************************
 * @fn      Mp3Decode
 *          Main decoder routine
 *
 ***************************************************************************************************/
int Mp3Decode(const char* pszFile)
{
    int nResult = 0;
    BYTE* pInData = g_Mp3InBuffer;
    UINT unInDataLeft = 0;
    FIL fIn;
    UINT bEof = FALSE;
    UINT bOutOfData = FALSE;
    MP3FrameInfo mp3FrameInfo;
    uint32_t unDmaBufMode = 0;
    g_pMp3DmaBufferPtr = g_pMp3DmaBuffer;
    UINT bCodecInitialized = FALSE;
    uint16_t evtFlags;
    osEvent  ret;
    int nDecodeRes = ERR_MP3_NONE;
    UINT unFramesDecoded = 0;

    FRESULT errFS = f_open(&fIn, pszFile, FA_OPEN_EXISTING | FA_READ);
    if(errFS != FR_OK)
    {
        D1_printf("Mp3Decode: Failed to open file \"%s\" for reading, err=%d\r\n", pszFile, errFS);
        return -1;
    }

    HMP3Decoder hMP3Decoder = MP3InitDecoder();
    if(hMP3Decoder == NULL)
    {
        D1_printf("Mp3Decode: Failed to initialize mp3 decoder engine\r\n");
        return -2;
    }

    D1_printf("Mp3Decode: Start decoding \"%s\"\r\n", pszFile);

    Mp3ReadId3V2Tag(&fIn);

    do
    {
        if(unInDataLeft < (2 * MAINBUF_SIZE) && (!bEof))
        {
            UINT unRead = Mp3FillReadBuffer(pInData, unInDataLeft, &fIn);
            unInDataLeft += unRead;
            pInData = g_Mp3InBuffer;
            if(unRead == 0)
            {
                bEof = 1;
            }
        }

        // find start of next MP3 frame - assume EOF if no sync found
        int nOffset = MP3FindSyncWord(pInData, unInDataLeft);
        if(nOffset < 0)
        {
            bOutOfData = TRUE;
            break;
        }
        pInData += nOffset;
        unInDataLeft -= nOffset;

        // decode one MP3 frame - if offset < 0 then bytesLeft was less than a full frame
        nDecodeRes = MP3Decode(hMP3Decoder, &pInData, (int*)&unInDataLeft, (short*)g_pMp3OutBuffer, 0);
        switch(nDecodeRes)
        {
        case ERR_MP3_NONE:
            {
                MP3GetLastFrameInfo(hMP3Decoder, &mp3FrameInfo);
                if(unFramesDecoded == 0)
                {
                    D1_printf("Mp3Decode: %d Hz %d Bit %d Channels\r\n",
                        mp3FrameInfo.samprate, mp3FrameInfo.bitsPerSample, mp3FrameInfo.nChans);
                    if((mp3FrameInfo.samprate > 48000) || (mp3FrameInfo.bitsPerSample != 16) || (mp3FrameInfo.nChans < 1) || (mp3FrameInfo.nChans > 2))
                    {
                        D1_printf("Mp3Decode: incompatible MP3 file.\r\n");
                        nResult = -5;
                        break;
                    }
                    // Duplicate data in case of mono to maintain playback speed [CHECK!! - increased playback speed]
                    if (mp3FrameInfo.nChans == 1)
                    {
                        for(int i = (mp3FrameInfo.outputSamps-1); i >= 0; i--)
                        {
                            g_pMp3OutBuffer[2 * i]     = g_pMp3OutBuffer[i];
                            g_pMp3OutBuffer[2 * i + 1] = g_pMp3OutBuffer[i];
                        }
                        mp3FrameInfo.outputSamps *= 2;
                        mp3FrameInfo.samprate /= 2;
                    }
                }
                if((unFramesDecoded) % 100 == 0)
                {
                    //D1_printf("Mp3Decode: frame %u, bitrate=%d\r\n", unFramesDecoded, mp3FrameInfo.bitrate);
                }
                unFramesDecoded++;
                g_pMp3OutBufferPtr = g_pMp3OutBuffer;

                uint32_t unOutBufferAvail= mp3FrameInfo.outputSamps;
                while(unOutBufferAvail > 0)
                {
                    // fill up the whole DMA buffer
                    uint32_t unDmaBufferSpace = 0;
                    if(unDmaBufMode == 0)
                    {
                        // fill the whole buffer
                        // DMA buf ptr was reset to beginning of the buffer
                        unDmaBufferSpace = g_pMp3DmaBuffer + MP3_DMA_BUFFER_SIZE - g_pMp3DmaBufferPtr;
                    }
                    else if(unDmaBufMode == 1)
                    {
                        // fill the first half of the buffer
                        // DMA buf ptr was reset to beginning of the buffer
                        unDmaBufferSpace = g_pMp3DmaBuffer + (MP3_DMA_BUFFER_SIZE / 2) - g_pMp3DmaBufferPtr;
                    }
                    else
                    {
                        // fill the last half of the buffer
                        // DMA buf ptr was reset to middle of the buffer
                        unDmaBufferSpace = g_pMp3DmaBuffer + MP3_DMA_BUFFER_SIZE - g_pMp3DmaBufferPtr;
                    }
                    uint32_t unCopy = unDmaBufferSpace > unOutBufferAvail ? unOutBufferAvail : unDmaBufferSpace;
                    if(unCopy > 0)
                    {
                        memcpy(g_pMp3DmaBufferPtr, g_pMp3OutBufferPtr, unCopy * sizeof(uint16_t));
                        unOutBufferAvail -= unCopy;
                        g_pMp3OutBufferPtr += unCopy;
                        unDmaBufferSpace -= unCopy;
                        g_pMp3DmaBufferPtr += unCopy;
                    }
                    if(unDmaBufferSpace == 0)
                    {
                        // DMA buffer full
                        // see if this was the first run
                        if(unDmaBufMode == 0)
                        {
                            // on the first buffer fill up,
                            // start the DMA transfer
                            if(BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, HP_VOL_LEVEL, (uint32_t)mp3FrameInfo.samprate))
                            {
                                D1_printf("Mp3Decode: audio init failed\r\n");
                                nResult = -4;
                                break;
                            }
                            bCodecInitialized = TRUE;
                            BSP_AUDIO_OUT_Play(g_pMp3DmaBuffer, MP3_DMA_BUFFER_SIZE * sizeof(uint16_t));
                        }

                        // we must wait for the DMA stream tx interrupt here
                        //result = os_evt_wait_or( DMA_EVT_HALF_TRANSFER | DMA_EVT_FULL_TRANSFER | APP_EVT_STOP_REQUEST, OS_WAIT_FOREVER );
                        //ASF_assert( result == OS_R_EVT );
                        //evFlags = os_evt_get();
                        evtFlags = 0;
                        ret = osSignalWait( 0, osWaitForever ); //0 => Any signal will resume thread
                        if (ret.status == osEventSignal)
                        {
                            evtFlags = ret.value.signals;
                        }

                        if((evtFlags & APP_EVT_STOP_REQUEST) || BUTTON)
                        {
                            // stop requested
                            D1_printf("Mp3Decode: Stop requested\r\n");
                            nResult = 1;
                            break;
                        }

                        if((evtFlags & DMA_EVT_HALF_TRANSFER) && (evtFlags & DMA_EVT_FULL_TRANSFER))
                        {
                            D1_printf("Mp3Decode: DMA out of sync (HT and TC both set)\r\n");
                            nResult = -3;
                            break;
                        }

                        if(unDmaBufMode == 0 || unDmaBufMode == 2)
                        {
                            // the DMA event we expect is "half transfer" (=2)
                            if(evtFlags & DMA_EVT_HALF_TRANSFER)
                            {
                                // set up first half mode
                                unDmaBufMode = 1;
                                g_pMp3DmaBufferPtr = g_pMp3DmaBuffer;
                            }
                            else
                            {
                                D1_printf("Mp3Decode: DMA out of sync (expected HT, got TC)\r\n");
                                nResult = -3;
                                break;
                            }
                        }
                        else
                        {
                            // the DMA event we expect is "transfer complete" (=4)
                            if(evtFlags & DMA_EVT_FULL_TRANSFER)
                            {
                                // set up last half mode
                                unDmaBufMode = 2;
                                g_pMp3DmaBufferPtr = g_pMp3DmaBuffer + (MP3_DMA_BUFFER_SIZE / 2);
                            }
                            else
                            {
                                D1_printf("Mp3Decode: DMA out of sync (expected TC, got HT)\r\n");
                                nResult = -3;
                            }
                        }
                    }
                }
                break;
            }
        case ERR_MP3_MAINDATA_UNDERFLOW:
            {
                // do nothing - next call to decode will provide more mainData
                break;
            }
        case ERR_MP3_FREE_BITRATE_SYNC:
            {
                break;
            }
        case ERR_MP3_INDATA_UNDERFLOW:
            {
                D1_printf("Mp3Decode: Decoding error ERR_MP3_INDATA_UNDERFLOW\r\n");
                bOutOfData = TRUE;
                break;
            }
        default:
            {
                D1_printf("Mp3Decode: Decoding error %d\r\n", nDecodeRes);
                bOutOfData = TRUE;
                break;
            }
        }
    } while((!bOutOfData) && (nResult == 0));

    D1_printf("Mp3Decode: Finished decoding\r\n------------------------------------\r\n\n");

    MP3FreeDecoder(hMP3Decoder);

    if (bCodecInitialized == TRUE)
    {
        if(BSP_AUDIO_OUT_Stop(CODEC_PDWN_HW))
        {
            D1_printf("Mp3Decode: Failed to stop audio\r\n");
        }
        //BSP_AUDIO_OUT_DeInit(); //TODO
    }
    f_close(&fIn);

    if (nResult == 1)
    {
        while(BUTTON); //Wait for user button release
    }

    /* Get rid of any pending flags that might have been posted from ISR */
    ret = osSignalWait( 0, 50 ); //0 => Any signal will resume thread
    //result = os_evt_wait_or( DMA_EVT_HALF_TRANSFER | DMA_EVT_FULL_TRANSFER | APP_EVT_STOP_REQUEST, MSEC_TO_TICS(50) );
    //D1_printf("EVT-WAIT result = %d\r\n", result);
    //if ( result == OS_R_EVT )
    //{
    //    evFlags = os_evt_get();
        //D1_printf("EVT-GET result = %04X\r\n", evFlags);
    //}

    return nResult;
}


/****************************************************************************************************
 * @fn      Mp3Play
 *          MP3 Application handler
 *
 ***************************************************************************************************/
static void Mp3Play( void )
{
    FRESULT res;
    FILINFO fno;
    DIR dir;
    char *fn; /* This function is assuming non-Unicode cfg. */
    char buffer[200];
    char *path = "";
#if _USE_LFN //Long File Name option
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof(lfn);
#endif

    res = f_opendir(&dir, path); /* Open the directory */
    if (res == FR_OK)
    {
        for (;;)
        {
            res = f_readdir(&dir, &fno); /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0)
            {
                path = "Music";
                res = f_opendir(&dir, path); /* Open the directory again */
                if (res != FR_OK)
                {
                    D1_printf("Failed to open '%s' folder\r\n", path);
                    break;
                }
                continue;
                //break; /* Break on error or end of dir */
            }
            if (fno.fname[0] == '.') continue; /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR)
            {
                /* It is a directory */
            }
            else
            { /* It is a file. */
                sprintf(buffer, "%s/%s", path, fn);
                //D1_printf("File found: %s\r\n", buffer);

                // Check if it is an mp3 file
                if (strcmp("mp3", get_filename_ext(buffer)) == 0)
                {
                    Mp3Decode( buffer );
                }
            }
        }
    }

}


/*-------------------------------------------------------------------------------------------------*\
 |    P U B L I C     F U N C T I O N S
\*-------------------------------------------------------------------------------------------------*/

/**
  * @brief  Manages the DMA full Transfer complete event.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    osSignalSet( asfTaskHandleTable[MP3_APP_TASK_ID].handle, DMA_EVT_FULL_TRANSFER );
    //isr_evt_set(DMA_EVT_FULL_TRANSFER, asfTaskHandleTable[MP3_APP_TASK_ID].handle );
}

/**
  * @brief  Manages the DMA Half Transfer complete event.
  * @param  None
  * @retval None
  */
void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
    osSignalSet( asfTaskHandleTable[MP3_APP_TASK_ID].handle, DMA_EVT_HALF_TRANSFER );
    //isr_evt_set(DMA_EVT_HALF_TRANSFER, asfTaskHandleTable[MP3_APP_TASK_ID].handle );
}

/****************************************************************************************************
 * @fn      osMessagePut
 *          Sends events to the USB Host task. Called by USB Stack sources
 *
 ***************************************************************************************************/
#if 1//ndef __CMSIS_RTOS
void ASFMessagePut( TaskId tid, uint32_t info, uint32_t timeout )
{
    osSignalSet( asfTaskHandleTable[tid].handle, info );
//    if (GetContext() == CTX_ISR)
//    {
//        isr_evt_set(info, asfTaskHandleTable[tid].handle );
//    }
//    else
//    {
//        os_evt_set(info, asfTaskHandleTable[tid].handle );
//    }
}
#endif

/****************************************************************************************************
 * @fn      Mp3PlayerTask
 *          This task handles USB Host (OTG) stack.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
ASF_TASK void Mp3PlayerTask( ASF_TASK_ARG )
{
    MessageBuffer *rcvMsg = NULLP;

    D1_printf(":: MP3 Player Task Ready ::\r\n");

    /* Init Host Library */
    USBH_Init(&hUSB_Host, USBH_UserProcess, 0);

    /* Add Supported Class */
    USBH_RegisterClass(&hUSB_Host, USBH_MSC_CLASS);

    /* Start Host Process */
    USBH_Start(&hUSB_Host);


    while(1)
    {
        ASFReceiveMessage( MP3_APP_TASK_ID, &rcvMsg );

        if (rcvMsg->msgId ==  MSG_APP_EVENT)
        {
            switch (rcvMsg->msg.msgAppEvent.U.dword)
            {
            case CONNECTION_EVENT:
                /* Link the USB Host disk I/O driver */
                if(FATFS_LinkDriver(&USBH_Driver, USBDISKPath) == 0)
                {
                    MSC_Application();
                }
                else
                {
                    D1_printf("FATFS_LinkDriver Failed!\r\n");
                }
                break;

            case DISCONNECTION_EVENT:
                f_mount(NULL, (TCHAR const*)"", 0);
                /* Unlink the USB disk I/O driver */
                FATFS_UnLinkDriver(USBDISKPath);
                LED_Off(LED_BLUE);
                LED_Off(LED_GREEN);
                break;

            default:
                break;
            }
        }
    }
}

#if 1//ndef __CMSIS_RTOS
/****************************************************************************************************
 * @fn      UsbHostTask
 *          This task handles USB Host (OTG) stack.
 *
 * @param   none
 *
 * @return  none
 *
 ***************************************************************************************************/
ASF_TASK void UsbHostTask( ASF_TASK_ARG )
{
    //MessageBuffer *rcvMsg = NULLP;
    //OS_RESULT result;
    uint16_t flags;
    osEvent  ret;

    D1_printf(":: USB HOST IF Task Ready ::\r\n");

    while(1)
    {
        flags = 0;
        ret = osSignalWait( 0, osWaitForever ); //0 => Any signal will resume thread
        if (ret.status == osEventSignal)
        {
            flags = ret.value.signals;
        }
//        result = os_evt_wait_or( 0x00FF, OS_WAIT_FOREVER );
//        ASF_assert( result == OS_R_EVT );
//        flags = os_evt_get();
        USBH_Process( &hUSB_Host );
        (void)flags;
    }
}
#endif


/*-------------------------------------------------------------------------------------------------*\
 |    E N D   O F   F I L E
\*-------------------------------------------------------------------------------------------------*/

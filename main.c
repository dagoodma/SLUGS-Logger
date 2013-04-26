/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */
#define UART2_BUFFER_SIZE 512
#define SD_SECTOR_SIZE 512
#define CB_SIZE 512*35
#define SD_IN !SD_CD

#define FCY 40000000
#define INT_P_MIN 1
#define T2_PRESCALE FCY / INT_P_MIN * 60 - 1

#include <stdint.h>
#include "CircularBuffer.h"
#include <xc.h>
// The Uartx.h files need to be included before stddef.h
#include "Uart2.h"
#include "Uart1.h"
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include "NewSDWrite.h"
#include "Node.h"
#include "Timer2.h"

_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
void Timer2InterruptRoutine(void);

CircularBuffer circBuf;
unsigned char cbData[CB_SIZE];


// for diagnostics
uint32_t timeStamp;
uint32_t maxBuffer;
uint32_t latestMaxBuffer;
uint32_t failedWrites;

/*
 * 
 */
int main(void)
{
    // Watchdog Timer Enabled/disabled by user software
    // (LPRC can be disabled by clearing SWDTEN bit in RCON registe

    // Configure Oscillator to operate the device at 40Mhz
    // Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
    // Fosc= 8M*40/(2*2)=80Mhz for 8M input clock
    PLLFBD = 38; // M=40
    CLKDIVbits.PLLPOST = 0; // N1=2
    CLKDIVbits.PLLPRE = 0; // N2=2
    OSCTUN = 0; // Tune FRC oscillator, if FRC is used

    RCONbits.SWDTEN = 0; /* Disable Watch Dog Timer*/

    // Clock switch to incorporate PLL
    __builtin_write_OSCCONH(0x03); // Initiate Clock Switch to Primary
    // Oscillator with PLL (NOSC=0b011)
    __builtin_write_OSCCONL(0x01); // Start clock switching
    while (OSCCONbits.COSC != 0b011); // Wait for Clock switch to occur

    while (OSCCONbits.LOCK != 1) {}; /* Wait for PLL to lock*/

    Uart2Init(NewSDInit(), Uart2InterruptRoutine);
    Uart1Init(BRGVAL);

    timeStamp = 0;
    Timer2Init(&Timer2InterruptRoutine, 0xFFFF);

    if (!CB_Init(&circBuf, cbData, CB_SIZE)) FATAL_ERROR();

    int SDConnected = 0;
    maxBuffer = 0;
    latestMaxBuffer = 0;
    while(1)
    {
        if (SD_IN)
        {
            // if the board was just plugged in try to reinitialize
            if(!SDConnected) {
                MEDIA_INFORMATION * Minfo;
                do {
                    Minfo = MDD_MediaInitialize();
                } while(Minfo->errorCode == MEDIA_CANNOT_INITIALIZE);
                SDConnected = 1;
            }
            // When we are connected and initialized, poll the buffer, if there
            // is data, write it.
            unsigned char outData[SD_SECTOR_SIZE];
            if (CB_PeekMany(&circBuf, outData, SD_SECTOR_SIZE)){
                if(NewSDWriteSector(outData)){
                    // Remove the data we just written.
                    CB_Remove(&circBuf, SD_SECTOR_SIZE);
                } else failedWrites++;
            }
        } else {
            SDConnected = 0;
        }
    }
}

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    CB_WriteMany(&circBuf, Buffer, BufferSize, true); // fail early
    if(circBuf.dataSize >= maxBuffer) maxBuffer = circBuf.dataSize;
    if(circBuf.dataSize >= latestMaxBuffer) latestMaxBuffer = circBuf.dataSize;
}

void Timer2InterruptRoutine(void)
{
    timeStamp += 1;

    Uart1WriteByte('!');
    Uart1WriteData(&timeStamp, sizeof(timeStamp));
    Uart1WriteByte(latestMaxBuffer >> 9);
    Uart1WriteByte(maxBuffer >> 9);
    Uart1WriteData(&failedWrites, sizeof(failedWrites));
    latestMaxBuffer = 0;
}
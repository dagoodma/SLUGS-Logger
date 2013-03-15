/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */
#define UART2_BUFFER_SIZE 512
#define DATA_SIZE 512*10
#define SD_IN !SD_CD

#include <stdint.h>
#include "CircularBuffer.h"
#include <xc.h>
#include "Uart2.h"
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include "NewSDWrite.h"
#include "Node.h"

_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

void InterruptRoutine(unsigned char *Buffer, int BufferSize);

FSFILE *file;
CircularBuffer circBuf;
unsigned char data[DATA_SIZE];
unsigned char fakeInput[UART2_BUFFER_SIZE]; // DEBUG a fake input for the cbuf
//unsigned char * bufferPointer;
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

    while (OSCCONbits.LOCK != 1) {
    }; /* Wait for PLL to lock*/

//    bufferPointer = NULL;
    Uart2Init(InterruptRoutine);

    if (!CB_Init(&circBuf, data, DATA_SIZE)) FATAL_ERROR();

    // Generate a fake input to record to the circular buffer
    // give beginning and end special characters
    int i;
    fakeInput[0] = 'A';
    for (i=1; i<510; i++) {
        if (i%27 == 26)
            fakeInput[i] = '\n';
        else
            fakeInput[i] = i%27 + 'a';
    }
    fakeInput[510] = 'Z';
    fakeInput[511] = '\n';
    
    Uart2PrintStr("Begin.\n");
    file = NewSDInit("newfile.txt");

    int SDConnected = 0;
    while(1)
    {
//        if (bufferPointer != NULL) { TODO implement this?
//            CB_WriteMany(&circBuf, bufferPointer, UART2_BUFFER_SIZE, 1); // the 1 is arbitrary
//            bufferPointer = NULL;
//        }
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
            unsigned char outData[UART2_BUFFER_SIZE];
            if (CB_PeekMany(&circBuf, outData, UART2_BUFFER_SIZE)){
                if(NewSDWriteSector(file, outData)){
                    // Remove the data we just wrote.
                    CB_Remove(&circBuf, UART2_BUFFER_SIZE);
                }
            }
        } else {
            SDConnected = 0;
        }
    }
}

void InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    // When one buffer has been filled
    // Print both buffers (sorta echo)
//    int i;
//    for (i = 0; i < BufferSize; i++) {
//        Uart2PrintChar(Buffer[i]);
//    }
//     Set the pointer so we can copy the data in main.
//    bufferPointer = Buffer;

    CB_WriteMany(&circBuf, &fakeInput, 512, 1); // fake data
    // CB_WriteMany(&circBuf, Buffer, BufferSize, 1); // the 1 is arbitrary
}
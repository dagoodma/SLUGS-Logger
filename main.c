/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */
#define UART2_BUFFER_SIZE 512
#define DATA_SIZE 512*10

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

    Uart2Init(InterruptRoutine);

    if (!CB_Init(&circBuf, data, DATA_SIZE)) FATAL_ERROR();

    
    Uart2PrintStr("Begin.\n");
    file = NewSDInit("newfile.txt");
    while(1)
    {
        if (MDD_MediaDetect()){
            unsigned char outData[UART2_BUFFER_SIZE];
            if (CB_PeekMany(&circBuf, outData, UART2_BUFFER_SIZE)){
                if(NewSDWriteSector(file, outData)){
                    CB_Remove(&circBuf, UART2_BUFFER_SIZE);
                }
            }
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

    CB_WriteMany(&circBuf, Buffer, BufferSize, 1); // the 1 is arbitrary
}
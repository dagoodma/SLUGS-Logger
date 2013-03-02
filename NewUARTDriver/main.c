/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */
#define UART2_BUFFER_SIZE 512

#include <xc.h>
#include "Uart2.h"
#include <stddef.h>
#include "FSIO.h"
#include "NewSDWrite.h"

_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

void InterruptRoutine(unsigned char *Buffer, int BufferSize);

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


    NewSDInit();

    unsigned char outbuf[512]; // generate some data
    unsigned long i;
    for (i = 0; i < 512; i++) {
        outbuf[i] = i % 26 + 'a';
    }

    NewSDWriteSector(*outbuf);

    while(1);
}

void InterruptRoutine(unsigned char *Buffer, int BufferSize) // not used currently
{
    // When one buffer has been filled
    // Print both buffers (sorta echo)
    int i;
    for (i = 0; i < BufferSize; i++) {
        Uart2PrintChar(Buffer[i]);
    }
    OFB_set(Buffer);
}
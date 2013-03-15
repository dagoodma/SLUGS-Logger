/* 
 * File:   main.c
 * Author: jesseharkin
 *
 * Created on March 14, 2013, 3:50 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "uart2.h"


_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

void InterruptRoutine(unsigned char *Buffer, int BufferSize);

/*
 * 
 */
void main()
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

    int i;
    unsigned char j;
    for (i = 0; i < 1000; i++) {
        for (j = 0; j<26 ; j++) {
            Uart2PrintChar(j+'a');
        }
        Uart2PrintChar('\n');
    }

    while(1);
}

void InterruptRoutine(unsigned char *Buffer, int BufferSize){}

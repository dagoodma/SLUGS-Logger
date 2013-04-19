/* 
 * File:   main.c
 * Author: jesseharkin
 *
 * Created on March 14, 2013, 3:50 PM
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include "Uart2.h"
#include "book.h"


_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

void InterruptRoutine(unsigned char *Buffer, int BufferSize);
char checksum(char * data, int dataSize);

/*
 * 
 */
void main() {
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

    // configure the button as input
    TRISDbits.TRISD6 = 1;

    char toSend[512];
    int i;
    for (i = 0; i < 512; i++) {
        toSend[i] = (char) i;
    }

    Uart2Init(InterruptRoutine);

    while (PORTDbits.RD6);

    uint32_t j = 0;
    for (i = 0; i < 720; i++) {
        j = 0;
        while (book[j] != '\0') {
            Uart2PrintChar(book[j++]);
        }
    }

    while (1) {
    }
}

void InterruptRoutine(unsigned char *Buffer, int BufferSize) {
}

// calculates a basic byte Xor checksum of some data

char checksum(char * data, int dataSize) {
    char sum = 0;
    int i;
    for (i = 0; i < dataSize; i++) {
        sum ^= data[i];
    }
    return sum;
}
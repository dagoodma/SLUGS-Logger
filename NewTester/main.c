/* 
 * File:   main.c
 * Author: jesseharkin
 *
 * Created on March 14, 2013, 3:50 PM
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <xc.h>
#include "Uart2.h"
#include <pps.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"

#define SD_IN (!SD_CD)

// Initial setup for the clock
_FGS( GCP_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_FRC & IESO_OFF )
_FOSC( FCKSM_CSDCMD & OSCIOFNC_ON & POSCMD_NONE )
_FWDT( FWDTEN_OFF & PLLKEN_ON )
_FICD( JTAGEN_OFF & ICS_PGD2 )
// Inital setup for the clock

void InterruptRoutine(unsigned char *Buffer, int BufferSize);
char checksum(char * data, int dataSize);
void initPins(void);

/*
 * 
 */
int main()
{
    // Configure Oscillator to operate the device at 40Mhz
    // Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
    // Fosc= 7.37*43/(2*2)=80Mhz for 7.37 input clock
    PLLFBD = 43;                  // M=43
    CLKDIVbits.PLLPOST=0;       // N1=2
    CLKDIVbits.PLLPRE=0;        // N2=2
    OSCTUN=0;                   // Tune FRC oscillator, if FRC is used

    // Disable Watch Dog Timer
    RCONbits.SWDTEN=0;
    
    initPins();
    
    Uart2Init(115200L, InterruptRoutine);
//    TRISAbits.TRISA4 = 0;
//    LATAbits.LATA4 = 0;
//    while(1) {
//        LATAbits.LATA4 = SD_IN;
//    }

    Uart2PrintChar('S');
    printf("S");
    while (!SD_IN);
    
    // initialize the file system, open the file, read the file and send in chunks
    FSInit();

    FSFILE * openFile = FSfopen("send.txt", FS_READ);

    char toSend[512];
    int n;
    while (n = FSfread(toSend, 1, 512, openFile)) {
        Uart2SendBytes(toSend, n);
    }
    FSfclose(openFile);

    // turn on amber LED
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 1;
    
    while (1);
}

void InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
}

// calculates a basic byte Xor checksum of some data

char checksum(char * data, int dataSize)
{
    char sum = 0;
    int i;
    for (i = 0; i < dataSize; i++) {
        sum ^= data[i];
    }
    return sum;
}

/**
 * Initialize pin mappings and set pins as digital.
 */
void initPins(void)
{
    // And configure the Peripheral Pin Select pins:
    PPSUnLock;
    // To enable ECAN1 pins: TX on 7, RX on 4
    PPSOutput(OUT_FN_PPS_C1TX, OUT_PIN_PPS_RP39);
    PPSInput(PPS_C1RX, PPS_RP20);

    // To enable UART2 pins: TX on 11, RX on 13
    PPSOutput(OUT_FN_PPS_U2TX, OUT_PIN_PPS_RP43);
    PPSInput(PPS_U2RX, PPS_RPI45);
    PPSLock;

    // Disable A/D functions on pins
    ANSELA = 0;
    ANSELB = 0;
}
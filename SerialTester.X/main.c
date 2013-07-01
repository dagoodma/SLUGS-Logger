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
#include <pps.h>


// Use internal RC to start; we then switch to PLL'd iRC.
_FOSCSEL(FNOSC_FRC & IESO_OFF);
// Clock Pragmas
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
// Disable watchdog timer
_FWDT(FWDTEN_OFF);
// Disable JTAG and specify port 3 for ICD pins.
_FICD(JTAGEN_OFF & ICS_PGD3);

void InterruptRoutine(unsigned char *Buffer, int BufferSize);
char checksum(char * data, int dataSize);
void initPins(void);

/*
 * 
 */
void main() {
     // Switch the clock over to 80MHz.
    PLLFBD = 63;            // M = 65
    CLKDIVbits.PLLPOST = 0; // N1 = 2
    CLKDIVbits.PLLPRE = 1;  // N2 = 3

    __builtin_write_OSCCONH(0x01); // Initiate Clock Switch to

    __builtin_write_OSCCONL(OSCCON | 0x01); // Start clock switching

    while (OSCCONbits.COSC != 1); // Wait for Clock switch to occur

    while (OSCCONbits.LOCK != 1);

    initPins();
    
    Uart2Init((long)115200, InterruptRoutine);

    Uart2PrintChar('!');
//    int i;
//    uint32_t j = 0;
//    for (i = 0; i < 1; i++) {
//        j = 0;
//        while (book[j] != '\0') {
//            Uart2PrintChar(book[j++]);
//        }
//    }

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

/**
 * Initialize pin mappings and set pins as digital.
 */
void initPins(void)
{
    PPSUnLock;

    // To enable UART1 pins: TX on 11, RX on 13
	PPSOutput(OUT_FN_PPS_U2TX, OUT_PIN_PPS_RP11);
	PPSInput(PPS_U2RX, PPS_RP13);

	// Configure SPI1 so that:
	//  * (input) SPI1.SDI = B10
	PPSInput(PPS_SDI1, PPS_RP10);
	//  * SPI1.SCK is output on B15
	PPSOutput(OUT_FN_PPS_SCK1, OUT_PIN_PPS_RP15);
	//  * (output) SPI1.SDO = B1
	PPSOutput(OUT_FN_PPS_SDO1, OUT_PIN_PPS_RP1);

	PPSLock;

	// Enable pull-up on open-drain card detect pin.
	CNPU1bits.CN2PUE = 1;

    // Configure all used pins to not be AD pins and
    // be digital I/O instead.
    AD1PCFGLbits.PCFG0 = 1;
    AD1PCFGLbits.PCFG1 = 1;
    AD1PCFGLbits.PCFG2 = 1;
    AD1PCFGLbits.PCFG3 = 1;
    AD1PCFGLbits.PCFG4 = 1;
    AD1PCFGLbits.PCFG5 = 1;
}
/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 *
 * Known bug: Sometimes taking the SD card out causes a stack and/or address error. As far as we
 * have tested, these occur simultaniously, but this is not confirmed. It might happend if the card
 * is taken out while writing to it, but this is also not confirmed.
 */
#define UART2_BUFFER_SIZE 512
#define SD_SECTOR_SIZE 512
#define CB_SIZE 512*10
#define SD_IN !SD_CD

#include <stdint.h>
#include "CircularBuffer.h"
#include <xc.h>
#include <pps.h>
// The Uartx.h files need to be included before stddef.h
#include "Uart2.h"
#include "Uart1.h"
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include "NewSDWrite.h"
#include "Node.h"
#include "Timer2.h"

// Use internal RC to start; we then switch to PLL'd iRC.
_FOSCSEL(FNOSC_FRC & IESO_OFF);
// Clock Pragmas
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
// Disable watchdog timer
_FWDT(FWDTEN_OFF);
// Disable JTAG and specify port 3 for ICD pins.
_FICD(JTAGEN_OFF & ICS_PGD3);

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
void Timer2InterruptRoutine(void);
void setLeds(char input);
void initPins(void);

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
    // Switch the clock over to 80MHz.
    PLLFBD = 63;            // M = 65
    CLKDIVbits.PLLPOST = 0; // N1 = 2
    CLKDIVbits.PLLPRE = 1;  // N2 = 3

    __builtin_write_OSCCONH(0x01); // Initiate Clock Switch to

    __builtin_write_OSCCONL(OSCCON | 0x01); // Start clock switching

    while (OSCCONbits.COSC != 1); // Wait for Clock switch to occur

    while (OSCCONbits.LOCK != 1);
    
    initPins();

    while (!SD_IN);

    Uart2Init(NewSDInit(), Uart2InterruptRoutine);
    Uart1Init(BRGVAL);

    timeStamp = 0;
    Timer2Init(&Timer2InterruptRoutine, 0xFFFF);

    if (!CB_Init(&circBuf, cbData, CB_SIZE)) {
        FATAL_ERROR();
    }
    
    int SDConnected = 0;
    maxBuffer = 0;
    latestMaxBuffer = 0;
    Uart2PrintChar('N');
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

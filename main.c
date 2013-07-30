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
#include <stdint.h>
#include "CircularBuffer.h"
#include <xc.h>
#include <pps.h>
// The UartX.h files need to be included before stddef.h
#include "Uart2.h"
#include "Uart1.h"
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include "NewSDWrite.h"
#include "Node.h"
#include "Timer2.h"
#include "DEE Emulation 16-bit.h"

#define SD_SECTOR_SIZE (BYTES_PER_SECTOR)
#define CB_SIZE (UART2_BUFFER_SIZE * 12)
#define SD_IN (!SD_CD)

// Initial setup for the clock
_FGS( GCP_OFF & GWRP_OFF )
_FOSCSEL( FNOSC_FRC & IESO_OFF )
_FOSC( FCKSM_CSDCMD & OSCIOFNC_ON & POSCMD_NONE )
_FWDT( FWDTEN_OFF & PLLKEN_ON )
_FICD( JTAGEN_OFF & ICS_PGD2 )
// Inital setup for the clock

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
void Timer2InterruptRoutine(void);
void setLeds(char input);
void initPins(void);

CircularBuffer circBuf;
unsigned char cbData[CB_SIZE];
Sector tempSector;


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

    while (!SD_IN);

    DataEEInit();
    Uart2Init(NewSDInit(), Uart2InterruptRoutine);

    timeStamp = 0;

    if (!CB_Init(&circBuf, cbData, CB_SIZE)) {
        FATAL_ERROR();
    }
    
    int SDConnected = 0;
    maxBuffer = 0;
    latestMaxBuffer = 0;
    Uart2PrintChar('N');

    unsigned int eetest = DataEERead(1);
    if (eetest != 0xFFFF) {
        Uart2PrintChar(eetest);
    }
    
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
            
            tempSector.sectorFormat.maxBuffer = maxBuffer/505;
            if (CB_PeekMany(&circBuf, tempSector.sectorFormat.data, UART2_BUFFER_SIZE)){
                if(NewSDWriteSector(&tempSector)){
                    // Remove the data we just written.
                    CB_Remove(&circBuf, UART2_BUFFER_SIZE);
//                    maxBuffer = 0; // reset the buffer count for the next time we send
                } else failedWrites++;
            }
        } else {
            SDConnected = 0;
        }
    }
}

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    if (Buffer[0] == '\0') { // NOT FOR FINAL CODE
        Buffer[0] = '0';
    }
    CB_WriteMany(&circBuf, Buffer, BufferSize, true); // fail early
    if(circBuf.dataSize >= maxBuffer) maxBuffer = circBuf.dataSize;
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

    // To enable UART1 pins: TX on 11, RX on 13
    PPSOutput(OUT_FN_PPS_U1TX, OUT_PIN_PPS_RP43);
    PPSInput(PPS_U1RX, PPS_RPI45);
    PPSLock;

    // Disable A/D functions on pins
    ANSELA = 0;
    ANSELB = 0;
}

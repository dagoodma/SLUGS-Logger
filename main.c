/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
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
#define CB_SIZE (UART2_BUFFER_SIZE * 38)
#define SD_IN (!SD_CD)

// Initial setup for the clock
_FOSCSEL(FNOSC_FRC & PWMLOCK_OFF);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);
_FICD(JTAGEN_OFF & ICS_PGD3);
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
    //Clock init  M=43, N1,2 = 2 == 39.61MIPS
	PLLFBD = 43;
	CLKDIVbits.PLLPOST = 0; // N1 = 2
	CLKDIVbits.PLLPRE = 0; // N2 = 2
	OSCTUN = 0;
	RCONbits.SWDTEN = 0;

	__builtin_write_OSCCONH(0x01); // Initiate Clock Switch to Primary (3?)

	__builtin_write_OSCCONL(0x01); // Start clock switching

	while (OSCCONbits.COSC != 0b001); // Wait for Clock switch to occur

	while (OSCCONbits.LOCK != 1) {
	};
	//End of clock init.
    
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

    // To enable UART2 pins: TX on 11, RX on 13
    PPSOutput(OUT_FN_PPS_U2TX, OUT_PIN_PPS_RP43);
    PPSInput(PPS_U2RX, PPS_RPI45);

    // enable the SPI stuff: clock, in, out
    PPSOutput(OUT_FN_PPS_SCK2, OUT_PIN_PPS_RP41);
    PPSOutput(OUT_FN_PPS_SDO2, OUT_PIN_PPS_RP40);
    PPSInput(PPS_SDI2, PPS_RP42);
    PPSLock;

    // Disable A/D functions on pins
    ANSELA = 0;
    ANSELB = 0;
}

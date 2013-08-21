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
#include "NewSDWrite.h"
// The UartX.h files need to be included before stddef.h
#include "Uart2.h"
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include "Node.h"
#include "Timer2.h"
#include "DEE Emulation 16-bit.h"
#include <stdio.h>

#define SD_SECTOR_SIZE (BYTES_PER_SECTOR)
#define CB_SIZE (UART2_BUFFER_SIZE * 20)
#define SD_IN (!SD_CD)

/*
 * Pic shadow register pragmas.  These set main oscillator sources, and
 * other low-level hardware stuff like PGD/PGC (debug/programming) pin positions.
 */
#ifdef _FSS       /* for chip with memory protection options */
_FSS(RSS_NO_RAM & SSS_NO_FLASH & SWRP_WRPROTECT_OFF)
#endif
_FOSCSEL(FNOSC_FRC & PWMLOCK_OFF);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);
_FICD(JTAGEN_OFF & ICS_PGD2);

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
void Timer2InterruptRoutine(void);
void setLeds(char input);
void initPins(void);

CircularBuffer circBuf;
__eds__ unsigned char __attribute__((eds, space(eds))) cbData[CB_SIZE];
Sector tempSector;
int sdConnected;

// Debug variables
uint16_t maxCbGlobal;
uint16_t writeFailes;
uint16_t cbSize;
int cbFilling;
char string[15];

int main()
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

    if (DataEEInit()) { // must be called prior to any other operation
        // DataEEInit failed
        FATAL_ERROR();
    }

    initPins();

    while (!SD_IN);
    sdConnected = 1;

    // turn on amber LED
    TRISAbits.TRISA4 = 0;
    LATAbits.LATA4 = 1;

    Uart2Init(NewSDInit(), Uart2InterruptRoutine);
    Uart2PrintChar('S');

    if (!CB_Init(&circBuf, cbData, CB_SIZE)) {
        FATAL_ERROR();
    }

    while (1) {
        if (SD_IN) {
            // if the card was just plugged in try to reinitialize
            if (!sdConnected) {
                MEDIA_INFORMATION * Minfo;
                do {
                    Minfo = MDD_MediaInitialize();
                } while (Minfo->errorCode == MEDIA_CANNOT_INITIALIZE);
                sdConnected = 1;
            }
            // When we are connected and initialized, poll the buffer, if there
            // is data, write it.
            if (CB_PeekMany(&circBuf, tempSector.sectorFormat.data, UART2_BUFFER_SIZE)) {
                if (NewSDWriteSector(&tempSector)) {
                    // Remove the data we just written.
                    CB_Remove(&circBuf, UART2_BUFFER_SIZE);

                    // Debug: just after writing
                    if (cbSize >= 2 && cbFilling) {
                        //print buffer size
                        sprintf(string, "%d ", cbSize);
                        Uart2PrintStr(string);
                    }
                    cbFilling = 0;
                    cbSize -= 1;
                } else { // write failed
                    Uart2PrintStr("WF ");
                }
            }
        } else {
            sdConnected = 0;
        }
    }
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

    // enable the SPI stuff: clock, in, out
    PPSOutput(OUT_FN_PPS_SCK2, OUT_PIN_PPS_RP41);
    PPSOutput(OUT_FN_PPS_SDO2, OUT_PIN_PPS_RP40);
    PPSInput(PPS_SDI2, PPS_RP42);
    PPSLock;

    // Disable A/D functions on pins
    ANSELA = 0;
    ANSELB = 0;
}

void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    CB_WriteMany(&circBuf, Buffer, BufferSize, true); // fail early

    // Debug: just after recieving
    cbSize += 1;
    if (cbSize > maxCbGlobal) {
        maxCbGlobal = cbSize;
    }
    cbFilling = 1;
}
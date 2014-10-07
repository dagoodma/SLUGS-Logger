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
#include "NewSDWrite/NewSDWrite.h"
// The UartX.h files need to be included before stddef.h
#include "Uart2.h"
#include <stddef.h>
#include "MDD File System/FSIO.h"
#include "Node.h"
#include "DEE/DEE Emulation 16-bit.h"
#include <stdio.h>

// The sector size of the SD card in bytes
#define SD_SECTOR_SIZE (BYTES_PER_SECTOR)
// The circular buffer size in bytes, enough to hold 20 full UART2 buffers
#define CB_SIZE (UART2_BUFFER_SIZE * 20)
// The state of the SD card, 0 means that no SD card is inserted.
#define SD_IN (!SD_CD)

/*
 * Pic shadow register pragmas.  These set main oscillator sources, and
 * other low-level hardware stuff like PGD/PGC (debug/programming) pin positions.
 */
#ifdef _FSS       /* for chip with memory protection options */
_FSS(RSS_NO_RAM & SSS_NO_FLASH & SWRP_WRPROTECT_OFF)
#endif
_FOSCSEL(FNOSC_FRC & PWMLOCK_OFF);
_FOSC(FCKSM_CSECMD & OSCIOFNC_ON & POSCMD_NONE);
_FWDT(FWDTEN_OFF);
_FICD(JTAGEN_OFF & ICS_PGD2);

static void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
static void InitPins(void);

static CircularBuffer circBuf;
static __eds__ unsigned char __attribute__((eds, space(eds))) cbData[CB_SIZE];
static Sector tempSector;
// Whether the SD card is connected or not. This is different from the card select
// pin as it is tracking the "true" state of the SD card, if it's connected and
// initialized properly.
static bool sdConnected;

// Debug variables
static uint16_t maxCbGlobal;
static uint16_t cbSize;
static bool cbFilling;
static char string[15];

int main()
{
    // Clock init: M=43, N1,2 = 2 == 39.61MIPS
    {
        PLLFBD = 43;
        CLKDIVbits.PLLPOST = 0; // N1 = 2
        CLKDIVbits.PLLPRE = 0; // N2 = 2
        OSCTUN = 0;
        RCONbits.SWDTEN = 0;

        __builtin_write_OSCCONH(0x01); // Initiate Clock Switch to Primary (3?)

        __builtin_write_OSCCONL(0x01); // Start clock switching

        while (OSCCONbits.COSC != 0b001); // Wait for Clock switch to occur

        while (OSCCONbits.LOCK != 1);
    }

    // Initialize EEPROM emulation library.
    // (must be called prior to any other operation)
    if (DataEEInit()) {
        FATAL_ERROR();
    }

    // Set up the peripheral pin mappings
    InitPins();

    // Initialize the CircularBuffer for receiving data
    if (!CB_Init(&circBuf, cbData, CB_SIZE)) {
        FATAL_ERROR();
    }

    // Set sdConnected to the opposite of the card detection signal. This allows
    // the normal connecting/disconnecting handlers to fire as appropriate without
    // duplication of code
    sdConnected = !SD_IN;

    // Turn on the amber LED, indicating that bootup succeeded
    _LATA4 = 1;

    // Also make sure the red LED is off, we'll use it for error status later
    _LATA3 = 0;

    // Main event loop
    while (1) {
        // If a card exists...
        if (MDD_SDSPI_MediaDetect()) {
            // If the card was just plugged in, try to reinitialize.
            if (!sdConnected) {
                // Initialize the file system
                if (!FSInit()) {
                    FATAL_ERROR();
                }

                // Attempt to read baud rate configuration data from the SD card
                unsigned long baudRate = NewSDInit();
                if (!baudRate) {
                    FATAL_ERROR();
                }

                // And configure the baud rate accordingly
                Uart2Init(baudRate, Uart2InterruptRoutine);

                // Attempt to initialize the SD card
                MEDIA_INFORMATION *minfo = MDD_MediaInitialize();
                if (minfo->errorCode == MEDIA_CANNOT_INITIALIZE) {
                    FATAL_ERROR();
                }

                // Update status, including turning off the red LED
                sdConnected = true;
                LATAbits.LATA3 = 0;
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
                    cbFilling = false;
                    cbSize -= 1;
                } else { // write failed
                    Uart2PrintStr("WF ");
                }
            }
        }
        // Otherwise, if the card has become disconnected, show the red error LED
        else if (sdConnected) {
            LATAbits.LATA3 = 1;
            sdConnected = false;
        }
    }
}

/**
 * Initialize input/output pins for the processor.
 */
static void InitPins(void)
{
    // And configure the Peripheral Pin Select pins:
    PPSUnLock;
    // To enable ECAN1 pins: TX on 7, RX on 4
    PPSOutput(OUT_FN_PPS_C1TX, OUT_PIN_PPS_RP39);
    PPSInput(IN_FN_PPS_C1RX, IN_PIN_PPS_RP20);

    // To enable UART2 pins: TX on 11, RX on 13
    PPSOutput(OUT_FN_PPS_U2TX, OUT_PIN_PPS_RP43);
    PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RPI45);

    // enable the SPI stuff: clock, in, out
    PPSOutput(OUT_FN_PPS_SCK2, OUT_PIN_PPS_RP41);
    PPSOutput(OUT_FN_PPS_SDO2, OUT_PIN_PPS_RP40);
    PPSInput(IN_FN_PPS_SDI2, IN_PIN_PPS_RP42);
    PPSLock;

    // And enable both the LED pins as outputs
    _TRISA3 = 0; // Red LED
    _TRISA4 = 0; // Amber LED

    // Disable A/D functions on all pins
    ANSELA = 0;
    ANSELB = 0;
}

static void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    CB_WriteMany(&circBuf, Buffer, BufferSize, true); // fail early

    // Debug: just after recieving
    cbSize += 1;
    if (cbSize > maxCbGlobal) {
        maxCbGlobal = cbSize;
    }
    cbFilling = true;
}

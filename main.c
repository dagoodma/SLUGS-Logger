/*
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <pps.h>
#include <timer.h>
#include <xc.h>

#include "CircularBuffer.h"
#include "Uart2.h"

#include "MDD File System/FSIO.h" // This must come before the DEE file because the ERASE macro is overloaded
#include "NewSDWrite/NewSDWrite.h"

// The sector size of the SD card in bytes
#define SD_SECTOR_SIZE (BYTES_PER_SECTOR)
// The circular buffer size in bytes, enough to hold 20 full UART2 buffers.
#define CB_SIZE (UART2_BUFFER_SIZE * 20)
// The timeout value for the amber LED counter
#define AMBER_LED_TIMEOUT UINT16_MAX

/**
 * Enter an infinite loop and flash one of the status LEDs at 10Hz. This should be used when there
 * is an unrecoverable error onboard, like when a subsystem fails to initialize.
 */
#define FATAL_ERROR() do {                                                         \
                          _TRISA3 = 0;                                             \
                          _LATA3 = 1;                                              \
                          while (1) {                                              \
                              unsigned long int i;                                 \
                              for (i = 0; i < GetInstructionClock() / 200; ++i);   \
                              _LATA3 ^= 1;                                         \
                          }                                                        \
                      } while (0);


/**
 * Enter an infinite loop and flash one of the status LEDs at 10Hz. This should be used when there
 * is an unrecoverable error onboard, like when a subsystem fails to initialize.
 */
#define ERROR_UNTIL_REMOVAL() do {                                                         \
                                  _TRISA3 = 0;                                             \
                                  _LATA3 = 1;                                              \
                                  while (1) {                                              \
                                      unsigned long int i;                                 \
                                      for (i = 0; i < GetInstructionClock() / 200; ++i);   \
                                      _LATA3 ^= 1;                                         \
                                      if (!MDD_MediaDetect()) {                            \
                                          break;                                           \
                                      }                                                    \
                                  }                                                        \
                              } while (0);

/*
 * Pic shadow register pragmas.  These set main oscillator sources, and
 * other low-level hardware stuff like PGD/PGC (debug/programming) pin positions.
 */
#ifdef _FSS       /* for chip with memory protection options */
_FSS(RSS_NO_RAM & SSS_NO_FLASH & SWRP_WRPROTECT_OFF)
#endif
// Use internal RC to start; we then switch to PLL'd iRC.
_FOSCSEL(FNOSC_FRC & PWMLOCK_OFF);
// We need to make sure we can configure the peripheral pins multiple times, as I need to do it
// in separate locations for the different peripherals.
_FOSC(FCKSM_CSECMD & OSCIOFNC_ON & POSCMD_NONE & IOL1WAY_OFF);
// Disable watchdog timer
_FWDT(FWDTEN_OFF);
// Disable JTAG and specify port 2 for ICD pins.
_FICD(JTAGEN_OFF & ICS_PGD2);

// Function prototypes for the helper functions below main
static void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize);
static void InitPins(void);
void SetStatusModeLed(void);

// Function prototypes for SD-SPI.c
void Delayms(BYTE milliseconds);

// The circular buffer and its dataspace
static CircularBuffer circBuf;
static __eds__ unsigned char __attribute__((eds, space(eds))) cbData[CB_SIZE];

// A temporary variable used for writing the buffer to the SD card
static Sector tempSector;

// Keep a running timer. This is relative to when a new file has been created.
static float timerCounter = 0.0;

// Track the time since data was last receiving from the circular buffer. In units of .1s.
static uint8_t timeSinceLastData = 0;

// Set a cutoff for when the system considers itself "not receiving data". 1 second seems pretty
// reasonable to receive 512b. In units of .1s.
#define DATA_TIMEOUT 10

// Flag indicating if the system is currently receiving data. Should be pretty accurate when data is
// being received, but a little slow to respond when the data flow has stopped.
// @see DATA_TIMEOUT timeSinceLastData
static bool receivingData = false;

// Whether the SD card is connected or not. This is different from the card select
// pin as it is tracking the "true" state of the SD card, if it's connected and
// initialized properly.
static bool sdConnected;

// Keep a counter for turning on the amber LED after X units of time after the
// last data packet was received.
static uint16_t ledCounter = 0;

// Keep track of which peripherals are currently enabled. Used for de-initializing them on card
// removal.
enum {
    PERIPHERAL_NONE  = 0,
    PERIPHERAL_UART1 = 0x1,
    PERIPHERAL_UART2 = 0x2,
    PERIPHERAL_ECAN  = 0x4
} activePeripherals = PERIPHERAL_NONE;

int main()
{
    // Clock init to 80MHz for 40MIPS operation
    {
	/// First step is to move over to the FRC w/ PLL clock from the default FRC clock.
	// Set the clock to 79.84MHz.
	PLLFBD = 63;            // M = 65
	CLKDIVbits.PLLPOST = 0; // N2 = 2
	CLKDIVbits.PLLPRE = 1;  // N1 = 3

	// Initiate Clock Switch to FRM oscillator with PLL.
	__builtin_write_OSCCONH(0x01);
	__builtin_write_OSCCONL(OSCCON | 0x01);

	// Wait for Clock switch to occur.
	while (OSCCONbits.COSC != 1);

	// And finally wait for the PLL to lock.
	while (OSCCONbits.LOCK != 1);
    }

    // Initialize Timer2 to trigger at 10Hz.
    const uint16_t countLimit = GetInstructionClock() / 256 / 10;
    OpenTimer2(T2_ON & T2_IDLE_CON & T2_GATE_OFF & T2_PS_1_256 & T2_32BIT_MODE_OFF & T2_SOURCE_INT, countLimit);
    ConfigIntTimer2(T2_INT_PRIOR_4 & T2_INT_ON);


    // Set up the peripheral pin mappings
    InitPins();

    // Initialize the CircularBuffer for receiving data
    // This is a fatal error as it's a hardware problem if it doesn't init and requires a reset.
    if (!CB_Init(&circBuf, cbData, CB_SIZE)) {
        FATAL_ERROR();
    }

    // Set sdConnected to the opposite of the card detection signal. This allows
    // the normal connecting/disconnecting handlers to fire as appropriate without
    // duplication of code. Since the SD_CD signal is active-low, we don't need
    // to invert it as sdConnected is active-high.
    sdConnected = SD_CD;

    // Initialize with the red LED on, indicating that the system isn't ready yet.
    // This will be cleared once the SD card is inserted.
    _LATA3 = 1;

    // Track how big the input circular buffer has gotten.
    static uint16_t biggerCircBufSize = 0;

    // Main event loop
    while (true) {

        // If a card exists...
        if (MDD_SDSPI_MediaDetect()) {
            // If the card was just plugged in, try to reinitialize.
            if (!sdConnected) {

                // Initialize the file system. This includes setting up all the pins and initializing
                // the SD card, it's a big function.
                if (!FSInit()) {
                    ERROR_UNTIL_REMOVAL();
                    LATAbits.LATA3 = 1; // Make sure we turn back on the red LED, as it may have
                                        // been turned off by the ERROR_UNTIL_REMOVAL() macro.
                    continue;
                }

                // Currently only systems with a 512b sector size are supported so error out if
                // that's not what we find.
                if (GetSectorSize() != BYTES_PER_SECTOR) {
                    ERROR_UNTIL_REMOVAL();
                }

                // Open a new log file, attemping to select a number based on what's on the SD card
                uint16_t lastLogFileNumber = GetLastLogNumberFromCard();
                if (OpenNewLogFile(lastLogFileNumber) == INVALID_LOG_NUMBER) {
                    ERROR_UNTIL_REMOVAL();
                    LATAbits.LATA3 = 1; // Make sure we turn back on the red LED, as it may have
                                        // been turned off by the ERROR_UNTIL_REMOVAL() macro.
                    continue;
                }

                // Reset our timer since we opened a new log file
                timerCounter = 0.0;

                // Reset the event tracker for the most buffer usage
                biggerCircBufSize = 0;

                // Read the configuration from the SD card
                ConfigParams params;
                if (!ProcessConfigFile(&params)) {
                    ERROR_UNTIL_REMOVAL();
                    LATAbits.LATA3 = 1; // Make sure we turn back on the red LED, as it may have
                                        // been turned off by the ERROR_UNTIL_REMOVAL() macro.
                    continue;
                }

                // Make sure that at least one source is enabled
                if (params.canBaudRate == 0 &&
                    (params.uart1Input == UART_SRC_NONE || params.uart1BaudRate == 0) &&
                    (params.uart2Input == UART_SRC_NONE || params.uart2BaudRate == 0)) {
                    ERROR_UNTIL_REMOVAL();
                    LATAbits.LATA3 = 1; // Make sure we turn back on the red LED, as it may have
                                        // been turned off by the ERROR_UNTIL_REMOVAL() macro.
                    continue;
                }

                // Check for the reason that logging has started
                if (RCONbits.POR) {
                    LogMetaEvent("Logging after power on.", (uint32_t)timerCounter);
                } else if (RCONbits.BOR) {
                    LogMetaEvent("Logging after brown-out.", (uint32_t)timerCounter);
                } else if (RCONbits.EXTR) {
                    LogMetaEvent("Logging after being manually reset.", (uint32_t)timerCounter);
                } else if (RCON) {
                    LogMetaEvent("Logging after unknown event.", (uint32_t)timerCounter);
                } else {
                    LogMetaEvent("Logging after card insertion.", (uint32_t)timerCounter);
                }
                RCON = 0; // And clear all the status bits as we're done with them at this point

                // Initialize the ECAN if it's been enabled in the config file
                if (params.canBaudRate > 0) {

                    // Enable ECAN1 pins: TX on B7, RX on B4
                    PPSUnLock;
                    PPSOutput(OUT_FN_PPS_C1TX, OUT_PIN_PPS_RP39);
                    PPSInput(IN_FN_PPS_C1RX, IN_PIN_PPS_RP20);
                    PPSLock;

                    // Set the TRIS bits properly for ECAN1 as well.
                    _TRISB7 = 0;
                    _TRISB4 = 1;

                    // Update our internal state of the peripherals
                    activePeripherals |= PERIPHERAL_ECAN;

                    // Log the configuration info used when starting.
                    char x[] = "Started logging CAN data at \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
                    size_t xLen = strlen(x);
                    ultoa(&x[xLen], params.canBaudRate, 10);
                    xLen = strlen(x);
                    strcpy(&x[xLen], " baud.");
                    LogMetaEvent(x, (uint32_t)timerCounter);
                }

                // And configure UART2
                if (params.uart2BaudRate > 0 && params.uart2Input != UART_SRC_NONE) {

                    // Set up the correct UART pins based on the selected connector. No need to set
                    // TRIS bits, because they default to INPUT.
                    PPSUnLock;
                    switch (params.uart2Input) {
                        case UART_SRC_BUILTIN_RECEIVE: // B13
                            PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RPI45);
                            _TRISB13 = 1;
                            break;
                        case UART_SRC_CONN1_TRANSMIT: // B15
                            PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RPI47);
                            _TRISB15 = 1;
                            break;
                        case UART_SRC_CONN1_RECEIVE: // B12
                            PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RPI44);
                            _TRISB12 = 1;
                            break;
                        case UART_SRC_CONN2_TRANSMIT: // B1
                            PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RPI33);
                            _TRISB1 = 1;
                            break;
                        case UART_SRC_CONN2_RECEIVE: // B10
                            PPSInput(IN_FN_PPS_U2RX, IN_PIN_PPS_RP42);
                            _TRISB10 = 1;
                            break;
                        default:
                            ERROR_UNTIL_REMOVAL();
                            LATAbits.LATA3 = 1; // Make sure we turn back on the red LED, as it may
                                                // have been turned off by the ERROR_UNTIL_REMOVAL()
                                                // macro.
                    }
                    PPSLock;

                    // Update our internal state of the peripherals
                    activePeripherals |= PERIPHERAL_UART2;

                    // Log the configuration info used when starting.
                    char x[] = "Started logging from UART2 on \0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
                    size_t xLen = strlen(x);
                    switch (params.uart2Input) {
                    case UART_SRC_BUILTIN_RECEIVE:
                        strcpy(&x[xLen], "BUILTIN_RX");
                        break;
                    case UART_SRC_CONN1_RECEIVE:
                        strcpy(&x[xLen], "CONN1_RX");
                        break;
                    case UART_SRC_CONN1_TRANSMIT:
                        strcpy(&x[xLen], "CONN1_TX");
                        break;
                    case UART_SRC_CONN2_RECEIVE:
                        strcpy(&x[xLen], "CONN2_RX");
                        break;
                    case UART_SRC_CONN2_TRANSMIT:
                        strcpy(&x[xLen], "CONN2_TX");
                        break;
                    default:
                        strcpy(&x[xLen], "UNKNOWN");
                        break;
                    }
                    xLen = strlen(x);
                    strcpy(&x[xLen], " at ");
                    xLen = strlen(x);
                    ultoa(&x[xLen], params.uart2BaudRate, 10);
                    xLen = strlen(x);
                    strcpy(&x[xLen], " baud.");
                    LogMetaEvent(x, (uint32_t)timerCounter);

                    // And initialize the UART peripheral. We do this last to prevent filling up the
                    // buffers while the above metadata is being written to the card.
                    Uart2Init(params.uart2BaudRate, Uart2InterruptRoutine);
                }

                // Update status, including turning off the red LED
                sdConnected = true;
                LATAbits.LATA3 = 0;
            }

            // When we are connected and initialized, poll the buffer, if there
            // is data, write it.
            if (CB_PeekMany(&circBuf, tempSector.sectorFormat.data, UART2_BUFFER_SIZE)) {

                // Log the size of the circular buffer if it's grown at all. We do this before writing
                // out the current sector because we want it to trigger for the 1st time even if the
                // buffer is never used beyond 1 sector.
                if (circBuf.dataSize > biggerCircBufSize) {
                    char x[] = "Buffer usage hit \0\0\0\0\0\0\0\0";
                    size_t xLen = strlen(x);
                    ultoa(&x[xLen], circBuf.dataSize / DATA_PER_SECTOR, 10);
                    xLen = strlen(x);
                    strcpy(&x[xLen], " sectors.");
                    LogMetaEvent(x, (uint32_t)timerCounter);
                    biggerCircBufSize = circBuf.dataSize;
                }

                // Try to write the data, removing it from the buffer if we succeeded. If we don't
                // succeed, we just continue, which will try to write the sector again as it'll
                // remain in the buffer.
                if (NewSDWriteSector(&tempSector)) {
                    CB_Remove(&circBuf, UART2_BUFFER_SIZE);
                }
            }
        }
        // Otherwise, if the card has become disconnected, show the red error LED and disable
        // peripherals.
        else if (sdConnected) {
            // Turn on the red LED
            LATAbits.LATA3 = 1;

            // Update state to indicate there's no SD card available
            sdConnected = false;

            // Disable peripherals. Any mapped pins will be remapped on card re-insertion, so no
            // need to unmap them here.
            if (activePeripherals & PERIPHERAL_UART2) {
                Uart2Disable();

                activePeripherals &= ~PERIPHERAL_UART2;
            }
        }
    }
}

/**
 * Initialize input/output pins for the processor.
 */
static void InitPins(void)
{
    // And configure the Peripheral Pin Select pins to enable SPI2: clock (B9), out (B8), in (B14)
    PPSUnLock;
    PPSOutput(OUT_FN_PPS_SCK2, OUT_PIN_PPS_RP41);
    PPSOutput(OUT_FN_PPS_SDO2, OUT_PIN_PPS_RP40);
    PPSInput(IN_FN_PPS_SDI2, IN_PIN_PPS_RPI46);
    PPSLock;

    // And enable the TRIS pins for SPI2
    _TRISB9 = 0;
    _TRISB8 = 0;
    _TRISB14 = 1;

    // And enable both the LED pins as outputs
    _TRISA3 = 0; // Red LED
    _TRISA4 = 0; // Amber LED

    // Disable A/D functions on all pins
    ANSELA = 0;
    ANSELB = 0;
}

static void Uart2InterruptRoutine(unsigned char *Buffer, int BufferSize)
{
    // Turn off the amber LED, indicating that we're receiving data
    receivingData = true;
    timeSinceLastData = 0;

    // Write this data to our circular buffer. We use the fail early mode to make
    // sure the buffer is always only full of an integer number of sectors
    CB_WriteMany(&circBuf, Buffer, BufferSize, true);
}

void _ISR _T2Interrupt(void)
{
    // Track the uptime of the system. Used for logging.
    timerCounter += 0.1;

    // See if we haven't received new data in a while and update the system state accordingly.
    if (timeSinceLastData >= DATA_TIMEOUT) {
        receivingData = false;
    } else {
        ++timeSinceLastData;
    }

    // Blink the amber status LED
    SetStatusModeLed();

    // Clear the interrupt flag
    IFS0bits.T2IF = 0;
}

/**
 * Blink the status LED at 1Hz when on and initialized and at 2Hz when receiving data. This function
 * assumes it's being called at 10Hz.
 */
void SetStatusModeLed(void)
{
    // Keep a variable here for scaling the 4Hz timer to a 1Hz timer.
    static uint8_t timerCounter = 0;

    // Check if it's time to toggle the status LED. The limit is decided based on whether HIL is
    // active and if the rudder is detected.
    uint8_t countLimit;
    if (sdConnected && receivingData) {
        countLimit = 4; // Set to 2Hz
    } else {
        countLimit = 9; // Set to 1Hz
    }
    if (++timerCounter >= countLimit) {
        _LATA4 ^= 1;
        timerCounter = 0;
    }
}
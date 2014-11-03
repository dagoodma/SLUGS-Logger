#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

#include <stdint.h>
#include <stdbool.h>

#include "Libs/Microchip/Include/MDD File System/FSIO.h"

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L
#define HEAD_FOOT_LEN 6
#define HEADER_TAG 0x5E25 // '%^'
#define FOOTER_TAG 0x2425 // '%$'
#define EE_ADDRESS 0x01

// a struct to combine data with a header and footer
typedef struct {
    uint16_t headerTag;
    uint8_t number;
    uint8_t data[506];
    uint8_t checksum;
    uint16_t footerTag;
} SectorFormat;

// this union makes the SectorFormat usable as a simple array
typedef union {
    SectorFormat sectorFormat;
    uint8_t raw[sizeof(SectorFormat)];
} Sector;

/**
 * These are possible sources for the UART input data streams.
 * @see ConfigParams
 */
typedef enum {
    UART_SRC_NONE,
    UART_SRC_BUILTIN_TRANSMIT,
    UART_SRC_BUILTIN_RECEIVE,
    UART_SRC_CONN1_TRANSMIT,
    UART_SRC_CONN1_RECEIVE,
    UART_SRC_CONN2_TRANSMIT,
    UART_SRC_CONN2_RECEIVE
} UartSource;

/**
 * A struct of configuration options.
 * @see ProcessConfigFile
 */
typedef struct {
    uint32_t   uart1BaudRate;
    UartSource uart1Input;
    uint32_t   uart2BaudRate;
    UartSource uart2Input;
    uint32_t   canBaudRate;
} ConfigParams;

/**
 * Initalizes filesystems and returns a file structure for use with
 * NewSDWriteSector.
 * @param filename A string with the filename (name.ext)
 * @return Whether opening the log file succeeded or failed
 */
bool OpenNewLogFile();

/**
 * Process the configuration file on the current SD card. Assumes all hardware is
 * initialized and the filesystem is ready for reading
 * @param params The struct to return the parameter values into.
 * @return False if the configuration file was invalid, true otherwise.
 */
bool ProcessConfigFile(ConfigParams *params);

/**
 * Writes the data in outbuf to the file (pointer)
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in length)
 * @return True if successful, false otherwise
 */
bool NewFileUpdate(FSFILE *pointer);

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 * @return True if successful, false otherwise
 */
bool NewSDWriteSector(Sector *sector);

#endif
#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

#include <stdint.h>
#include <stdbool.h>

#include "Libs/Microchip/Include/MDD File System/FSIO.h"

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L
#define HEAD_FOOT_LEN 6
#define DATA_PER_SECTOR (BYTES_PER_SECTOR-HEAD_FOOT_LEN)
#define HEADER_TAG 0x5E25 // '%^'
#define FOOTER_TAG 0x2425 // '%$'
#define EE_ADDRESS 0x01

// The largest UINT16 number is reserved as an invalid log number, mostly for easy use with the EEPROM.
#define INVALID_LOG_NUMBER (UINT16_MAX)

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
 * Opens a new log/meta file pair with an appropriate file number.
 *
 * @param lastFileNumber The last file number used.
 * @return The new log file number or INVALID_LOG_NUMBER if error.
 */
uint16_t OpenNewLogFile(uint16_t lastFileNumber);

/**
 * Returns the last log number used in the EEPROM.
 *
 * @returns The last log number or INVALID_LOG_NUMBER if error or not found.
 */
uint16_t GetLastLogNumberFromEeprom(void);

/**
 * Returns the largest log number that's in the root directory of the card.
 *
 * @return The biggest log number on the card or INVALID_LOG_NUMBER if none are found.
 */
uint16_t GetLastLogNumberFromCard(void);

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

/**
 * Logs a string to the metadata file.
 * @param eventString The string to log to the metadata file.
 * @param timestamp A timestamp indicating the time of the event. Seconds since the start of the logfile.
 * @return True if the entire string was written.
 */
bool LogMetaEvent(const char *eventString, uint32_t timestamp);

/**
 * Convert a uint16 integer to a 4-character NUL-terminated string.
 * @see HexToUint16
 *
 * @param in The uint16 to convert
 * @param out An output 4-character string, NUL-terminated
 */
void Uint16ToHex(uint16_t in, char out[5]);

/**
 * Convert a 4-character hex string (no NUL-terminator required) to a uint16 integer.
 * @see Uint16ToHex
 *
 * @param in The 4 hex characters.
 * @param out The converted number. Output is undetermined if conversion fails.
 * @return If the string was successfully converted
 */
bool HexToUint16(const char in[4], uint16_t *out);

#endif
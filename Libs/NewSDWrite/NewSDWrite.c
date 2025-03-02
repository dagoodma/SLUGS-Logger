#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <spi.h>
#include <xc.h>

#include "NewSDWrite.h"
#include "Node.h"
#include "Uart2.h"

#include "Microchip/Include/MDD File System/FSIO.h"

// The configuration file cannot be bigger than this (in bytes)
#define MAX_CONFIG_FILE_SIZE 1024

// Specify how many clusters should be allocated at a time. Clusters default to 32KiB.
#define MULTIPLE_CLUSTERS 4

/** Rip some datatypes and function definitions from FSIO.c **/
// Directory entry structure
typedef struct {
    char DIR_Name[DIR_NAMESIZE]; // File name
    char DIR_Extension[DIR_EXTENSION]; // File extension
    BYTE DIR_Attr; // File attributes
    BYTE DIR_NTRes; // Reserved byte
    BYTE DIR_CrtTimeTenth; // Create time (millisecond field)
    WORD DIR_CrtTime; // Create time (second, minute, hour field)
    WORD DIR_CrtDate; // Create date
    WORD DIR_LstAccDate; // Last access date
    WORD DIR_FstClusHI; // High word of the entry's first cluster number
    WORD DIR_WrtTime; // Last update time
    WORD DIR_WrtDate; // Last update date
    WORD DIR_FstClusLO; // Low word of the entry's first cluster number
    DWORD DIR_FileSize; // The 32-bit file size
} _DIRENTRY;
typedef _DIRENTRY * DIRENTRY; // A pointer to a directory entry structure
typedef FSFILE * FILEOBJ;
BYTE FILEallocate_new_cluster(FSFILE *fo, BYTE mode);
DWORD Cluster2Sector(DISK *, DWORD);
DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle);
DWORD WriteFAT(DISK *dsk, DWORD ccls, DWORD value, BYTE forceWrite);
BYTE Write_File_Entry(FILEOBJ fo, WORD *curEntry);
BYTE flushData (void);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;
extern DISK gDiskData; // A global datastore for storing disk information. Populated by FSInit().

// Internal functions
static uint8_t Checksum(uint8_t *data, int dataSize);
static bool NewAllocateMultiple(FSFILE *fo);
void CloseLogFile(void);
bool Hex2Nibble(char in, uint8_t *out);

static FSFILE *metaFilePointer = NULL; // A pointer to the current log file
static FSFILE *logFilePointer = NULL; // A pointer to the current log file
static DWORD lastCluster; // The last cluster number used for the current log file
static uint16_t fileNumber; // The current log number file. Stored in EEPROM.

/**
 * Opens the config file, extracts info, searches for a new filename to use, opens the file for
 * writing. This function assumes that the chip, SD card, and file system are all
 * initialized and the file is ready to be read.
 *
 * TODO: Find previous file used, update it to actual size
 * TODO: Allocate appropriate file size for new entry
 * @return Buad rate extracted from the config file or 0 if invalid
 */
uint16_t OpenNewLogFile(uint16_t lastFileNumber)
{
    // If either the metadata or log file are already open, close them before opening new ones.
    CloseLogFile();

    // Save the file number we're using. We make sure to skip 0xFFFF, as that's an invalid number in
    // the EEPROM, and we wouldn't be able to differentiate from a good log file with that name and
    // no saved log file.
    if (lastFileNumber == INVALID_LOG_NUMBER || lastFileNumber == INVALID_LOG_NUMBER - 1) {
        fileNumber = 0;
    } else {
        fileNumber = lastFileNumber + 1;
    }

    // Open a new meta file
    char metaFileName[] = "0000.meta";
    Uint16ToHex(fileNumber, metaFileName);
    metaFileName[4] = '.'; // Re-add the period that was overwritten by the NUL-terminating character in Uint16ToHex()
    metaFilePointer = FSfopen(metaFileName, FS_WRITE);
    if (!metaFilePointer) {
        return INVALID_LOG_NUMBER;
    }

    // Open a new log file
    char logFileName[] = "0000.log";
    Uint16ToHex(fileNumber, logFileName);
    logFileName[4] = '.'; // Re-add the period that was overwritten by the NUL-terminating character in Uint16ToHex()
    logFilePointer = FSfopen(logFileName, FS_WRITE);
    if (!logFilePointer) {
        return INVALID_LOG_NUMBER;
    }

    // Initialize data for NewSDWriteSector
    logFilePointer->ccls = logFilePointer->cluster;

    // Allocate some clusters
    NewAllocateMultiple(logFilePointer);

    return fileNumber;
}

uint16_t GetLastLogNumberFromCard(void)
{
    SearchRec r;
    if (FindFirst("????.log", ATTR_MASK, &r) == 0) {
        uint16_t lastLog = 0x0000;
        do {
            // Decode the current filename into a log number, saving it if it's the largest found.
            uint16_t logNum;
            if (HexToUint16(r.filename, &logNum)) {
                if (logNum > lastLog) {
                    lastLog = logNum;
                }
            }

        } while (FindNext(&r) == 0);

        return lastLog;
    } else {
        return INVALID_LOG_NUMBER;
    }
}

/**
 * Close the log and metadata file. This should be done before calling OpenNewLogFile() again to
 * prevent running out of file descriptors.
 */
void CloseLogFile(void)
{
    if (metaFilePointer) {
        FSfclose(metaFilePointer);
        metaFilePointer = NULL;
    }
    if (logFilePointer) {
        FSfclose(logFilePointer);
        logFilePointer = NULL;
    }
}

bool ProcessConfigFile(ConfigParams *params)
{
    // Open the configuration file read-only
    FSFILE *configFile = FSfopen("CONFIG.TXT", FS_READ);
    if (!configFile) {
        return false;
    }

    // If the config file is empty or huge (1k), we assume the file is invalid.
    if (configFile->size == 0 || configFile->size > MAX_CONFIG_FILE_SIZE) {
        FSfclose(configFile);
        return false;
    }

    // Grab the entire file into an array for processing.
    char fileText[configFile->size + 2];
    const size_t bytesRead = FSfread(fileText, sizeof(char), configFile->size, configFile);
    FSfclose(configFile); // No need to keep the file open any longer
    fileText[bytesRead] = '\n'; // Make sure it's newline-terminated
    fileText[bytesRead + 1] = '\0'; // And also a proper C-style string

    // Fill the params struct with nice defaults
    params->uart1BaudRate = 0;
    params->uart1Input = UART_SRC_NONE;
    params->uart2BaudRate = 0;
    params->uart2Input = UART_SRC_NONE;
    params->canBaudRate = 0;

    // We track the start and end of each line and iterate until we run off the end of it.
    char *endOfLine = strchr(fileText, '\n');
    char *startOfLine = fileText;
    while (startOfLine < &fileText[bytesRead]) {
        // Convert Windows line endings to Unix ones
        if (*(endOfLine - 1) == '\r') {
            *(endOfLine - 1) = '\n';
        }

        // Grab the parameter name and value
        char *param = strtok(startOfLine, " \t");
        char *value = strtok(NULL, "\n");
        if (param && value) {
            // Now that we have a parameter/value pair, process them into the
            // output configuration parameter struct.
            if (strcmp(param, "UART1_INPUT") == 0) {
                if (strcmp(value, "BUILTIN_RX") == 0) {
                    params->uart1Input = UART_SRC_BUILTIN_RECEIVE;
                } else if (strcmp(value, "CONN1_TX") == 0) {
                    params->uart1Input = UART_SRC_CONN1_TRANSMIT;
                } else if (strcmp(value, "CONN1_RX") == 0) {
                    params->uart1Input = UART_SRC_CONN1_RECEIVE;
                } else if (strcmp(value, "CONN2_TX") == 0) {
                    params->uart1Input = UART_SRC_CONN2_TRANSMIT;
                } else if (strcmp(value, "CONN2_RX") == 0) {
                    params->uart1Input = UART_SRC_CONN2_RECEIVE;
                } else {
                    return false;
                }
            } else if (strcmp(param, "UART2_INPUT") == 0) {
                if (strcmp(value, "BUILTIN_RX") == 0) {
                    params->uart2Input = UART_SRC_BUILTIN_RECEIVE;
                } else if (strcmp(value, "CONN1_TX") == 0) {
                    params->uart2Input = UART_SRC_CONN1_TRANSMIT;
                } else if (strcmp(value, "CONN1_RX") == 0) {
                    params->uart2Input = UART_SRC_CONN1_RECEIVE;
                } else if (strcmp(value, "CONN2_TX") == 0) {
                    params->uart2Input = UART_SRC_CONN2_TRANSMIT;
                } else if (strcmp(value, "CONN2_RX") == 0) {
                    params->uart2Input = UART_SRC_CONN2_RECEIVE;
                } else {
                    return false;
                }
            } else if (strcmp(param, "UART1_BAUD") == 0) {
                params->uart1BaudRate = strtoul(value, NULL, 10);
                if (!params->uart1BaudRate) {
                    return false;
                }
            } else if (strcmp(param, "UART2_BAUD") == 0) {
                params->uart2BaudRate = strtoul(value, NULL, 10);
                if (!params->uart2BaudRate) {
                    return false;
                }
            } else if (strcmp(param, "CAN_BAUD") == 0) {
                params->canBaudRate = strtoul(value, NULL, 10);
                if (!params->canBaudRate) {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }

        // And continue on to the next line
        startOfLine = endOfLine + 1;
        endOfLine = strchr(startOfLine, '\n');
    }

    return true;
}

/**
 * Appends the given data to the end of the file. Additionally, if there's no more room in the file,
 * allocate another several clusters worth of space.
 *
 * @param sector A complete sector of data to write.
 * @return True if successful
 */
bool NewSDWriteSector(Sector *sector)
{
    const DWORD currentSector = Cluster2Sector(logFilePointer->dsk, logFilePointer->ccls) +
        logFilePointer->sec;
    const DWORD sectorLimit = Cluster2Sector(logFilePointer->dsk, logFilePointer->ccls) +
        logFilePointer->dsk->SecPerClus;

    // add header and footer
    sector->sectorFormat.headerTag = HEADER_TAG;
    sector->sectorFormat.number = fileNumber; // need to figure out how to number these
    sector->sectorFormat.checksum = Checksum(sector->sectorFormat.data,
            sizeof (sector->sectorFormat.data));
    sector->sectorFormat.footerTag = FOOTER_TAG;

    // Write the data
    const bool success = MDD_SDSPI_SectorWrite(currentSector, sector->raw, false);
    if (!success) {
        return false;
    }

    // Check to see if we need to go to a new cluster. Also check for the end of the allocated area
    // allocating another cluster if necessary.
    if (currentSector == sectorLimit - 1) {
        // if this is the last cluster, allocate more
        if (logFilePointer->ccls == lastCluster) {
            if (!NewAllocateMultiple(logFilePointer)) {
                return false;
            }
        }
        // Set cluster and sector to next cluster in our chain
        if (FILEget_next_cluster(logFilePointer, 1) != CE_GOOD) {
            return false;
        }
        logFilePointer->sec = 0;
    } else {
        logFilePointer->sec++;
    }
    // save off the positon
    logFilePointer->pos = BYTES_PER_SECTOR - 1; // current position in sector (bytes)

    // save off the seek
    logFilePointer->seek += BYTES_PER_SECTOR; // current position in file (bytes)

    return true;
}

bool LogMetaEvent(const char *eventString, uint32_t timestamp)
{
    char x[128] = {};
    ultoa(x, timestamp, 10);
    size_t xLen = strlen(x);
    x[xLen++] = ':';
    x[xLen++] = ' ';
    strcpy(&x[xLen], eventString);
    xLen = strlen(x);
    x[xLen++] = '\n';

    // Write the string to the file and make sure to flush it to the SD card.
    const size_t bytesWritten = FSfwrite(x, sizeof(char), xLen, metaFilePointer);
    flushData();
    gNeedFATWrite = TRUE;
    NewFileUpdate(metaFilePointer);
    if (bytesWritten < xLen) {
       return false;
    }

    return true;
}

/**
 * Allocates multiple clusters to the given file.
 * @param fo Pointer to the file object to allocate to
 * @return Whether it was successful or not
 */
bool NewAllocateMultiple(FSFILE *fo)
{
    // Save the current cluster of the file
    const DWORD clusterSave = fo->ccls;

    // Allocate several new clusters
    uint8_t i;
    for (i = 0; i < MULTIPLE_CLUSTERS; i++) {
        FILEallocate_new_cluster(fo, 0);
    }

    // store the last cluster in the file
    lastCluster = fo->ccls;

    // reset current cluster
    fo->ccls = clusterSave;

    // update file size
    fo->size += fo->dsk->SecPerClus * MULTIPLE_CLUSTERS * BYTES_PER_SECTOR;

    // Save all this to the card. We don't need to write any data, just update the FAT table, so
    // we have to set some internal variables for the FSIO library to get this to work.
    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    if (NewFileUpdate(logFilePointer)) { // put NewFileUpdates into NewAllocateMultiple
        return true;
    } else {
        return false;
    }
}

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 * @return If it was successful
 */
bool NewFileUpdate(FSFILE *fo)
{
    if (fo == NULL) {
        return 0;
    }
    WORD fHandle = fo->entry;

    // Write the file data
    WriteFAT(fo->dsk, 0, 0, TRUE);

    // Update file entry data
    DIRENTRY dir = LoadDirAttrib(fo, &fHandle);
    if (dir == NULL) {
        return false;
    }
    dir->DIR_FileSize = fo->size;
    dir->DIR_Attr = fo->attributes;
    dir->DIR_FstClusHI = (fo->cluster & 0xFFFF0000) >> 16;
    dir->DIR_FstClusLO = fo->cluster & 0x0000FFFF;

    Write_File_Entry(fo, &fHandle);

    return true;
}

DWORD GetSectorSize(void)
{
    return gDiskData.sectorSize;
}

/**
 * Creates a two byte checksum - used for the sector checksum
 * return format : [odds][evens]
 *                 15          0
 * @param data A pointer to the data to checksum
 * @param dataSize The size of the data
 * @return the checksum
 */
uint8_t Checksum(uint8_t * data, int dataSize)
{
    int i;
    uint8_t sum = 0;
    for (i = 0; i < dataSize; i++) {
        sum ^= data[i];
    }
    return sum;
}

/**
 * Convert an unsigned integer to a 4-character hexadecimal ASCII string.
 * @param x The number to convert
 * @param out[out] The output string.
 */
void Uint16ToHex(uint16_t x, char out[5])
{
   const char hexDigits[] = "0123456789ABCDEF";
   out[3] = hexDigits[x & 0xF];
   out[2] = hexDigits[(x >> 4) & 0xF];
   out[1] = hexDigits[(x >> 8) & 0xF];
   out[0] = hexDigits[(x >> 12) & 0xF];
   out[4] = '\0';
}

bool Hex2Nibble(char in, uint8_t *out)
{
    if ('a' <= in && in <= 'f') {
        *out = in - 'a' + 0xA;
        return true;
    } else if ('A' <= in && in <= 'F') {
        *out = in - 'A' + 0xA;
        return true;
    } else if ('0' <= in && in <= '9') {
        *out = in - '0';
        return true;
    }

    return false;
}

bool HexToUint16(const char in[4], uint16_t *out)
{
    *out = 0x0000;
    uint8_t nibble;

    if (Hex2Nibble(in[3], &nibble)) {
        *out |= nibble;
    } else {
        return false;
    }
    if (Hex2Nibble(in[2], &nibble)) {
        *out |= nibble << 4;
    } else {
        return false;
    }
    if (Hex2Nibble(in[1], &nibble)) {
        *out |= nibble << 8;
    } else {
        return false;
    }
    if (Hex2Nibble(in[0], &nibble)) {
        *out |= nibble << 12;
    } else {
        return false;
    }

    return true;
}
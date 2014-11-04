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

#include "DEE/DEE Emulation 16-bit.h"
#include "Microchip/Include/MDD File System/FSIO.h"

// The configuration file cannot be bigger than this (in bytes)
#define MAX_CONFIG_FILE_SIZE 1024

#define EIGHT_THREE_LEN (8 + 1 + 3 + 1)
#define MULTIPLE_CLUSTERS 0x04

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
BYTE Write_File_Entry(FILEOBJ fo, WORD * curEntry);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;

static uint8_t Checksum(uint8_t * data, int dataSize);
static bool NewAllocateMultiple(FSFILE * fo);

FSFILE * filePointer;
DWORD lastCluster;
unsigned int fileNumber;

/**
 * Opens the config file, extracts info, searches for a new filename to use, opens the file for
 * writing. This function assumes that the chip, SD card, and file system are all
 * initialized and the file is ready to be read.
 *
 * TODO: Find previous file used, update it to actual size
 * TODO: Allocate appropriate file size for new entry
 * @return Buad rate extracted from the config file or 0 if invalid
 */
bool OpenNewLogFile(void)
{
    // Read EEPROM to find next file name
    while (1) {
        fileNumber = DataEERead(EE_ADDRESS);
        if (fileNumber != 0xFFFF) {
            break;
        }
        DataEEWrite(0x00, EE_ADDRESS);
    }
    // TODO CORRECT THE LAST FILE SIZE HERE
    DataEEWrite(++fileNumber, EE_ADDRESS);

    // Open a new file
    char fileName[EIGHT_THREE_LEN];
    sprintf(fileName, "%04x.log", fileNumber);
    filePointer = FSfopen(fileName, FS_WRITE);
    if (!filePointer) {
        return false;
    }

    // Initialize data for NewSDWriteSector
    filePointer->ccls = filePointer->cluster;

    // allocate some clusters
    NewAllocateMultiple(filePointer);

    return true;
}

bool ProcessConfigFile(ConfigParams *params)
{
    // Open the configuration file read-only
    FSFILE *configFile = FSfopen("CONFIG.TXT", FS_READ);
    if (!configFile) {
        return false;
    }

    // Fill the params struct with nice defaults
    params->uart1BaudRate = 0;
    params->uart1Input = UART_SRC_NONE;
    params->uart2BaudRate = 0;
    params->uart2Input = UART_SRC_NONE;
    params->canBaudRate = 0;

    // If the config file is empty or huge (1k), we assume the file is invalid.
    if (configFile->size == 0 || configFile->size > MAX_CONFIG_FILE_SIZE) {
        return false;
    }

    // Grab the entire file into an array for processing.
    char fileText[configFile->size + 2];
    size_t bytesRead = FSfread(fileText, sizeof(char), configFile->size, configFile);
    fileText[bytesRead + 1] = '\n'; // Make sure it's newline-terminated
    fileText[bytesRead + 2] = '\0'; // And also a proper C-style string
    
    // Locate the end of the line if there is one.
    char *endOfLine = strchr(fileText, '\n');
    char *startOfLine = fileText;
    while (endOfLine) {
        // Null-terminate this line, making sure to account for Windows line endings
        if (*(endOfLine - 1) == '\r') {
            *endOfLine = '\0';
        }
        *endOfLine = '\0';

        // Grab the parameter name and value
        char *param = strtok(startOfLine, " \t");
        char *value = strtok(NULL, " \t");
        if (param && value) {
            // Now that we have a parameter/value pair, process them into the
            // output configuration parameter struct.
            if (strcmp(param, "UART1_INPUT") == 0) {
                if (strcmp(value, "BUILTIN_TX") == 0) {
                    params->uart1Input = UART_SRC_BUILTIN_TRANSMIT;
                } else if (strcmp(value, "BUILTIN_RX") == 0) {
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
                if (strcmp(value, "BUILTIN_TX") == 0) {
                    params->uart2Input = UART_SRC_BUILTIN_TRANSMIT;
                } else if (strcmp(value, "BUILTIN_RX") == 0) {
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
    }

    return true;
}

/**
 * Writes the data in outbuf to the file (pointer)
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return True if successful
 */
bool NewSDWriteSector(Sector *sector)
{
    DWORD CurrentSector = Cluster2Sector(filePointer->dsk, filePointer->ccls)
            + filePointer->sec;
    DWORD SectorLimit = Cluster2Sector(filePointer->dsk, filePointer->ccls)
            + filePointer->dsk->SecPerClus;

    // add header and footer
    sector->sectorFormat.headerTag = HEADER_TAG;
    sector->sectorFormat.number = fileNumber; // need to figure out how to number these
    sector->sectorFormat.checksum = Checksum(sector->sectorFormat.data,
            sizeof (sector->sectorFormat.data));
    sector->sectorFormat.footerTag = FOOTER_TAG;

    // Write the data
    int success = MDD_SDSPI_SectorWrite(CurrentSector, sector->raw, false);
    if (!success) {
        return false;
    }

    // Check to see if we need to go to a new cluster; !! Also check for end of allocated area
    // else, next sector
    if (CurrentSector == SectorLimit - 1) {
        // if this is the last cluster, allocate more
        if (filePointer->ccls == lastCluster) {
            if (!NewAllocateMultiple(filePointer)) {
                return false;
            }
        }
        // Set cluster and sector to next cluster in our chain
        if (FILEget_next_cluster(filePointer, 1) != CE_GOOD) {
            return false;
        }
        filePointer->sec = 0;
    } else {
        filePointer->sec++;
    }
    // save off the positon
    filePointer->pos = BYTES_PER_SECTOR - 1; // current position in sector (bytes)

    // save off the seek
    filePointer->seek += BYTES_PER_SECTOR; // current position in file (bytes)

    return true;
}

/**
 * Allocates multiple clusters to the given file. !! UNTESTED
 * @param fo Pointer to the file object to allocate to
 * @return Whether it was successful or not
 */
bool NewAllocateMultiple(FSFILE *fo)
{
    // save the current cluster of the file
    DWORD clusterSave = fo->ccls;

    // allocate new clusters
    int i;
    for (i = 0; i < MULTIPLE_CLUSTERS; i++) {
        FILEallocate_new_cluster(fo, 0);
    }

    // store the last cluster in the file
    lastCluster = fo->ccls;

    // reset current cluster
    fo->ccls = clusterSave;

    // update file size
    fo->size += fo->dsk->SecPerClus * MULTIPLE_CLUSTERS * BYTES_PER_SECTOR;

    // save all this to the card
    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    if (NewFileUpdate(filePointer)) { // put NewFileUpdates into NewAllocateMultiple
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
bool NewFileUpdate(FSFILE * fo)
{
    if (fo == NULL) {
        return 0;
    }
    DIRENTRY dir;
    WORD fHandle = fo->entry;

    // Write the file data
    WriteFAT(fo->dsk, 0, 0, TRUE);

    // Update file entry data
    dir = LoadDirAttrib(fo, &fHandle);
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

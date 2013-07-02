#include <xc.h>
#include <stdint.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include <spi.h>
#include <math.h>
#include "Uart2.h"
#include <stdio.h>
#include "Node.h"
#include <string.h>
#include <stdbool.h>
#include "Uart2.h"
#include <stddef.h>
#include "NewSDWrite.h"
#include "DEE Emulation 16-bit.h"

#define CONFIG_READ_SIZE 50
#define MAX_PREFIX "5"
#define MAX_SUFFIX "3"
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
int utf16toStr(unsigned short int * origin, char * result, int number);
uint16_t twoByteChecksum(uint8_t * data, int dataSize);
int NewAllocateMultiple(FSFILE * fo);

FSFILE * filePointer;
DWORD lastCluster;

/**
 * Opens the config file, extracts info, searches for a new filename to use, opens the file for
 * writing
 * NEEDED FUNCTIONALITY:
 *  Find previous file used, update it to actual size
 *  Allocate apporpriate file size for new entry
 * @return Buad rate extracted from the config file
 */
long int NewSDInit(void)
{
    // used for accessing config file
    FSFILE *configFile = NULL;
    char configText[CONFIG_READ_SIZE + 1];

    // extracted from the config
    long int baudRate;
    
    // the final file name
    char fileName[EIGHT_THREE_LEN] = {};

    while (!MDD_MediaDetect()); // TODO make this smarter
    while (!FSInit());

    // Open then read the config file, null terminate the config text string
    while (configFile == NULL) configFile = FSfopen("CONFIG.TXT", FS_READPLUS); // open the file
    FSfread(configText, 1, CONFIG_READ_SIZE, configFile);
    FSrewind(configFile);
    configText[CONFIG_READ_SIZE] = '\0';

    // extract config info
    if (sscanf(configText, "BAUD %ld", &baudRate) < 1) {
        FATAL_ERROR(); // TODO have default config file? maybe not
    }

    // Read EEPROM to find next file name
    unsigned int eeRead;
    while(1) {
        eeRead = DataEERead(EE_ADDRESS);
        if (eeRead != 0xFFFF) {
            break;
        }
        DataEEWrite(0x00, EE_ADDRESS);
    }
    // !! CORRECT THE LAST FILE SIZE HERE
    DataEEWrite(++eeRead, EE_ADDRESS);

    // Open a new file
    filePointer = NULL;
    sprintf(fileName, "%04x.txt", eeRead);
    while (filePointer == NULL) filePointer = FSfopen(fileName, FS_WRITE);

    // Initialize data for NewSDWriteSector
    filePointer->ccls = filePointer->cluster;

    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    NewFileUpdate(filePointer);
    return baudRate;
}

/**
 * Writes the data in outbuf to the file (pointer)
 * NEEDED FUNCTIONALITY:
 *  Calculate checksum DONE
 *  Add header, etc to given d// '%$'ata DONE
 *  Allocate multiple clusters when needed
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return 1 if successful, 0 otherwise. 
 */
int NewSDWriteSector(Sector sector)
{
    DWORD CurrentSector = Cluster2Sector(filePointer->dsk, filePointer->ccls)
        + filePointer->sec;
    DWORD SectorLimit = Cluster2Sector(filePointer->dsk, filePointer->ccls)
        + filePointer->dsk->SecPerClus;

    // add header and footer
    sector.sectorFormat.headerTag = HEADER_TAG;
    sector.sectorFormat.number = 0x01; // need to figure out how to number these
    sector.sectorFormat.checksum = twoByteChecksum(sector.sectorFormat.data,
        sizeof(sector.sectorFormat.data));
    sector.sectorFormat.footerTag = FOOTER_TAG;
    
    // Write the data
    int success = MDD_SDSPI_SectorWrite(CurrentSector, sector.raw, false);
    if (!success) {
        return 0;
    }

    // Check to see if we need to go to a new cluster; !! Also check for end of allocated area
    // else, next sector
    if (CurrentSector == SectorLimit - 1) {
        // if this is the last cluster, allocate more
        if(filePointer->ccls == lastCluster) {
            NewAllocateMultiple(filePointer);
        }
        // Set cluster and sector to next cluster in our chain
        if (FILEget_next_cluster(filePointer, 1) != CE_GOOD) {
            while (1);
        }
        filePointer->sec = 0;
    } else {
        filePointer->sec++;
    }

    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;

    // save off the positon
    filePointer->pos = BYTES_PER_SECTOR - 1; // current position in sector (bytes)

    // save off the seek
    filePointer->seek += BYTES_PER_SECTOR; // current position in file (bytes)

    // now the new size
    //  #Problem: This might not be accurate
    filePointer->size += BYTES_PER_SECTOR; // size of file (bytes)
    if (NewFileUpdate(filePointer)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 * @return 1 if success, 0 for failed update
 */
int NewFileUpdate(FSFILE * fo)
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
        return 0;
    }
    dir->DIR_FileSize = fo->size;
    dir->DIR_Attr = fo->attributes;
    dir->DIR_FstClusHI = (fo->cluster & 0xFFFF0000) >> 16;
    dir->DIR_FstClusLO = fo->cluster & 0x0000FFFF;
    
    Write_File_Entry(fo, &fHandle);
    
    return 1;
}

/**
 * Allocates multiple clusters to the given file. !! UNTESTED
 * @param fo Pointer to the file object to allocate to
 * @return sucess (1) or failure (0)
 */
int NewAllocateMultiple(FSFILE * fo)
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
    
    // update file data
    fo->size += fo->dsk->SecPerClus * MULTIPLE_CLUSTERS * BYTES_PER_SECTOR;
    return 0;
    /* Notes
     * use FILEget_next_cluster for stepping through clusters when writing (not this function)
     */
}

/**
 * Copies UTF-16 string to an ASCII string. Assumes all characters in the origin string can be
 * represnted in ASCII.
 * @param origin The string to copy
 * @param result Where to put it
 * @param number Max number of characters to copy
 * @return 0 if either of the arguments are NULL, or the string to copy is too large
 */
int utf16toStr(unsigned short int * origin, char * result, int number)
{
    if (origin == NULL || result == NULL) return 0;
    int index;
    for (index = 0; origin[index] != 0; index++) {
        result[index] = (char) origin[index];
        if (index == number - 1) return 0;
    }
    result[index] = (char) origin[index];
    return 1;
}

/**
 * Creates a two byte checksum - used for the sector checksum
 * return format : [odds][evens]
 *                 15          0
 * @param data A pointer to the data to checksum
 * @param dataSize The size of the data
 * @return the checksum
 */
uint16_t twoByteChecksum(uint8_t * data, int dataSize)
{
    int i;
    uint16_t sum = 0;
    for (i = 0; i < dataSize; i++) {
        if (i & 0x01) { // if odd
            sum ^= (data[i] << 8);
        } else { // if even
            sum ^= data[i];
        }
    }
    return sum;
}
/* NEW FUNCTION : allocate multiple clusters */

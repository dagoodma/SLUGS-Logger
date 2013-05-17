#include <xc.h>
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include <spi.h>
#include <math.h>
#include "NewSDWrite.h"
#include "Uart2.h"
#include <stdio.h>
#include "Node.h"
#include <string.h>

#define CONFIG_READ_SIZE 50
#define FILE_NAME "newfile.txt"
#define MAX_PREFIX "5"
#define MAX_SUFFIX "3"
#define EIGHT_THREE_LEN 8 + 1 + 3 + 1

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
int allocate_multiple_clusters(FSFILE*, DWORD);
BYTE FILEallocate_new_cluster(FSFILE *fo, BYTE mode);
DWORD FILEget_true_sector(FSFILE *);
DWORD ReadFAT(DISK *, DWORD);
DWORD Cluster2Sector(DISK *, DWORD);
DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle);
DWORD WriteFAT(DISK *dsk, DWORD ccls, DWORD value, BYTE forceWrite);
void IncrementTimeStamp(DIRENTRY dir);
BYTE Write_File_Entry(FILEOBJ fo, WORD * curEntry);
BYTE flushData(void);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;
int utf16toStr(unsigned short int * origin, char * result, int number);

FSFILE * filePointer;

/**
 * New functionality: Opens, reads config file. No
 * @return Buad rate
 */
long int NewSDInit()
{
    FSFILE *configFile = NULL;
    char configText[CONFIG_READ_SIZE + 1];
    long int baudRate;
    char fileName[EIGHT_THREE_LEN] = {}; // max size of a 8.3 file (null terminated)
    char fileBase[EIGHT_THREE_LEN] = {};
    SearchRec searchRec;

    filePointer = NULL;

    while (!MDD_MediaDetect()); // TODO make this smarter
    while (!FSInit());

    // Open then read the config file, null terminate the config text string
    while (configFile == NULL) configFile = FSfopen("CONFIG.TXT", FS_READPLUS); // open the file
    FSfread(configText, 1, CONFIG_READ_SIZE, configFile);
    FSrewind(configFile);
    configText[CONFIG_READ_SIZE] = '\0';

    // extract config info
    sscanf(configText, "BAUD %ld"
        "\nFNAME %" MAX_PREFIX "s", &baudRate, fileBase);

    char searchFormat[EIGHT_THREE_LEN];
    char extractFormat[EIGHT_THREE_LEN];
    int checkSuff = 0;
    int maxSuff = 0;
    char copyInto[EIGHT_THREE_LEN];

    sprintf(searchFormat, "%s???.txt", fileBase);
    sprintf(extractFormat, "%s%s", fileBase, "%3d");

    // search w/ searchFormat
    if (FindFirst(searchFormat, ATTR_MASK, &searchRec)) {
        // no file of that format was found - start at 000
        maxSuff = 0;
    } else {
        do {
            // check the filename
            if (searchRec.utf16LFNfoundLength) {
                utf16toStr(searchRec.utf16LFNfound, copyInto, EIGHT_THREE_LEN);
                sscanf(copyInto, extractFormat, &checkSuff);
            } else {
                sscanf(searchRec.filename, extractFormat, &checkSuff);
            }
            if (checkSuff > maxSuff) {
                maxSuff = checkSuff;
            }
        } while (!FindNext(&searchRec));
        maxSuff += 1; // important - increment the file suffix
    }
    if (maxSuff >= 1000) FATAL_ERROR();
    
    sprintf(fileName, "%s%03d.txt", fileBase, maxSuff);

    // Open a new file
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
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return 1 if successful, 0 otherwise. 
 */
int NewSDWriteSector(unsigned char outbuf[BYTES_PER_SECTOR])
{
    DWORD CurrentSector = Cluster2Sector(filePointer->dsk, filePointer->ccls)
        + filePointer->sec;
    DWORD SectorLimit = Cluster2Sector(filePointer->dsk, filePointer->ccls)
        + filePointer->dsk->SecPerClus;

    // Write the data
    int success = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0);
    if (!success) {
        return 0;
    }

    // Check to see if we need to go to a new cluster;
    // else, next sector
    if (CurrentSector == SectorLimit - 1) {
        // Set cluster and sector to next cluster in our chain
        if (FILEallocate_new_cluster(filePointer, 0) != CE_GOOD) { // allocate a new cluster
            // !! Also sets ccls to the new cluster
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
 * Allocate more clusters for a file.
 * !! This won't work. FILEallocate_new_cluster changes fo->ccls. One would need
 * to keep track of the real current cluster some other way.
 * @param fo File Object of the file you want to extend.
 * @param num_clusters The number of clusters to allocate.
 * @return Returns the number of successful allocations.
 */
int allocate_multiple_clusters(FSFILE* fo, DWORD num_clusters)
{
    int i;
    for (i = 0; i < num_clusters; i++) {
        if (FILEallocate_new_cluster(fo, 0) != CE_GOOD) {
            return i;
        }
    }
    return num_clusters;
}

/**
 * Copies UTF-16 string to an ASCII string. Assumes all characters in the origin string can be
 * represnted in ASCII.
 * @param origin The string to copy
 * @param result Where to put it
 * @param number Max number of characters to copy
 * @return 0 if either
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
#include <xc.h>
#include <stddef.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"
#include <spi.h>
#include <math.h>
#include "NewSDWrite.h"
#include "Uart2.h"
#include <stdio.h>

// Directory entry structure
typedef struct
{
    char      DIR_Name[DIR_NAMESIZE];           // File name
    char      DIR_Extension[DIR_EXTENSION];     // File extension
    BYTE      DIR_Attr;                         // File attributes
    BYTE      DIR_NTRes;                        // Reserved byte
    BYTE      DIR_CrtTimeTenth;                 // Create time (millisecond field)
    WORD      DIR_CrtTime;                      // Create time (second, minute, hour field)
    WORD      DIR_CrtDate;                      // Create date
    WORD      DIR_LstAccDate;                   // Last access date
    WORD      DIR_FstClusHI;                    // High word of the entry's first cluster number
    WORD      DIR_WrtTime;                      // Last update time
    WORD      DIR_WrtDate;                      // Last update date
    WORD      DIR_FstClusLO;                    // Low word of the entry's first cluster number
    DWORD     DIR_FileSize;                     // The 32-bit file size
}_DIRENTRY;

typedef _DIRENTRY * DIRENTRY;                   // A pointer to a directory entry structure
typedef FSFILE   * FILEOBJ;
int allocate_multiple_clusters(FSFILE*, DWORD);
BYTE FILEallocate_new_cluster(FSFILE *fo, BYTE mode);
DWORD FILEget_true_sector(FSFILE *);
DWORD ReadFAT (DISK *, DWORD);
DWORD Cluster2Sector(DISK *, DWORD);
DIRENTRY LoadDirAttrib(FILEOBJ fo, WORD *fHandle);
DWORD WriteFAT (DISK *dsk, DWORD ccls, DWORD value, BYTE forceWrite);
void IncrementTimeStamp(DIRENTRY dir);
BYTE Write_File_Entry( FILEOBJ fo, WORD * curEntry);
BYTE flushData (void);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;

/**
 * Initalizes filesystems and returns a file structure for use with
 * NewSDWriteSector.
 * @param filename A string with the filename (name.ext)
 * @return A file structure
 */
FSFILE * NewSDInit(char *filename)
{
    FSFILE * pointer = NULL;

    while (!MDD_MediaDetect()); // !! make this smarter
    while (!FSInit());

    // Open a new file
    while (pointer == NULL) pointer = FSfopen(filename, "w");

    // Initialize data for NewSDWriteSector
    pointer->ccls = pointer->cluster;

    // Trying something:
//    FILEallocate_new_cluster(pointer, 0);

    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    NewFileUpdate(pointer);
    return pointer;
}

/**
 * Writes the data in outbuf to the file (pointer)
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return 1 if successful, 0 otherwise. 
 */
int NewSDWriteSector(FSFILE *pointer, unsigned char outbuf[BYTES_PER_SECTOR])
{
    // Re-check SD card
    MEDIA_INFORMATION * Minfo;
    do {
        Minfo = MDD_MediaInitialize();
    } while(Minfo->errorCode == MEDIA_CANNOT_INITIALIZE);
//    if(!MDD_MediaDetect()) {
//        return 0;
//    }
//    while(!FSInit());
    
    char text[25]; // !! debug
    // Calculate the real sector number of our current place in the file.
    union{
        DWORD           dword;
        unsigned int    ints[2];
    }CSect;
    DWORD CurrentSector = Cluster2Sector(pointer->dsk, pointer->ccls)
            + pointer->sec;
    DWORD SectorLimit = Cluster2Sector(pointer->dsk, pointer->ccls)
            + pointer->dsk->SecPerClus;

    CSect.dword = CurrentSector;
    sprintf(text, "Writing: %X%X\n", CSect.ints[1], CSect.ints[0]);
    Uart2PrintStr(text);
    //Uart2PrintStr(sprintf("Writing: %x\n", CurrentSector));

    // Write the data
    int success = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0);
    if (!success) { // debug
        while(1);
        return 0;
    }

    // Check to see if we need to go to a new cluster;
    //  otherwise, next sector
    if (CurrentSector == SectorLimit - 1) {
        // Set cluster and sector to next cluster in our chain
        if(FILEallocate_new_cluster(pointer, 0)!=CE_GOOD){ // allocate a new cluster
            // !! Also sets ccls to the new cluster
            while(1);
        }
        pointer->sec = 0;
    } else {
        pointer->sec++;
    }

    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;

    // save off the positon
    pointer->pos = BYTES_PER_SECTOR-1; // current position in sector (bytes)

    // save off the seek
    pointer->seek += BYTES_PER_SECTOR; // current position in file (bytes)

    // now the new size
    //  #Problem: This might not be accurate
    pointer->size += BYTES_PER_SECTOR; // size of file (bytes)
    NewFileUpdate(pointer);

    
    CSect.dword = CurrentSector;
    sprintf(text, "Wrote: %X%X\n", CSect.ints[1], CSect.ints[0]);
    Uart2PrintStr(text);
    //Uart2PrintStr("Wrote.\n");

    return 1;
}

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 */
void NewFileUpdate(FSFILE * fo)
{
    DIRENTRY dir;
    WORD fHandle = fo->entry;

    // Write the file data
//    if(flushData()) while(1);
    WriteFAT (fo->dsk, 0, 0, TRUE);
    

    // Isn't correct anyways
    //IncrementTimeStamp(dir);

    // Update file entry data
    dir = LoadDirAttrib(fo, &fHandle);
    dir->DIR_FileSize = fo->size;
    dir->DIR_Attr = fo->attributes;
    dir->DIR_FstClusHI= (fo->cluster&0xFFFF0000)>>16;
    dir->DIR_FstClusLO= fo->cluster&0x0000FFFF;
    Write_File_Entry(fo,&fHandle);
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
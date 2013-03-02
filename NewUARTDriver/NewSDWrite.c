#include <xc.h>
#include <stddef.h>
#include "FSIO.h"
#include <spi.h>
#include <math.h>

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L

int allocate_multiple_clusters(FSFILE*, DWORD);
BYTE FILEallocate_new_cluster(FSFILE *fo, BYTE mode);
DWORD FILEget_true_sector(FSFILE *);
DWORD ReadFAT (DISK *, DWORD);
DWORD Cluster2Sector(DISK *, DWORD);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;

/**
 * Connects to the SD card, resets FAT, attempts to write.
 *
 */
void NewSDWrite()
{
    DWORD CurrentSector = 0;
    DWORD SectorLimit = 0;
    DWORD clusterTracker[TOTAL_SECTORS] = {}; // debug
    DWORD sectorTracker[TOTAL_SECTORS] = {}; // debug
    while (!MDD_MediaDetect());
    while (!FSInit());
    char filename[] = "Three.txt";
    FSFILE * pointer = NULL;

    MDD_SDSPI_MediaInitialize(); // conect to sd card
    while (pointer == NULL) {
        pointer = FSfopen(filename, "w"); // open a file
    }

    unsigned char outbuf[BYTES_PER_SECTOR]; // generate some data
    unsigned long i;
    for (i = 0; i < BYTES_PER_SECTOR; i++) {
        outbuf[i] = i % 26 + 'a';
    }

    // Chain together the clusters we need
    DWORD num_clusters = ceilf(TOTAL_SECTORS / (float)pointer->dsk->SecPerClus);
    allocate_multiple_clusters(pointer, num_clusters);

    // Set the current cluster and sector to the first cluster of the file.
    //  #Problem: this will overwrite a file that's already written
    pointer->ccls = pointer->cluster;
    CurrentSector = Cluster2Sector(pointer->dsk, pointer->ccls);
    SectorLimit = CurrentSector + pointer->dsk->SecPerClus;

    // For each sector we want to write...
    for (i = 0; i < TOTAL_SECTORS; i++) {
        // Write the data
        int success = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0);
        sectorTracker[i] = CurrentSector; // debug
        clusterTracker[i] = pointer->ccls; // debug
        if (!success) { // debug
            while (1);
        }

        // Check to see if we need to go to a new cluster;
        //  otherwise, next cluster
        if (CurrentSector == SectorLimit - 1) {
            // Set cluster and sector to next cluster in out chain
            pointer->ccls = ReadFAT(pointer->dsk, pointer->ccls);
            pointer->sec = 0;
            CurrentSector = Cluster2Sector(pointer->dsk, pointer->ccls);
            SectorLimit = CurrentSector + pointer->dsk->SecPerClus;
        } else {
            CurrentSector++;
        }
    }
    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    // save off the positon
    pointer->pos = BYTES_PER_SECTOR-1; // current position in sector (bytes)

    // save off the seek
    pointer->seek = TOTAL_SECTORS * BYTES_PER_SECTOR; // current position in file (bytes)

    // now the new size
    //  #Problem: This might not be accurate
    pointer->size = TOTAL_SECTORS * BYTES_PER_SECTOR; // size of file (bytes)
    FSfclose(pointer);
    int thelast = 1; // debug
}

/**
 * Allocate more clusters for a file.
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
 * Correct the sector and cluster data for a file to be valid numbers.
 * Not tested or used.
 * @param fo The file to normalize
 */
void normalize_file_data(FSFILE* fo)
{
    fo->ccls += fo->sec / fo->dsk->SecPerClus;  // The current cluster of the file
    fo->sec = fo->sec%fo->dsk->SecPerClus;      // The current sector in the current cluster of the file
}

/**
 * Doesn't work. Proposed functionality:
 * Write a sector's worth of data to the sd card. First call creates a new file.
 * Successive calls write to the same file.
 * @param outbuf Pointer to the data to write
 * @return 1 if successful, 0 othersise.
 */
int NewSDWriteSector(const unsigned char outbuf[BYTES_PER_SECTOR])
{
    DWORD CurrentSector = 0;
    DWORD SectorLimit = 0;
    static int firstCall = 1;
    while (!MDD_MediaDetect()); // !! make this smarter
    char filename[] = "1.txt";
    static FSFILE * pointer = NULL;

    // connect to sd card (!! similar to MDD_MediaDetect)
    MDD_SDSPI_MediaInitialize();

    // open a file (should run only once)
    while (pointer == NULL) {
        pointer = FSfopen(filename, "w"); 
    }

    // Chain together the clusters we need
    DWORD num_clusters = ceilf(TOTAL_SECTORS / (float)pointer->dsk->SecPerClus);
    allocate_multiple_clusters(pointer, num_clusters);

    // Set the current cluster and sector to the first cluster of the file.
    //  #Problem: this will overwrite a file that's already written
    if (firstCall) {
        pointer->ccls = pointer->cluster;
    }

    // Calculate the real sector number of our current place in the file.
    CurrentSector = Cluster2Sector(pointer->dsk, pointer->ccls) + pointer->sec;
    SectorLimit = Cluster2Sector(pointer->dsk, pointer->ccls) + pointer->dsk->SecPerClus;
    
    // Write the data
    int success = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0);
    if (!success) { // debug
        return 0;
    }

    // Check to see if we need to go to a new cluster;
    //  otherwise, next cluster
    if (CurrentSector == SectorLimit - 1) {
        // Set cluster and sector to next cluster in out chain
        pointer->ccls = ReadFAT(pointer->dsk, pointer->ccls);
        pointer->sec = 0;
        CurrentSector = Cluster2Sector(pointer->dsk, pointer->ccls);
        SectorLimit = CurrentSector + pointer->dsk->SecPerClus;
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
    FSfclose(pointer);
    return 1;
}

void NewSDInit()
{
    while (!MDD_MediaDetect()); // !! make this smarter
    while (!FSInit());
}
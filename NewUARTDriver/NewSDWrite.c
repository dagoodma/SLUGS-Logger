#include <xc.h>
#include <stddef.h>
#include "FSIO.h"
#include <spi.h>
#include <math.h>

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 130

int allocate_multiple_clusters(FSFILE*, DWORD);
BYTE FILEallocate_new_cluster(FSFILE *fo, BYTE mode);
extern BYTE gNeedFATWrite;
extern BYTE gNeedDataWrite;

/**
 * Connects to the SD card, resets FAT, attempts to write.
 *
 */
void NewSDWrite()
{
    static DWORD CurrentSector = 0;
    while (!MDD_MediaDetect());
    while (!FSInit());
    char filename[] = "Two.txt";
    FSFILE * pointer = NULL;

    MDD_SDSPI_MediaInitialize(); // conect to sd card
    long int serialNumber = 0; // specific serial # doesn't matter
    while (pointer == NULL) {
        pointer = FSfopen(filename, "w"); // open a file
    }

    unsigned char outbuf[TOTAL_SECTORS * 512]; // generate some data
    unsigned int i;
    for (i = 0; i < TOTAL_SECTORS * 512; i++) {
        outbuf[i] = i % 26 + 'a';
    }

    DWORD num_clusters = ceilf(TOTAL_SECTORS / (float)pointer->dsk->SecPerClus);
    allocate_multiple_clusters(pointer, num_clusters);
    CurrentSector = get_First_Sector(pointer);
    for (i = 0; i < TOTAL_SECTORS; i++) {
        int success = MDD_SDSPI_SectorWrite(CurrentSector++, &outbuf[i * 512], 0);
        if (!success) {
            while (1);
        }
    }
    gNeedFATWrite = TRUE;
    gNeedDataWrite = FALSE;
    // save off the positon
    pointer->pos = 512;

    // save off the seek
    pointer->seek = TOTAL_SECTORS * 512;

    // now the new size
    pointer->size = TOTAL_SECTORS * 512;
    pointer->sec = TOTAL_SECTORS - 1;
    FSfclose(pointer);
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
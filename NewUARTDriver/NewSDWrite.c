#include <xc.h>
#include <stddef.h>
#include "FSIO.h"

/**
 * Connects to the SD card, resets FAT, attempts to write.
 */
void NewSDWrite() {
    static DWORD CurrentSector = 0;
    while (!MDD_MediaDetect());
    while (!FSInit());
    char filename[] = "Yay.txt";
    FSFILE * pointer = NULL;
    while (pointer == NULL) {
        pointer = FSfopen(filename, "w"); // #FOR TUESDAY: might want to do this after reset
    }
    MDD_SDSPI_MediaInitialize();
    long int serialNumber = 0;
    char * volumeID = "Nice";
    FSformat (1, serialNumber, volumeID); // Unmodified format func. Resets FAT

    unsigned char outbuf[512]; // generate some data
    unsigned int i;
    for (i = 0; i < 512; i++) {
        outbuf[i] = i % 26 + 'A';
    }

    if(FSfwrite(outbuf, 512, 1, pointer) != 1){ // Unmodified write-file function
        while(1);
    }
    
    //FILEallocate_multiple_clusters(pointer, 1); // CHEW_FAT_SIZE_IN_SECTORS

//    CurrentSector = get_First_Sector(pointer);
    
//    BYTE error;
//    for (i = 0; i < 1000; i++) {
//        error = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0);
//        if(error) break;
//    }
//    while (error == FALSE);
}
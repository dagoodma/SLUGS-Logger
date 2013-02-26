#include <xc.h>
#include <stddef.h>
#include "FSIO.h"

/**
 * Connects to the SD card, resets FAT, attempts to write.
 *
 */
void NewSDWrite() {
    static DWORD CurrentSector = 0;
    while (!MDD_MediaDetect());
    while (!FSInit());
    char filename[] = "Yay.txt";
    FSFILE * pointer = NULL;
    
    MDD_SDSPI_MediaInitialize();    // conect to sd card
    long int serialNumber = 0;      // specific serial # doesn't matter
    while (pointer == NULL) {
        pointer = FSfopen(filename, "w"); // open a file
    }

    unsigned char outbuf[512]; // generate some data
    unsigned int i;
    for (i = 0; i < 512; i++) {
        outbuf[i] = i % 26 + 'A';
    }

    if(FSfwrite(outbuf, 512, 1, pointer) != 1){ // "write" to the file, check if it worked
        while(1);
    }
    FSfclose(pointer); // File "wrote" successfully. This function actually puts the data on the sd card
}
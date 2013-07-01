#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

#include <stdint.h>
#include "Libs/Microchip/Include/MDD File System/FSIO.h"

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L
#define HEAD_FOOT_LEN 8
#define HEADER_TAG (('%' << 8) & '^') // '%^'
#define FOOTER_TAG (('%' << 8) & '$') // '%$'
#define EE_ADDRESS 1

// a struct to combine data with a header and footer
typedef struct {
    uint16_t headerTag;
    uint16_t number;
    uint8_t  data[504];
    uint16_t checksum;
    uint16_t footerTag;
} SectorFormat;

// this union makes the SectorFormat usable as a simple array
typedef union {
    SectorFormat sectorFormat;
    uint8_t raw[sizeof(SectorFormat)];
} Sector;

/**
 * Initalizes filesystems and returns a file structure for use with
 * NewSDWriteSector.
 * @param filename A string with the filename (name.ext)
 * @return A file structure
 */
long int NewSDInit();

/**
 * Writes the data in outbuf to the file (pointer)
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return 1 if successful, 0 otherwise.
 */
int NewFileUpdate(FSFILE * pointer);

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 */
int NewSDWriteSector(Sector sector);

#endif
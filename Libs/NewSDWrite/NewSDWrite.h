#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L

/**
 * Initalizes filesystems and returns a file structure for use with
 * NewSDWriteSector.
 * @param filename A string with the filename (name.ext)
 * @return A file structure
 */
FSFILE * NewSDInit(char *filename);

/**
 * Writes the data in outbuf to the file (pointer)
 * @param pointer The FSFILE to write to
 * @param outbuf An array of data (BYTES_PER_SECTOR in legnth)
 * @return 1 if successful, 0 otherwise.
 */
void NewFileUpdate(FSFILE * pointer);

/**
 * Mimics FSfileClose to write file info to the SD card.
 * @param fo The file to update
 */
int NewSDWriteSector(FSFILE *pointer, unsigned char outbuf[BYTES_PER_SECTOR]);

#endif
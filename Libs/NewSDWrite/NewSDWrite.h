#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

// this cannot be greater than the number of sectors in a cluster
#define TOTAL_SECTORS 80L
#define BYTES_PER_SECTOR 512L

FSFILE * NewSDInit(char *filename);
void NewFileUpdate(FSFILE * pointer);
int NewSDWriteSector(FSFILE *pointer, unsigned char outbuf[BYTES_PER_SECTOR]);

#endif
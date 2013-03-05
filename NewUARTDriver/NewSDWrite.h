#ifndef NEWSDWRITE_H
#define	NEWSDWRITE_H

int NewSDSimpleWriteSector(const unsigned char *);
void NewSDSimpleInit();
FSFILE * NewSDInit(char *filename);
void NewFileUpdate(FSFILE * pointer);


#endif
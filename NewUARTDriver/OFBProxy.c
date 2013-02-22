
#define SECTORSIZE 512

static int OFB_size = 0;
unsigned char *OFB_buffer;

void OFB_set(unsigned char *buffer) { // used by main.c to signal when a buffer is filled
    OFB_size = 1;
    OFB_buffer = buffer;
}

unsigned int OFB_getSize(void) {
    return OFB_size;
}

int OFB_pop(void) {
    OFB_size = 0;
    return 0;
}

int OFB_read_tail(unsigned char outputarray[]) {
    if ( OFB_size != 0) {
        int j;
        for (j = 0; j < SECTORSIZE; j++)
            outputarray[j] = OFB_buffer[j];
        return 1;
    } else {
        return 0;
    }
}
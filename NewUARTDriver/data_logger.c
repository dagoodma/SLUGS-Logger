#include "xc.h"
//#include "apDefinitions.h"
//#include "MultiCircBuffer.h"
#include "MDD File System/FSIO.h"
#include "stdio.h"
#include "loggerDefinitions.h"
#include "OFBProxy.h"


#define BUFFERSIZE 512

#include "SD-SPI.h"

static char bufferflag;
static char need_buffer;
//FSFILE * pointer;

void Init_Data_Logger(int id) {

}



static DWORD first_sector;

void set_First_Sector(DWORD sector) {
    //CurrentSector=sector;
    first_sector = sector;
}


//bulk of the program, checks for flags and then adds items to the buffer or
//writes to SPI as needed

void Service_Spi(FSFILE *fo) {
    static int card_initerr = 1;
    static DWORD sector_count = 0;
    static DWORD CurrentSector = 0;
    static int IncreaseSize = FALSE;
    BYTE error = TRUE;

    int i  = 0;
    i++;

    // if bytes are ready from DMA interrupt

    //if sectors are still in the buffer or there was en error
    if ((sector_count >= CHEW_FAT_SIZE_IN_SECTORS) || (sector_count == 0)) {
        FILEallocate_multiple_clusters(fo, CHEW_FAT_SIZE_IN_SECTORS);
        sector_count = 1;
        if (CurrentSector == 0) {
            CurrentSector = get_First_Sector(fo);
        }

    }
    if (OFB_getSize() != 0)//if (need_write==1)
    {
        if (sector_count >= NINETY_PER_CHEW_FAT_SIZE_IN_SECTORS) {
            IncreaseSize = TRUE;
            sector_count = 1;
            if (CurrentSector == 0) {
                CurrentSector = get_First_Sector(fo);
            }
        }
        //if there is an error from writing to the card or the last card
        //init has failed it trys to init the card again
        //if(error!=TRUE||card_initerr!=1)
        if (card_initerr != TRUE) {
            card_initerr = MDD_SDSPI_MediaInitialize();
            return;
        } else {
            //if the card is successfully inited it reads out a sector and
            //dumps to spi
            unsigned char outbuf[512];
            OFB_read_tail(outbuf); // 1
            error = MDD_SDSPI_SectorWrite(CurrentSector, outbuf, 0); // 2
            if (error == TRUE) {
                //if it succeeds pop sector off buffer and advance
                OFB_pop();
                CurrentSector++;
                sector_count++;
                return;
            } else {
                //if error has again occured try to init card again
                card_initerr = MDD_SDSPI_MediaInitialize();
            }
        }
    } else if (IncreaseSize) {
        FILEallocate_multiple_clusters(fo, CHEW_FAT_SIZE_IN_SECTORS);
        IncreaseSize = FALSE;
        sector_count = 1;
    }
}
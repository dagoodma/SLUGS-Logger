# Slogger #

This is the code for a data logger currently being created at the UCSC Autonomous Systems Lab.

See http://byron.soe.ucsc.edu/wiki/Slogger for more documentation.

## Using the Slogger ##
Put a valid "config.txt" file onto the target micro SD card. Insert the card and connect the Slogger to a serial input. A new file will be created on the card with a name like "0001.txt". (The number increases by one for every reset.) When logging is finished, use the "dataExtract.py" tool to remove headers and footers from the data.

### Other Usage Notes ###

* Put a valid "config.txt" file on the target SD card. See documentation for an example file. NOTE: The Slogger has only been tested with 115200 baud UART.

* Inserting an SD card might cause the device to reset.

* Resetting the device will cause a new file to be created.

* Data is recorded in chunks of 506 bytes. It is possible for the last 505 bytes of data to be lost.

* __IMPORTANT:__ This project is still in development. Chunks of data might be lost. Please take note of any issues, and send them to the developer: Jesse Harkin (jdharkin@ucsc.edu).

## Code ##
### Overview ###
![](/docs/images/slogger_data_flow.png "Slogger Data Flow")
* __DMA__ - The DMA peripheral receives incoming serial data from the UART peripheral, puts it into two ping-pong buffers in memory, then triggers an iterrupt when a buffer is full.
* __Software__ - Inside the DMA interrupt, the full ping-pong buffer is copied into the circular buffer.
Inside the main loop, a chunk of data is taken from the circular buffer, formatted with a header and footer, then sent via SPI to the SD card.

### Custom Libraries ###
* __NewSDWrite__ - This library uses parts of Microchip's SD-SPI and FSIO libraries to read and efficiently write to the SD card. It uses functions which were meant to only be used inside of Microchip's libraries.
* Uart2 - This library uses the DMA to receive data from the UART2 peripheral. All of the DMA stuff is in here.

### Other Libraries ###
* __CircularBuffer__ - This library sets up and deals with the circular buffer.
* __DEE__ - This library emulates an EEPROM peripheral. This is used inside the NewSDWrite library for naming the new file created on reset.
* __SD-SPI__ and __FSIO__ - These libraries are used by NewSDWrite.

*The Uart2 and Timer2 libraries are not used in the current version.

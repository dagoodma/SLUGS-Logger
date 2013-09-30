# Slogger #

This is the code and important files for a data logger currently being created at the UCSC Autonomous Systems Lab.

## Using the Slogger ##
Put a valid "config.txt" (see below) file onto the target micro SD card. Insert the card and connect the Slogger to a serial input. A new file will be created on the card with a name like "0001.txt". (The number increases by one for every reset.) When logging is finished, use the "dataExtract.py" tool to remove headers and footers from the data.

### Other Usage Notes ###

* Put a valid "config.txt" file on the target SD card. See documentation for an example file. NOTE: The Slogger has only been tested with 115200 baud UART.

* Inserting an SD card might cause the device to reset.

* Resetting the device will cause a new file to be created.

* Data is recorded in chunks of 506 bytes. It is possible for the last 505 bytes of data to be lost.

* The red LED is used to indicate the state of the device. When the red LED is off, the Slogger is able to log data. When the red LED is on, the device has encountered a fatal error and needs to be reset to continue logging.

* **IMPORTANT:** This project is still in development. Chunks of data might be lost. Please take note of any issues, and send them to the developer: Jesse Harkin (jdharkin@ucsc.edu).

## Motivation ##
This project was created because our lab found no available data-loggers that are:
* Reliable at high speeds
* Robust

More specifically, the Slogger was designed to meet these requirements:
* Support continuous 115200 buad UART
* Maintain the file system while minimizing writes
  * Allocate a file "sweet spot" in size and then increase in that size as needed
* Integrity checking
  * Data checksums
* Robust to power/card errors
  * Ensure file system integrity even when card errors out at most inopportune moments. (i.e., a cluster update)

It has also been designed with the posibility for supporting multiple inputs and multiple input types in the future.

## Code ##
### Overview ###
* **Initialization** - The DEE and peripherals are initialized. The config file is read, and a new log file is created on the SD card.
* **DMA** - The DMA peripheral receives incoming serial data from the UART peripheral, puts it into two ping-pong buffers in memory, then triggers an iterrupt when a buffer is full.
* **Software** - Inside the DMA interrupt, the full ping-pong buffer is copied into the circular buffer.
Inside the main loop, a chunk of data is taken from the circular buffer, formatted with a header and footer, then sent via SPI to the SD card.
![](/docs/images/slogger_data_flow.png "Slogger Data Flow")

### Custom Libraries ###
* **NewSDWrite** - This library uses parts of Microchip's SD-SPI and FSIO libraries to read and efficiently write to the SD card. It uses functions which were meant to only be used inside of Microchip's libraries.
* **Uart2** - This library uses the DMA to receive data from the UART2 peripheral. All of the DMA stuff is in here.

### Other Libraries ###
* **CircularBuffer** - This library sets up and deals with the circular buffer.
* **DEE** - This library emulates an EEPROM peripheral. This is used inside the NewSDWrite library for naming the new file created on reset.
* **SD-SPI** and **FSIO** - These libraries are used by NewSDWrite.

* The Uart1 and Timer2 libraries are not used in the current version.

## Data Header/Footer ##
Data is stored in 512 byte sectors on the SD card, which includes 6 bytes for the header/footer. (Because of this, data in this device is dealt with in chunks of 506 bytes.)
* Header: 2 byte tag ('%^'), 1 byte identification (same as file name)
* Footer: 1 byte checksum, 2 byte tag ('%$')

The checksum is an XOR of all bytes in the data; the header and footer are not included. The header and footer are checked then removed by the DataExtract.py script.

## Config File ##
The config file should be located on the target SD card and named "config.txt". Configurations are set with a keyword and an integer value for the desired setting. At the moment, the config file sets only one thing: the UART input baud.

NOTE: The Slogger has only been tested at 115200 baud.

**Example:** 
```
BAUD 115200

```

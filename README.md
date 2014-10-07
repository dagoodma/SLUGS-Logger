# Slogger #

This is the code and important files for a data logger currently being created at the UCSC Autonomous Systems Lab.

## Using the Slogger ##
Put a valid "config.txt" (see below) file onto the target micro SD card. Insert the card and connect the Slogger to a serial input. A new file will be created on the card with a name like "0001.txt" (The number increases by one for every reset). When logging is finished, use the "dataExtract.py" tool to remove headers and footers from the data.

### Other Usage Notes ###

* Create a "config.txt" file on the target SD card with the desired configuration options. See the "Config file" section.

* Resetting the device will cause a new file to be created.

* Inserting an SD card might cause the device to reset. This happens when the power supply can't supply power fast enough for the SD card, which consumes a lot of power on initial power-up. This won't be a problem for the device, but will create a new file as the device was reset. To solve this issue, put at least a 200uF electrolytic capacitor across the power lines near the SD card slot.

* Data is recorded in chunks of 506 bytes.

* The yellow LED indicates when the processor has initialized properly and is ready to use.

* The red LED is used to indicate the status of the SD card. When the red LED is off, the Slogger is able to log data. When the red LED is on, the device has encountered a fatal error and needs to be reset to continue logging. This normally indicates a problem with the SD card or the configuration file.

## Motivation ##
This project was created because our lab found no available data-loggers that are:
* Reliable at high speeds
* Robust

More specifically, the Slogger was designed to meet these requirements:
* Support continuous 115200 buad UART recording
* Maintain the file system while minimizing writes
  * Allocate a file "sweet spot" in size and then increase in that size as needed
* Integrity checking
  * Data checksums
* Robust to power/card errors
  * Ensure file system integrity even when card errors out at most inopportune moments. (i.e., a cluster update)

It has also been designed with the possibility for supporting additional inputs in the future.

## Code ##
### Overview ###
* **Initialization** - The DEE and peripherals are initialized. The config file is read, and a new log file is created on the SD card.
* **DMA** - The DMA peripheral receives incoming serial data from the UART peripheral, puts it into two ping-pong buffers in memory, then triggers an interrupt when a buffer is full.
* **Software** - Inside the DMA interrupt, the full ping-pong buffer is copied into the circular buffer.
Inside the main loop, a chunk of data is taken from the circular buffer, formatted with a header and footer, then sent via SPI to the SD card.
![](/docs/slogger_data_flow.png "Slogger Data Flow")

### Custom Libraries ###
* **NewSDWrite** - This library uses parts of Microchip's SD-SPI and FSIO libraries to read and efficiently write to the SD card. It uses functions which were meant to only be used inside of Microchip's libraries.
* **Uart2** - This library uses the DMA to receive data from the UART2 peripheral. All of the DMA stuff is in here.

### Other Libraries ###
* **CircularBuffer** - This library sets up and deals with the circular buffer.
* **DEE** - This library emulates an EEPROM peripheral. This is used inside the NewSDWrite library for naming the new file created on reset.
* **SD-SPI** and **FSIO** - These libraries are used by NewSDWrite.

### Building ###
Building requires the MPLAB X IDE and XC16 compiler, all available for free on all platforms directly from Microchip's website.

Building entails creating a new MPLAB X project including all *.s and *.c in the root directory and under /Libs. Additionally, some include directories must be specified: the root directory, /Libs, and /Libs/Microchip/Include. Finally, under project properties->xc16-gcc, set the compiler to use the large data model.

## Data Header/Footer ##
Data is stored in 512 byte sectors on the SD card, which includes 6 bytes for the header/footer. (Because of this, data in this device is dealt with in chunks of 506 bytes.)
* Header: 2 byte tag ('%^'), 1 byte identification (same as file name)
* Footer: 1 byte checksum, 2 byte tag ('%$')

The checksum is an XOR of all bytes in the data; the header and footer are not included. The header and footer are checked then removed by the DataExtract.py script.

## Config File ##
The config file should be located on the target SD card and named "config.txt". Configurations are set with a keyword and an integer value for the desired setting. At the moment, the config file sets only one thing: the UART input baud.

**Example:**
```
BAUD 115200

```

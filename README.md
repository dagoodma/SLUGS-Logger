# Slogger #

This is the code for a data logger currently being created at the UCSC Autonomous Systems Lab. It was written to run on the CANode and associated microSD shield. Contact Professor Gabriel Elkaim for more information on this device.

## Using the Slogger ##
Use a FAT16/32-formatted microSD card. Create a valid "config.txt" (see below) file on it and insert the card. Powering up the device starts a new logfile. If the red LED turns off, then the Slogger is reading for logging. When logging is finished, use the "dataExtract.py" tool to remove headers and footers from the data.

### Other Usage Notes ###

* Create a "config.txt" file on the target SD card with the desired configuration options. See the "Config file" section.

* Resetting the device will cause a new file to be created.

* Inserting an SD card might cause the device to reset. This happens when the power supply can't supply power fast enough for the SD card, which consumes a lot of power on initial power-up. This won't be a problem for the device, but will create a new file as the device was reset. To solve this issue, put at least a 200uF electrolytic capacitor across the power lines near the SD card slot.

* UART data is recorded in chunks of 506 bytes. Note that some data may be lost at the end of the file (on a 17MB datafile 0.013% of the total file was lost, and it was just at the end of the file).

* The yellow LED indicates when the processor has initialized properly and is ready to use. If this LED doesn't turn on, it's likely a failure with the onboard MCU or possibly power issues on the board.

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
  
## Hardware ##
The SLUGS Logger was redesigned to fit onto a compact PCB that includes UART and optional CAN connections. See the schematic in `docs/SLogger_rev5.pdf`. Below are lists of components required to assemble the PCB. Also see `docs/slugs_logger_board_layout.png` for a sketch of the board layout with components labelled.

### IC Components ###
* 1A LDO - MCP1826-SOT22305
* [Optional CAN Controller - MCP2551-SO08]
* PIC uController - DSPIC33FJ802GP-28-SOIC28-W
* Micro-SD-Socket-PP: USD-SOCKETNEW

### Discrete Components ###
* D1: ?
* LED1: red
* LED2: amber
* R1: 100 kOhm
* R2, R3: 330 Ohm
* C1, C2, C3: 4.7 uF
* C4, C6: 1 uF
* C8: 10 uF
* C9, C10: 0.1uF

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

## Building ##
Building requires the MPLAB X IDE and XC16 compiler, all available for free on all platforms directly from Microchip's website.

Building entails creating a new MPLAB X project including all *.s and *.c in the root directory and under /Libs. Additionally, some include directories must be specified: the root directory, /Libs, and /Libs/Microchip/Include. Finally, under project properties->xc16-gcc, set the compiler to use the large data model.

## Data storage format ##
Data is stored in 512 byte sectors on the SD card, which includes 6 bytes for the header/footer. (Because of this, data in this device is dealt with in chunks of 506 bytes.)
* Header: 2-byte tag ('%^'), 2-byte identification number (same as file name), 4-byte timestamp in milliseconds since file creation
* Mid-file (8 + 16 * 16 = 264): 4-byte timestamp in milliseconds since file creation
* Footer: 2-byte XOR-checksum, 2-byte tag ('%$')

The checksum is a 16-bit XOR of the entire sector of data starting from the identification number and stopping before the checksum (so the checksum and header and footer tags are excluded).

File naming uses a 4-digit identifier number along with the peripheral name, so '0001_can.log', '01bf_uart1.log', and '1fff_uart2.log' are all valid file names. The file number is incremented every time the board is reset or powered off. Once the highest number (0xFFFF) is reached, the file number will just roll over back to 0. If multiple peripherals are recording, they'll be stored in separate files, but have the same identifier, as the identifier is used for all log files during the same power-on interval.

### Data extraction ###
A Python3 script has been provided that can extract UART data from appropriate data logs. See the `Testing/ExtractData.py` script.

## Testing ##
Testing can be easily performed by using a suitable computer or other UART data source and sending UART data at the SLUGS-Logger. Test files have been provided in the `Testing` folder as text files. When sending a file, let the file transfer complete on the PC, and load the SD card into a computer. From here, use the ExtractData.py script to extract the data from the file and use a file diff tool (Meld, WinMerge, etc.) to check for any missing data.

## Config File ##
The config file should be located on the target SD card and named "config.txt". Configurations are set with a keyword and an integer value for the desired setting. Possible options are:

UARTx_BAUD - This parameter enables logging on the UART1 peripheral. Valid values are from 9600 through 115200. (x can be 1 or 2)
UARTx_INPUT - This parameter specifies where the input data stream is coming from. Valid values are [BUILTIN_RX, CONN1_TX, CONN1_RX, CONN2_TX, CONN2_RX]. See the hardware you're using to see which pins map to which connectors.

CAN_BAUD - This parameter controls the CAN baud rate. Right now the only valid option is 250000.

**Example:**
```
UART1_BAUD 115200
UART1_INPUT CONN1_TX
CAN_BAUD 250000
```

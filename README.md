SLUGS-Logger
============

This is the code for a data logger currently being created at the UCSC Autonomous Systems Lab.

See http://byron.soe.ucsc.edu/wiki/Slogger for more documentation.

Usage Notes:
------------
-Send data to the device with 115200 baud UART
-Put valid config.txt file on the target SD card. See documentation for an example file. NOTE: The config file does not set the baudrate of the device, but a valid config file with BAUD [number] tag is still required.
-Ejecting the card and reinserting it will cause the device to reset.
-Resetting the device will cause a new file to be created
-Data is recorded in chunks of 506 bytes. It is possible for the last 505 bytes of data to be lost.

-IMPORTANT: This project is still in development. Chunks of data might not be recorded. Please take note of any issues, and send them to the developer: Jesse Harkin (jdharkin@ucsc.edu).

/* 
 * File:   Uart2.h
 * Author: Jesse
 *
 * Created on February 19, 2013, 1:56 PM
 */

#ifndef UART2_H
#define	UART2_H

#ifndef UART2_BUFFER_SIZE
#define UART2_BUFFER_SIZE (512-7) // the size of an individual ping-ping buffer
#endif

#ifndef FCY
#define FCY 40000000UL // assumes 80Mhz freq (FCY / 2)
#endif
#define BAUDRATE 115200 // assumes 115200 baud rate
#define BRGVAL ((FCY/BAUDRATE)/16)-1 // used in Uart2Init()

void Uart2Init(long int, void (*Callback)(unsigned char *, int)); // initializes UART and DMA to a ping-pong buffer

void Uart2PrintChar(char in); // sends a character with UART2

void Uart2PrintStr(char *string); // sends a string

void Uart2SendBytes(char * buffer, int n);

#endif	/* UART2_H */

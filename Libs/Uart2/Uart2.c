#include <xc.h>
#include "NewSDWrite.h"
#include "Uart2.h"
#include <string.h>

unsigned char BufferA[UART2_BUFFER_SIZE];
unsigned char BufferB[UART2_BUFFER_SIZE];

void (*InterruptCallback)(unsigned char *, int);

/**
 * Initializes the UART2 and DMA0.
 * UART recieves 1 stop bit,  8 data bits, no parity. Assumes 112500 baud rate.
 * DMA puts data recived into two buffers (ping-pong) and interrupts when one is
 * full.
 * @param Callback
 * A user-defined function. It is called when the DMA has filled up a ping-pong
 * buffer. First argument is a pointer the the buffer. Second is buffer size (int).
 */
void Uart2Init(long int baudRate, void (*Callback)(unsigned char *, int))
{ // Just sit and spin with this. It's all with interrupts. (see lower)
	InterruptCallback = Callback;

	U2MODEbits.STSEL = 0; // 1-stop bit
	U2MODEbits.PDSEL = 0; // No Parity, 8-data bits
	U2MODEbits.ABAUD = 0; // Auto-Baud Disabled

	U2BRG = (FCY / baudRate / 16); // Should be 21 at 115200 baud

	U2STAbits.UTXISEL0 = 0; // Interrupt after one Tx character is transmitted
	U2STAbits.UTXISEL1 = 0;
	U2STAbits.URXISEL = 0; // Interrupt after one Rx character is recieved

	IEC1bits.U2RXIE = 0; // Disable Rx Interrupt
	IEC1bits.U2TXIE = 0; // Disable Tx Interrupt

	U2MODEbits.UARTEN = 1; // Enable UART
	U2STAbits.UTXEN = 1; // Enable UART Tx

	// Set Up DMA Channel 0 to Transmit in Continuous, Ping-Pong Mode:
	DMA0CON = 0x4002; // Continuous, Post-Increment, Peripheral-to-RAM, Ping-Pong
	DMA0CNT = UART2_BUFFER_SIZE-1; // DMA requests before Interrupting
	DMA0REQ = 0x001E; // Select UART2 Reciever

	DMA0PAD = (volatile unsigned int) &U2RXREG;
	DMA0STAL = (unsigned int)&BufferA;
        DMA0STAH = 0x0;
	DMA0STBL = (unsigned int)&BufferB;
        DMA0STBH = 0x0;

	IFS0bits.DMA0IF = 0;
	IEC0bits.DMA0IE = 1; // Enable DMA interrupt

	DMA0CONbits.CHEN = 1; // Enable DMA Channel
}

/**
 * Sends a single character over the UART2 peripheral. Meant to be used for
 * debugging.
 * @param in
 * An ASCII character to send. Really, this could be any byte data.
 */
void Uart2PrintChar(char in)
{
	while(U2STAbits.UTXBF); // wait for a space in the buffer
	U2TXREG = in;
}

/**
 * Sends multiple characters through the UART2
 * @param string The string to send
 */
void Uart2PrintStr(char *string)
{
    int i;
    for (i = 0; string[i] != '\0'; i++) {
        Uart2PrintChar(string[i]);
    }
}

/**
 * Send a certain number of bytes from an array
 * @param buffer Pointer to the bytes
 * @param n Size of the buffer in chars
 */
void Uart2SendBytes(char * buffer, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        Uart2PrintChar(buffer[i]);
    }
}


/**
 * DMA0 Interrupt.
 * Calls the user-defined callback function with the correct buffer pointer.
 */
void __attribute__((__interrupt__, __auto_psv__)) _DMA0Interrupt(void)
{	
	static unsigned int CurrentBuffer = 0;
	if(!CurrentBuffer) {
		InterruptCallback(BufferA, UART2_BUFFER_SIZE);
	}
	else
	{
		InterruptCallback(BufferB, UART2_BUFFER_SIZE);
	}
 	CurrentBuffer ^= 1; //
	IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt Flag
}

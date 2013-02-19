#include <xc.h>
#include "Uart2.h"

unsigned int BufferA[UART2_BUFFER_SIZE] __attribute__((space(dma)));
unsigned int BufferB[UART2_BUFFER_SIZE] __attribute__((space(dma)));

void Uart2Init(void)
{
	U2MODEbits.STSEL = 0; // 1-stop bit
	U2MODEbits.PDSEL = 0; // No Parity, 8-data bits
	U2MODEbits.ABAUD = 0; // Auto-Baud Disabled

	U2BRG = BRGVAL; // Buad rate;

	U2STAbits.UTXISEL0 = 0; // Interrupt after one Tx character is transmitted
	U2STAbits.UTXISEL1 = 0;
	U2STAbits.URXISEL = 0; // Interrupt after one Rx character is recieved

	IEC1bits.U2RXIE = 0; // Disable Rx Interrupt
	IEC1bits.U2TXIE = 0; // Disable Tx Interrupt

	U2MODEbits.UARTEN = 1; // Enable UART
	U2STAbits.UTXEN = 1; // Enable UART Tx

	// Set Up DMA Channel 0 to Transmit in Continuous, Ping-Pong Mode:
	DMA0CON = 0x0002; // Continuous, Post-Increment, Peripheral-to-RAM, Ping-Pong
	DMA0CNT = UART2_BUFFER_SIZE-1; // 8 DMA requests
	DMA0REQ = 0x001E; // Select UART2 Reciever

	DMA0PAD = (volatile unsigned int) &U2RXREG;
	DMA0STA = __builtin_dmaoffset(BufferA);
	DMA0STB = __builtin_dmaoffset(BufferB);

	IFS0bits.DMA0IF = 0;
	IEC0bits.DMA0IE = 1; // Enable DMA interrupt

	DMA0CONbits.CHEN = 1; // Enable DMA Channel
}

void Uart2PrintChar(char in)
{
	while(U2STAbits.UTXBF); // wait for a space in the buffer
	U2TXREG = in;
}

void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void)
{ // Won't interrupt with current settings! Reading from U2RXREG and using DMA with UART creates problems
	char x = U2RXREG; // Read UxRXREG only once per interrupt!
	U2TXREG = x; // echo

	// If there was an overun error clear it and continue
	if (U2STAbits.OERR == 1) {
		U2STAbits.OERR = 0;
	}

	// display on LEDs
	TRISA = 0;
	PORTA = x;

	// clear the interrupt
	IFS1bits.U2RXIF = 0;
}

void __attribute__((__interrupt__)) _DMA0Interrupt(void)
{	// When one buffer has been filled
	// Print both buffers
	int i;
	for (i = 0; i<UART2_BUFFER_SIZE; i++) {
		Uart2PrintChar(BufferA[i]);
	}
	for (i = 0; i<UART2_BUFFER_SIZE; i++) {
		Uart2PrintChar(BufferB[i]);
	}
	IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt Flag
}
/* 
 * File:   main.c
 * Author: Jesse
 *
 * Created on February 12, 2013, 11:17 AM
 */

#include <xc.h>

#define FCY 40000000
#define BAUDRATE 115200
#define BRGVAL ((FCY/BAUDRATE)/16)-1

unsigned int BufferA[8] __attribute__((space(dma)));

_FOSCSEL(FNOSC_FRC);
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF & POSCMD_XT);
_FWDT(FWDTEN_OFF);

unsigned int i; // counter

/*
 * 
 */
void main(void)
{
	// Watchdog Timer Enabled/disabled by user software
	// (LPRC can be disabled by clearing SWDTEN bit in RCON registe

	// Configure Oscillator to operate the device at 40Mhz
	// Fosc= Fin*M/(N1*N2), Fcy=Fosc/2
	// Fosc= 8M*40/(2*2)=80Mhz for 8M input clock
	PLLFBD = 38; // M=40
	CLKDIVbits.PLLPOST = 0; // N1=2
	CLKDIVbits.PLLPRE = 0; // N2=2
	OSCTUN = 0; // Tune FRC oscillator, if FRC is used

	RCONbits.SWDTEN = 0; /* Disable Watch Dog Timer*/

	// Clock switch to incorporate PLL
	__builtin_write_OSCCONH(0x03); // Initiate Clock Switch to Primary
	// Oscillator with PLL (NOSC=0b011)
	__builtin_write_OSCCONL(0x01); // Start clock switching
	while (OSCCONbits.COSC != 0b011); // Wait for Clock switch to occur

	while (OSCCONbits.LOCK != 1) {
	}; /* Wait for PLL to lock*/

	// Set up UART for Rx and Tx:
	U2MODEbits.STSEL = 0; // 1-stop bit
	U2MODEbits.PDSEL = 0; // No Parity, 8-data bits
	U2MODEbits.ABAUD = 0; // Auto-Baud Disabled

	U2BRG = BRGVAL; // Buad rate;

	U2STAbits.UTXISEL0 = 0; // Interrupt after one Tx character is transmitted
	U2STAbits.UTXISEL1 = 0;
	U2STAbits.URXISEL = 0; // Interrupt after one Rx character is recieved

	IEC1bits.U2RXIE = 0;
	IEC1bits.U2TXIE = 0;

	U2MODEbits.UARTEN = 1; // Enable UART
	U2STAbits.UTXEN = 1; // Enable UART Tx


	// Set Up DMA Channel 0 to Transmit in One-Shot, Single-Buffer Mode:
	DMA0CON = 0x0000; // Continuous, Post-Increment, RAM-to-Peripheral
	DMA0CNT = 8-1; // 8 DMA requests
	DMA0REQ = 0x001E; // Select UART2 Reciever

	DMA0PAD = (volatile unsigned int) &U2RXREG;
	DMA0STA = __builtin_dmaoffset(BufferA);

	IFS0bits.DMA0IF = 0;
	IEC0bits.DMA0IE = 1; // Enable DMA interrupt

	DMA0CONbits.CHEN = 1; // Enable DMA Channel

	TRISA = 0x0;
	PORTA = 0x01;

	while (1){
	}
}

//void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void)
//{
//	char x = U2RXREG;	// Read UxRXREG only once per interrupt!
//	U2TXREG = x;
//
//	// If there was an overun error clear it and continue
//	if (U2STAbits.OERR == 1) {
//		U2STAbits.OERR = 0;
//	}
//
//
//	TRISA = 0;
//	PORTA = x;
//	// clear the interrupt
//	IFS1bits.U2RXIF = 0;
//}

// Set Up DMA Interrupt Handlers:

void __attribute__((__interrupt__)) _DMA0Interrupt(void)
{
	IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt Flag
}


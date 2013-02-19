#include <xc.h>
#include "Uart2.h"

void Uart2Init(void)
{
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
}

void __attribute__((__interrupt__, no_auto_psv)) _U2RXInterrupt(void)
{
	char x = U2RXREG;	// Read UxRXREG only once per interrupt!
	U2TXREG = x;

	// If there was an overun error clear it and continue
	if (U2STAbits.OERR == 1) {
		U2STAbits.OERR = 0;
	}


	TRISA = 0;
	PORTA = x;
	// clear the interrupt
	IFS1bits.U2RXIF = 0;
}
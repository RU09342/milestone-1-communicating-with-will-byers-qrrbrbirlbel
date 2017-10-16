/**********************************************************************
*  Stranger Things RGB LED Node  						   		      *
*  Authors: Glenn Dawson, Joshua Gould, and Jack Pedicone             *
*  2017-10-16												          *
**********************************************************************/

#include <msp430f5529.h>				 
#define RED BIT2;					
#define GRN BIT3;			
#define BLU BIT4;			

void initializeGPIO();
void initializeUART();

volatile int byte = 0;	
volatile int num_bytes;	

void main(void)
{
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	initializeGPIO();				// Initialize timers and LEDs 
	initializeUART();			    // Initialize UART controls


	__bis_SR_register(LPM0_bits + GIE);		// Set low power mode
											// Enable global interrupts
}

#pragma vector=USCI_A0_VECTOR
/* * Accepts data on RX buffer, determines which byte in the message is 
	 being examined
   * Uses "bytes" and "rgb" as state machine control variables
   * Assumes byte 0 is total number of bytes in message
      * Sends value of leading byte - 3 to next node
   * Extracts PWM values from bytes 1, 2, 3
   * Assumes final byte is '/r'
       * Passes all unused bytes, including final byte, to next node
   * Credit: Nick Gorab	and Glenn Dawson							 */

__interrupt void USCI_A0_ISR(void)
{
	switch (__even_in_range(UCA0IV, USCI_UCTXIFG)) {
		case USCI_NONE: 
      		break;
		case USCI_UCRXIFG:
      		// Switch statement determines how to handle byte input
			switch (byte) {
                case 0:
					// Wait till UTXBUF is empty (UTXIFG1 = 1)
					while (!(UCA0IFG & UCTXIFG));   
						// Extract the total number of bytes
						num_bytes = UCA0RXBUF;
						// Subtract the number of bytes needed (3) 
						// from the total
						UCA0TXBUF = UCA0RXBUF - 3;  
						// Halts until something changes
						__no_operation();			
					break;
				case 1:
					// Red LED receives the first byte from the string
					TA0CCR1 = (UCA0RXBUF * 4); 			
					break;
				case 2:
					// Green LED receives the second byte from the string
					TA0CCR2 = (UCA0RXBUF * 4); 			
                	break;
				case 3:
					// Blue LED receives the third byte from the string
					TA0CCR3 = (UCA0RXBUF * 4); 			
                    break;
				default:
					// Pass all other bytes to TXBUF
                    while (!(UCA0IFG & UCTXIFG));   
					UCA0TXBUF = UCA0RXBUF;
            }
			
            // Conditional keeps track of byte handling in switch 
            // statement
            
            // If the current byte is not the final byte...
			if (byte < num_bytes - 1) { 
				// ...iterate byte.
				byte++;					
			}
			
			// If the current byte is the final byte...
            else if (byte == num_bytes - 1) { 
				// ...reset byte to get ready for the next string
				byte = 0;					  
            }
            break;
		case USCI_UCTXIFG: 
      		break;
		default: 
      		break;					
	}
}

void initializeGPIO(void) 				// Initialize LEDs and timers
{
	TA0CTL |= TASSEL_2 + MC_1 + TACLR; 	// 1 Mhz clock in Up mode
	TA0CCR0 = 1020;						// 1 kHz frequency

	P1DIR |= RED; //p1.4 to red
	P1SEL |= RED;
	P1DIR |= GRN; //p1.5 to green
	P1SEL |= GRN;
	P1DIR |= BLU; //p1.6 to blue
	P1SEL |= BLU;

	// Initialize LED_RED to OFF
	TA0CCTL1 = OUTMOD_7;				// OUTMODE reset/set
	TA0CCR1 = 0;						//CCR1 Default Zero

	// Initialize LED_GREEN to OFF
	TA0CCTL2 = OUTMOD_7;				// OUTMODE reset/set
	TA0CCR2 = 0;						//CCR2 Default Zero

    //Initialize LED_BLUE to OFF
	TA0CCTL3 = OUTMOD_7;				// OUTMODE reset/set
	TA0CCR3 = 0;						//CCR3 Default Zero
}

void initializeUART(void) 		// from Lab 1 example code
{
	P3SEL |= BIT3;      	// UART TX
	P3SEL |= BIT4;      	// UART RX
	UCA0CTL1 |= UCSWRST;   	// Resets state machine
	UCA0CTL1 |= UCSSEL_2;  	// SMCLK
	UCA0BR0 = 6;         	// 9600 Baud Rate
	UCA0BR1 = 0;         	// 9600 Baud Rate
	UCA0MCTL |= UCBRS_0;   	// Modulation
	UCA0MCTL |= UCBRF_13;  	// Modulation
	UCA0MCTL |= UCOS16;    	// Modulation
	UCA0CTL1 &= ~UCSWRST;   // Initializes the state machine
	UCA0IE |= UCRXIE;    	// Enables USCI_A0 RX Interrupt
}

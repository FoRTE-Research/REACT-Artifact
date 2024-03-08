#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "aes.h"
#include "test.h"

#include "caps.h"

// Past projects had this just copied and pasted into the main file, which irritates me
#include "aes_stuff.txt"

// Debounce and return on button release
// Use this little macro for readability since button pulls down
#define pressed !(P5IN & BIT6)
inline uint8_t s1Pressed(){
    uint16_t i;
    if(pressed){
        for(i = 5000; i > 0; i--){
            if(!pressed){
                return 0;
            }
        }
        while(pressed);
        return 1;
    }else{
        return 0;
    }
}
#undef pressed

#define pressed !(P5IN & BIT5)
inline uint8_t s2Pressed(){
    uint16_t i;
    if(pressed){
        for(i = 5000; i > 0; i--){
            if(!pressed){
                return 0;
            }
        }
        while(pressed);
        return 1;
    }else{
        return 0;
    }
}
#undef pressed

#pragma PERSISTENT (aesCount)
uint32_t aesCount = 0;

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	// ----- Configure GPIO -----
	// Outputs to show AES count is held or reset
	P1OUT &= ~(BIT0 | BIT1 | BIT2);
	P1DIR |= BIT0 | BIT1 | BIT2;

	// P5.5, P5.6 -> inputs with pull-up resistors
	P5DIR &= ~(BIT5 | BIT6);
	P5REN |= BIT5 | BIT6;
	P5OUT |= BIT5 | BIT6;

	// Use 2 pins for external control: go up, go down, do nothing
	// P7.0, P7.1 -> inputs with external pull-up resistors
	P7DIR &= ~(BIT0 | BIT1);

    // Set inputs for electrical compatibility with other benchmarks
    P1DIR &= ~(BIT3);
    P1REN |= BIT3;
    P1OUT &= ~BIT3;

    P3DIR &= ~(BIT4 | BIT5);
    P3REN |= BIT4 | BIT5;
    P3OUT &= ~(BIT4 | BIT5);

    // ----- Configure clocks -----
    CSCTL0_H = CSKEY_H;                     // Unlock CS
    CSCTL1 = DCOFSEL_0;                     // DCO -> 1 MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;    // SMCLK = MCLK = DCO, ACLK = VLOCLK = ~10 kHz
    CSCTL0_H = 0;                           // Lock CS
    __delay_cycles(100);

    __bis_SR_register(GIE);                 // Enable interrupts
    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Application code

    init_aes();

	while(1){
	    if(s1Pressed()){                    // Hold so we can attach debugger and view aesCount
	        P1OUT |= BIT0;
	        while(1);                       // Wait for debugger
	    }else if(s2Pressed()){              // Reset aesCount for experiments
	        aesCount = 0;
	        P1OUT |= BIT1;
	        while(1);                       // Wait for reset
	    }
	    else{
	        test_encrypt();
	        aesCount++;
	    }
	}
}

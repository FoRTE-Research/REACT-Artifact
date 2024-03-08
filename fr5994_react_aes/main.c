#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "aes.h"
#include "test.h"

#include "caps.h"

// Past projects had this just copied and pasted into the main file, which irritates me
#include "aes_stuff.txt"

CapBank bank0 = (CapBank) {
    .enPort = &P3OUT,
    .enMask = BIT6,
    .modePort = &P3OUT,
    .modeMask = BIT7
};

CapBank bank1 = (CapBank) {
    .enPort = &P3OUT,
    .enMask = BIT4,
    .modePort = &P3OUT,
    .modeMask = BIT5
};

CapBank bank2 = (CapBank) {
    .enPort = &P5OUT,
    .enMask = BIT0,
    .modePort = &P5OUT,
    .modeMask = BIT1
};

CapBank bank3 = (CapBank) {
    .enPort = &P4OUT,
    .enMask = BIT1,
    .modePort = &P4OUT,
    .modeMask = BIT2
};

CapBank bank4 = (CapBank) {
    .enPort = &P8OUT,
    .enMask = BIT1,
    .modePort = &P8OUT,
    .modeMask = BIT2
};

size_t bankState = 0;
size_t minBankState = 0;

// Array of pointers to all cap banks
// Organize banks from smallest to largest here!!!
CapBank* cbpArray[] = {&bank0, &bank1, &bank2, &bank3, &bank4};

#define STEP_COOLDOWN 3
int stepCooldown = 0;

// Increase capacitance by smallest available step
// Returns 1 on success, 0 on failure
//inline uint8_t stepCapUp(){
uint8_t stepCapUp(){
    size_t i;
    for(i = 0; i < sizeof(cbpArray) / sizeof(cbpArray[0]); i++){
        if(expandCapBank3State(cbpArray[i])){
            stepCooldown = STEP_COOLDOWN;
            bankState++;
            return 1;
        }
    }
    return 0;
}

// Decrease capacitance by smallest available step
// Returns 1 on success, 0 on failure
//inline uint8_t stepCapDown(){
uint8_t stepCapDown(){
    size_t i;
    for(i = (sizeof(cbpArray) / sizeof(cbpArray[0])); i--> 0;){
//    for(i = 0; i < sizeof(cbpArray) / sizeof(cbpArray[0]); i++){
        if(contractCapBank3State(cbpArray[i])){
            stepCooldown = STEP_COOLDOWN;
            bankState--;
            return 1;
        }
    }
    return 0;
}

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

// Limit for runtime testing, remove for full traces
#define AES_LIMIT 50

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

    // ----- Configure clocks -----
    CSCTL0_H = CSKEY_H;                     // Unlock CS
    CSCTL1 = DCOFSEL_0;                     // DCO -> 1 MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;    // SMCLK = MCLK = DCO, ACLK = VLOCLK = ~10 kHz
    CSCTL0_H = 0;                           // Lock CS
    __delay_cycles(100);

    // ----- Configure capacitor measurement timer -----
    // ACLK, up mode, clear TAR, enable interrupt
    TA0CCTL0 = CCIE;
//    TA0CCR0 = 500;                          // 10 kHz clock -> 0.05 second period
    TA0CCR0 = 1000;                          // 10 kHz clock -> 0.1 second period
    TA0CTL = TASSEL__ACLK | MC__UP;

    __bis_SR_register(GIE);                 // Enable interrupts

	// Init all cap banks
	size_t i;
    for(i = sizeof(cbpArray) / sizeof(cbpArray[0]); i--> 0;){
        initCapBank(cbpArray[i]);
    }

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
	        if(aesCount < AES_LIMIT){
	            test_encrypt();
	            P1OUT ^= BIT2;
	            aesCount++;
	        }else{
	            P1OUT |= BIT0 | BIT2;
	        }
	    }
	}
}

// Timer0_A1 Interrupt Vector (TAIV) handler
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    if(stepCooldown){
        stepCooldown--;
        return;
    }
    if(~P7IN & BIT0){         // P7.0 connected to low voltage comparator. Low indicates voltage < 2.0V
        stepCapDown();
    } else if(P7IN & BIT1){     // P7.1 connected to high voltage comparator. High indicates voltage > 3.4V
        stepCapUp();
    }
}

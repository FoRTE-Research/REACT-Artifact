#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "caps.h"

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

// Decreases bank capacitance:
// CONNECTED_PAR -> CONNECTED_SER -> DISCONNECTED_SER
// Returns 1 on successful bank contraction, 0 if bank is already min size
// Doesn't support DISCONNECTED_PAR. Will sit in while(1) if it encounters it
inline uint8_t contractCapBank3StateSpecialRtFunction(CapBank* bank){
    switch(bank->state){
        case CONNECTED_PAR:
//            setCapBankState(bank, CONNECTED_SER);
            return 1;
        case CONNECTED_SER:
//            setCapBankState(bank, DISCONNECTED_SER);
            return 1;
        case DISCONNECTED_SER:
            return 0;
        default:
            while(1);
    }
}

// Decrease capacitance by smallest available step
// Returns 1 on success, 0 on failure
//inline uint8_t stepCapDown(){
uint8_t stepCapDown(){
    size_t i;
    for(i = (sizeof(cbpArray) / sizeof(cbpArray[0])); i--> 0;){
//    for(i = 0; i < sizeof(cbpArray) / sizeof(cbpArray[0]); i++){
//        if(contractCapBank3State(cbpArray[i])){
        if(contractCapBank3StateSpecialRtFunction(cbpArray[i])){
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

#pragma PERSISTENT (txCount)
uint32_t txCount = 0;

#define CAP_MEASURE_TIMER 500       // 10 kHz clock -> 0.05 second period
#define TX_FREQUENCY_SEC  1
#define TX_COUNTER_ROLLOVER (10000 / CAP_MEASURE_TIMER) * TX_FREQUENCY_SEC

#define PACKET_SIZE 128             // Bits
#define DATA_RATE 136               // Kbps

// Number of cycles to delay for tx assuming MCU running at 1 MHz
//#define TX_TIME(clockRate) (clockRate / 1000) * PACKET_SIZE / DATA_RATE
#define TX_TIMER_PERIOD 4000          // 10 kHz clock -> 300 ms

typedef enum{
    WAIT,
    SEND,
} State_t;

State_t state = WAIT;

uint16_t txWaitCounter = 0;

int txDone = 0;

//#define STATIC
#define DYNAMIC

int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

	// ----- Configure GPIO -----
    // Default state for all is low power
    P1OUT = 0;
    P1DIR = 0xFF;
    P2OUT = 0;
    P2DIR = 0xFF;
    P3OUT = 0;
    P3DIR = 0xFF;
    P4OUT = 0;
    P4DIR = 0xFF;
    P5OUT = 0;
    P5DIR = 0xFF;
    P6OUT = 0;
    P6DIR = 0xFF;
    P7OUT = 0;
    P7DIR = 0xFF;
    P8DIR = 0xFF;
    P8OUT = 0;
    PJOUT = 0;
    PJDIR = 0xFFFF;

	// Outputs to show sample count is held or reset
	P1OUT &= ~(BIT0 | BIT1);
	P1DIR |= BIT0 | BIT1;

	// P5.5, P5.6 -> inputs with pull-up resistors
	P5DIR &= ~(BIT5 | BIT6);
	P5REN |= BIT5 | BIT6;
	P5OUT |= BIT5 | BIT6;

	// Use 2 pins for external control: go up, go down, do nothing
	// P7.0, P7.1 -> inputs with external pull-up resistors
	P7DIR &= ~(BIT0 | BIT1);

	// P1.2 is ADC pin
	P1SEL1 |= BIT2;
	P1SEL0 |= BIT2;

	// Output to power radio
	P6OUT &= ~BIT0;
	P6DIR |= BIT0;

    // ----- Configure clocks -----
    CSCTL0_H = CSKEY_H;                     // Unlock CS
    CSCTL1 = DCOFSEL_0;                     // DCO -> 1 MHz
    CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;    // SMCLK = MCLK = DCO, ACLK = VLOCLK = ~10 kHz
    CSCTL0_H = 0;                           // Lock CS
    __delay_cycles(100);

    // ----- Configure capacitor measurement timer -----
    // ACLK, up mode, clear TAR, enable interrupt
    TA0CCTL0 = CCIE;
    TA0CCR0 = CAP_MEASURE_TIMER;            // 10 kHz clock -> 0.05 second period
    TA0CTL = TASSEL__ACLK | MC__UP;

    // ----- Configure tx timer -----
    TA1CCR0 = TX_TIMER_PERIOD;
    TA1CTL = TASSEL__ACLK | MC__UP;

    __bis_SR_register(GIE);                 // Enable interrupts

	// Init all cap banks
	size_t i;
    for(i = sizeof(cbpArray) / sizeof(cbpArray[0]); i--> 0;){
        initCapBank(cbpArray[i]);
    }

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Application code

	while(1){
	    if(s1Pressed()){                    // Hold so we can attach debugger and view txCount
	        P1OUT |= BIT0;
	        while(1);                       // Wait for debugger
	    }else if(s2Pressed()){              // Reset txCount for experiments
	        txCount = 0;
	        P1OUT |= BIT1;
	        while(1);                       // Wait for reset
	    }
	    else{
	        switch(state){
	        case WAIT:
                #ifdef STATIC
	            state = SEND;
                #elif defined(DYNAMIC)
	            minBankState = 5;
	            __bis_SR_register(LPM3_bits | GIE); // Timebase will force exit
                #endif
	        break;
	        case SEND:
//	            TA0CCTL0 &= ~CCIE;          // Disable capacitor measurement timer
                txDone = 0;
                TA1R = 0;                   // Reset tx timer
                TA1CCTL0 &= ~CCIFG;

//	            P6OUT |= BIT0;              // Enable "radio" - 680 resistor
                P6OUT |= BIT0;              // Replaced with 220 for testing
	            TA1CCTL0 |= CCIE;           // Enable tx timer
	            while(!txDone);
	            P6OUT &= ~BIT0;             // Disable radio
	            txCount++;

//                TA0CCTL0 |= CCIE;          // Enable capacitor measurement timer
	            state = WAIT;
	        break;
	        default:
	            while(1);
	        }
	    }
	}
}

// Timer0_A0 Interrupt Vector (TAIV) handler
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER0_A0_VECTOR
__interrupt void TIMER0_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    txWaitCounter++;
    if(stepCooldown){
        stepCooldown--;
        return;
    }
    if(~P7IN & BIT0){         // P7.0 connected to low voltage comparator. Low indicates voltage < 2.0V
        stepCapDown();
//        __no_operation();
    } else if(P7IN & BIT1){     // P7.1 connected to high voltage comparator. High indicates voltage < 3.4V
        // If enough energy stored, exit LPM
        #ifdef DYNAMIC
        if(bankState >= minBankState && state == WAIT){
            state = SEND;
            __bic_SR_register_on_exit(LPM3_bits); // Begin running
            return;
        }
        #endif
        stepCapUp();
    }
}

#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_10_VECTOR))) TIMER0_A1_ISR (void)
#else
#error Compiler not supported!
#endif
{
    txDone = 1;
    TA1CCTL0 &= ~CCIE;   // Disable tx timer
}

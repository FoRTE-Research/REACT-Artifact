#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

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
#pragma PERSISTENT (rxCount)
uint32_t rxCount = 0;

#pragma PERSISTENT (toForward)
uint32_t toForward = 0;

int rxDone, txDone;

#define CAP_MEASURE_TIMER 1000       // 10 kHz clock -> 0.01 second period

#define TX_TIMER_PERIOD 3000          // 10 kHz clock -> 300 ms
#define RX_TIMER_PERIOD 1500

#define canTx() P1IN & BIT3
#define canRx() P3IN & BIT4
#define incomingPacket() P3IN & BIT5

typedef enum{
    WAIT,
    READY_TO_RECEIVE,
    RECEIVE,
    TRANSMIT
} State_t;

State_t state = WAIT;

//#define STATIC
#define DYNAMIC

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

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

    // P1.3 receives TX power good signal
    P1DIR &= ~BIT3;
    P1REN |= BIT3;
    P1OUT &= ~BIT3;

    // P3.4 receives RX power good signal
    // P3.5 receives signal to receive packet
    P3DIR &= ~(BIT4 | BIT5);
    P3REN |= BIT4 | BIT5;
    P3OUT &= ~(BIT4 | BIT5);

    // Use 2 pins for external control: go up, go down, do nothing
    // P7.0, P7.1 -> inputs with external pull-up resistors
    P7DIR &= ~(BIT0 | BIT1);

    // P1.2 is ADC pin
    P1SEL1 |= BIT2;
    P1SEL0 |= BIT2;

    // Output to power radio: 6.0 is tx, 6.1 is rx
    P6OUT &= ~(BIT0 | BIT1);
    P6DIR |= BIT0 | BIT1;

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

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Application code

    while(1){
        if(s1Pressed()){                    // Hold so we can attach debugger and view txCount
            P1OUT |= BIT0;
            while(1);                       // Wait for debugger
        }else if(s2Pressed()){              // Reset txCount for experiments
            txCount = 0;
            rxCount = 0;
            toForward = 0;
            P1OUT |= BIT1;
            while(1);                       // Wait for reset
        }
        else{
            switch(state){
            case WAIT:
                __bis_SR_register(LPM3_bits | GIE); // Timebase will force exit
            break;
            case READY_TO_RECEIVE:
                __bis_SR_register(LPM3_bits | GIE); // Shouldn't really get here in active mode.
            break;
            case RECEIVE:
                // Set up timer for rx
                TA1CCR0 = RX_TIMER_PERIOD;
                TA1R = 0;
                TA1CCTL0 &= ~CCIFG;
                rxDone = 0;
                P6OUT |= BIT1;                      // Enable "rx" - 820 resistor
                TA1CCTL0 |= CCIE;
                while(!rxDone);
                P6OUT &= ~BIT1;
                rxCount++;
                toForward++;
                state = WAIT;
            case TRANSMIT:
                // Set up timer for tx
                TA1CCR0 = TX_TIMER_PERIOD;
                TA1R = 0;
                TA1CCTL0 &= ~CCIFG;
                txDone = 0;
                P6OUT |= BIT0;                      // Enable "tx" - 220 resistor
                TA1CCTL0 |= CCIE;
                while(!txDone);
                P6OUT &= ~BIT0;
                txCount++;
                toForward--;
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
    switch(state){
    case WAIT:
        if(incomingPacket() && canRx()){
            state = READY_TO_RECEIVE;
        }else if(toForward > 0 && canTx()){
//        }else if(toForward > 0){
            state = TRANSMIT;
            __bic_SR_register_on_exit(LPM3_bits);   // If a packet is buffered and we have sufficient energy, start tx
        }
        break;
    case READY_TO_RECEIVE:
        if(!(incomingPacket())){
            state = RECEIVE;
            __bic_SR_register_on_exit(LPM3_bits);   // Begin rx on incomingPacket negedge
        }
        break;
    case RECEIVE:
        // Interrupt fired during rx. Nothing to do
        break;
    case TRANSMIT:
        // Interrupt fired during tx. Nothing to do
        break;
    default:
        while(1);   // Shouldn't happen
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
    rxDone = 1;
    txDone = 1;
}

#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include "morphy.h"

#define STEP_COOLDOWN 3
int stepCooldown = STEP_COOLDOWN;

size_t timebase50ms = 0;

#define TIMER_PERIOD 5000       // Milliseconds
#define UP_TIME 500             // UP_TIME / TIMER_PERIOD = duty cycle

// Assumes 50ms counts
#define UP_TIME_COUNTS UP_TIME / 50
#define DOWN_TIME_COUNTS (TIMER_PERIOD - UP_TIME) / 50

#define txEnergyGood() P2OUT |= BIT6
#define txEnergyBad() P2OUT &= ~BIT6
#define rxEnergyGood() P2OUT |= BIT5
#define rxEnergyBad() P2OUT &= ~BIT5

typedef enum{
    LOW,
    HIGH
} State_t;

State_t state = LOW;

Morphy_states_t morphyState;
/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	// LED blink
	P1DIR |= BIT0;
	P1OUT &= ~(BIT0);

	// Comparator inputs with pull-ups
	P3DIR &= ~(BIT4 | BIT5);
	P3REN |= BIT4 | BIT5;
	P3OUT |= BIT4 | BIT5;

	// Timer output
    P5OUT &= ~BIT7;
    P5DIR |= BIT7;

    // TX energy good output
    P2OUT &= ~BIT6;
    P2DIR |= BIT6;

    // RX energy good output
    P2OUT &= ~BIT5;
    P2DIR |= BIT5;

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

    // ----- Configure measurement helper timer -----
    // ACLK, up mode, clear TAR, enable interrupt
    TA1CCTL0 = CCIE;
    TA1CCR0 = 500;                          // 10 kHz clock -> 0.05 second period
    TA1CTL = TASSEL__ACLK | MC__UP;

    __bis_SR_register(GIE);                 // Enable interrupts

    // Init Morphy to be as small as possible
	morphyGpioInit();
    morphyState = MORPHY_8SERIES;
	morphySetState(morphyState);

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Morphy starts operation in 1/8 mode to minimize capacitance.
    // To smooth voltage fluctuations during switching, however, we need to take at least one
    // capacitor out of the "dynamic bank". So realistically we have 7 capacitors to mess with.
    // One will always be in parallel.

//    morphySetCapOneOverEight();
//    morphySetCapOneOverTwo();
//    morphySetCapTwo();
//    morphySetCapEight();

	while(1){
//	    __delay_cycles(500000);
        switch(state){
        case LOW:
            if(timebase50ms >= DOWN_TIME_COUNTS){
                P5OUT |= BIT7;
                state = HIGH;
                timebase50ms = 0;
                P1OUT ^= BIT0;
            }
        break;
        case HIGH:
            if(timebase50ms >= UP_TIME_COUNTS){
                P5OUT &= ~BIT7;
                state = LOW;
                timebase50ms = 0;
            }
        break;
        default:
            while(1);
        break;
        }
	}
}

// Timer1_A0 Interrupt Vector (TAIV) handler
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=TIMER1_A0_VECTOR
__interrupt void TIMER1_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(TIMER0_A0_VECTOR))) TIMER0_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    timebase50ms++;
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
    stepCooldown = STEP_COOLDOWN;
    // Minimum capacitance charged is enough to successfully RX
//    rxEnergyGood();
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // PUT THIS BACK FOR PACKET FORWARDING
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    if(~P3IN & BIT5){         // P3.5 connected to low voltage comparator. Low indicates voltage < 2.0V
        switch(morphyState){
        case MORPHY_1X_VOLTAGE:
            morphyState = MORPHY_2X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyGood();
        break;
        case MORPHY_2X_VOLTAGE:
            morphyState = MORPHY_4X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyGood();
        break;
        case MORPHY_4X_VOLTAGE:
            morphyState = MORPHY_7X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_7X_VOLTAGE:
            morphyState = MORPHY_2SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_2SERIES:
            morphyState = MORPHY_3SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_3SERIES:
            morphyState = MORPHY_4SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_4SERIES:
            morphyState = MORPHY_5SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_5SERIES:
            morphyState = MORPHY_6SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_6SERIES:
            morphyState = MORPHY_7SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_7SERIES:
            morphyState = MORPHY_8SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_8SERIES:
        break;
        default:
            while(1);           // Shouldn't happen
//        break;
        }
    } else if(P3IN & BIT4){     // P3.4 connected to high voltage comparator. High indicates voltage > 3.4V
        switch(morphyState){
        case MORPHY_1X_VOLTAGE:
        break;
        case MORPHY_2X_VOLTAGE:
            morphyState = MORPHY_1X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyGood();
        break;
        case MORPHY_4X_VOLTAGE:
            morphyState = MORPHY_2X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyGood();
        break;
        case MORPHY_7X_VOLTAGE:
            morphyState = MORPHY_4X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyGood();
        break;
        case MORPHY_2SERIES:
            morphyState = MORPHY_7X_VOLTAGE;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_3SERIES:
            morphyState = MORPHY_2SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_4SERIES:
            morphyState = MORPHY_3SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_5SERIES:
            morphyState = MORPHY_4SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_6SERIES:
            morphyState = MORPHY_5SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_7SERIES:
            morphyState = MORPHY_6SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        case MORPHY_8SERIES:
            morphyState = MORPHY_7SERIES;
            morphySetState(morphyState);
            txEnergyBad();
        break;
        default:
            while(1);           // Shouldn't happen
//        break;
        }
    }
}

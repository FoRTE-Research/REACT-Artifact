#include <msp430.h>
#include "morphy.h"

// Init everything to disconnected as in labeled figure on 1/31
void morphyGpioInit(){
    P1OUT &= ~(BIT3 | BIT4 | BIT5);
    P3OUT &= ~(BIT0 | BIT1 | BIT2 | BIT3 | BIT6 | BIT7);
    P4OUT &= ~(BIT1 | BIT2 | BIT7);
    P5OUT &= ~(BIT2);
    P6OUT &= ~(BIT0 | BIT1 | BIT2);

    P1DIR |= BIT3 | BIT4 | BIT5;
    P3DIR |= BIT0 | BIT1 | BIT2 | BIT3 | BIT6 | BIT7;
    P4DIR |= BIT1 | BIT2 | BIT7;
    P5DIR |= BIT2;
    P6DIR |= BIT0 | BIT1 | BIT2;
}

// Disconnect all caps, typically in preparation for a reconfiguration
void morphyDisconnectAll(){
    MORPHY_S1_CLR();
    MORPHY_S2_CLR();
    MORPHY_S3_CLR();
    MORPHY_S4_CLR();
    MORPHY_S5_CLR();
    MORPHY_S6_CLR();
    MORPHY_S7_CLR();
    MORPHY_S8_CLR();
    MORPHY_S9_CLR();
    MORPHY_S10_CLR();
    MORPHY_S11_CLR();
    MORPHY_S12_CLR();
    MORPHY_S13_CLR();
    MORPHY_S14_CLR();
    MORPHY_S15_CLR();
    MORPHY_S16_CLR();
}

//MORPHY_1X_VOLTAGE,
//MORPHY_2X_VOLTAGE,
//MORPHY_4X_VOLTAGE,
//MORPHY_7X_VOLTAGE,
//MORPHY_2SERIES,
//MORPHY_3SERIES,
//MORPHY_4SERIES,
//MORPHY_5SERIES,
//MORPHY_6SERIES,
//MORPHY_7SERIES,
//MORPHY_8SERIES

// This assumes we are using one capacitor as the task capacitor
void morphySetState(Morphy_states_t newState){
    switch(newState){
    case MORPHY_1X_VOLTAGE:     // All caps in parallel: 8C
        morphyDisconnectAll();

        MORPHY_S1_SET();
        MORPHY_S3_SET();
        MORPHY_S5_SET();
        MORPHY_S7_SET();
        MORPHY_S9_SET();
        MORPHY_S11_SET();
        MORPHY_S13_SET();
        MORPHY_S15_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_2X_VOLTAGE:     // C || (3C - 4C) = 12C/7
        morphyDisconnectAll();

        MORPHY_S1_SET();
        MORPHY_S3_SET();
        MORPHY_S6_SET();
        MORPHY_S7_SET();
        MORPHY_S9_SET();
        MORPHY_S11_SET();
        MORPHY_S15_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_4X_VOLTAGE:     // C || (C-2C-2C-2C) = 7C/5
        morphyDisconnectAll();

        MORPHY_S1_SET();
        MORPHY_S4_SET();
        MORPHY_S5_SET();
        MORPHY_S8_SET();
        MORPHY_S9_SET();
        MORPHY_S12_SET();
//        MORPHY_S13_SET();
        MORPHY_S15_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_7X_VOLTAGE:     // C || (C-C-C-C-C-C-C) = 8C/7
        morphyDisconnectAll();
        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S6_SET();
        MORPHY_S8_SET();
        MORPHY_S10_SET();
        MORPHY_S12_SET();
        MORPHY_S14_SET();
        MORPHY_S15_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_2SERIES:        // C/2
        morphyDisconnectAll();

        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_3SERIES:        // C/3
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_4SERIES:        // C/4
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_5SERIES:        // C/5
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S6_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_6SERIES:        // C/6
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S6_SET();
        MORPHY_S8_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_7SERIES:        // C/7
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S6_SET();
        MORPHY_S8_SET();
        MORPHY_S10_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    case MORPHY_8SERIES:        // C/8
        morphyDisconnectAll();

        MORPHY_S2_SET();
        MORPHY_S4_SET();
        MORPHY_S6_SET();
        MORPHY_S8_SET();
        MORPHY_S10_SET();
        MORPHY_S12_SET();
        MORPHY_S14_SET();
        MORPHY_S16_SET();
    break;
    default:
        __no_operation();
    break;
    }
}

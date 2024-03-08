/*
 * morphy.h
 *
 *  Created on: Jan 31, 2023
 *      Author: harrison
 */

#ifndef MORPHY_H_
#define MORPHY_H_

// Implementation of Morphy figure 6 for varying capacitance
typedef enum {
    MORPHY_1X_VOLTAGE,
    MORPHY_2X_VOLTAGE,
    MORPHY_4X_VOLTAGE,
    MORPHY_7X_VOLTAGE,
    MORPHY_2SERIES,
    MORPHY_3SERIES,
    MORPHY_4SERIES,
    MORPHY_5SERIES,
    MORPHY_6SERIES,
    MORPHY_7SERIES,
    MORPHY_8SERIES
} Morphy_states_t;

#define MORPHY_S1_SET() P3OUT |= BIT0
#define MORPHY_S1_CLR() P3OUT &= ~BIT0

#define MORPHY_S2_SET() P3OUT |= BIT1
#define MORPHY_S2_CLR() P3OUT &= ~BIT1

#define MORPHY_S3_SET() P3OUT |= BIT2
#define MORPHY_S3_CLR() P3OUT &= ~BIT2

#define MORPHY_S4_SET() P3OUT |= BIT3
#define MORPHY_S4_CLR() P3OUT &= ~BIT3

#define MORPHY_S5_SET() P1OUT |= BIT4
#define MORPHY_S5_CLR() P1OUT &= ~BIT4

#define MORPHY_S6_SET() P1OUT |= BIT5
#define MORPHY_S6_CLR() P1OUT &= ~BIT5

#define MORPHY_S7_SET() P4OUT |= BIT7
#define MORPHY_S7_CLR() P4OUT &= ~BIT7

#define MORPHY_S8_SET() P6OUT |= BIT1
#define MORPHY_S8_CLR() P6OUT &= ~BIT1

#define MORPHY_S9_SET() P6OUT |= BIT0
#define MORPHY_S9_CLR() P6OUT &= ~BIT0

#define MORPHY_S10_SET() P6OUT |= BIT2
#define MORPHY_S10_CLR() P6OUT &= ~BIT2

#define MORPHY_S11_SET() P1OUT |= BIT3
#define MORPHY_S11_CLR() P1OUT &= ~BIT3

#define MORPHY_S12_SET() P5OUT |= BIT2
#define MORPHY_S12_CLR() P5OUT &= ~BIT2

#define MORPHY_S13_SET() P4OUT |= BIT1
#define MORPHY_S13_CLR() P4OUT &= ~BIT1

#define MORPHY_S14_SET() P4OUT |= BIT2
#define MORPHY_S14_CLR() P4OUT &= ~BIT2

#define MORPHY_S15_SET() P3OUT |= BIT7
#define MORPHY_S15_CLR() P3OUT &= ~BIT7

#define MORPHY_S16_SET() P3OUT |= BIT6
#define MORPHY_S16_CLR() P3OUT |= BIT6


void morphyGpioInit();
void morphyDisconnectAll();

void morphySetState(Morphy_states_t newState);

#endif /* MORPHY_H_ */

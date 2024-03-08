#include <msp430.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "caps.h"

#define SIGNAL_LEN 128
#define FILTER_LEN 4

// FIR taps. It doesn't matter what they are
const uint16_t fir[] = {1, 2, 3, 4};

void convolve(uint16_t Signal[SIGNAL_LEN],
              uint16_t Kernel[FILTER_LEN],
              uint16_t Result[SIGNAL_LEN + FILTER_LEN - 1])
{
  size_t n;

  for (n = 0; n < SIGNAL_LEN + FILTER_LEN - 1; n++)
  {
    size_t kmin, kmax, k;

    Result[n] = 0;

    kmin = (n >= FILTER_LEN - 1) ? n - (FILTER_LEN - 1) : 0;
    kmax = (n < SIGNAL_LEN - 1) ? n : SIGNAL_LEN - 1;

    for (k = kmin; k <= kmax; k++)
    {
      Result[n] += Signal[k] * Kernel[n - k];
    }
  }
}


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

#pragma PERSISTENT (sampleCount)
uint32_t sampleCount = 0;

#define CAP_MEASURE_TIMER 1000    // 10 kHz clock -> 0.10 second period
#define SENSE_PERIOD 5            // Seconds

#define SENSE_ROLLOVER (10000 / CAP_MEASURE_TIMER) * SENSE_PERIOD
uint16_t senseCounter = 0;
uint16_t adcSamples = 0;
uint16_t adcVoltage = 0;
uint16_t sampleBuffer[SIGNAL_LEN];

typedef enum{
    WAIT,
    SENSE,
    FILTER
} State_t;

State_t state = WAIT;

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

	// P1.3 gets signal to take sample
	// Input with pulldown
	P1DIR &= ~(BIT3);
	P1REN |= BIT3;
	P1OUT &= ~BIT3;

	// Output to power microphone
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

    // ----- Configure ADC reference -----
    // By default, REFMSTR=1 => REFCTL is used to configure the internal reference
//    while(REFCTL0 & REFGENBUSY);            // If ref generator busy, WAIT
//    REFCTL0 |= REFVSEL_0 | REFON;           // Select internal ref = 1.2V
//                                            // Internal Reference ON
    REFCTL0 |= REFVSEL_0;

    // ----- Configure ADC -----
    ADC12CTL0 = ADC12SHT0_2 | ADC12ON;
    ADC12CTL1 = ADC12SHP;                   // ADCCLK = MODOSC; sampling timer
    ADC12CTL2 |= ADC12RES_2;                // 12-bit conversion results
    ADC12IER0 |= ADC12IE0;                  // Enable ADC conv complete interrupt
    ADC12MCTL0 |= ADC12INCH_2 | ADC12VRSEL_1; // A1 ADC input select; Vref=1.2V

    __bis_SR_register(GIE);                 // Enable interrupts

	// Init all cap banks
	size_t i;
    for(i = sizeof(cbpArray) / sizeof(cbpArray[0]); i--> 0;){
        initCapBank(cbpArray[i]);
    }

    PM5CTL0 &= ~LOCKLPM5;                   // Disable the GPIO power-on default high-impedance mode
                                            // to activate previously configured port settings

    // Application code

    volatile uint16_t result[SIGNAL_LEN + FILTER_LEN - 1];

	while(1){
	    if(s1Pressed()){                    // Hold so we can attach debugger and view sampleCount
	        P1OUT |= BIT0;
	        while(1);                       // Wait for debugger
	    }else if(s2Pressed()){              // Reset sampleCount for experiments
	        sampleCount = 0;
	        P1OUT |= BIT1;
	        while(1);                       // Wait for reset
	    }
	    else{
	        switch(state){
	        case WAIT:
	            __bis_SR_register(LPM3_bits | GIE); // Timebase will force exit
	        break;
	        case SENSE:
	            P6OUT |= BIT0;                      // Enable "microphone" - 12k resistor
	            ADC12CTL0 |= ADC12ENC | ADC12SC;    // Sampling and conversion start
	            __bis_SR_register(LPM3_bits | GIE); // LPM3, ADC12_ISR will force exit
	        break;
	        case FILTER:
	            convolve(sampleBuffer, fir, result);
	            sampleCount++;
                __bis_SR_register(LPM3_bits | GIE); // Timebase will force exit
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
    if(state == WAIT && (P1IN & BIT3)){             // If P1.3 high, we've received signal to take a sample
        state = SENSE;
        __bic_SR_register_on_exit(LPM3_bits); // Begin running
    }else if(state == FILTER && (~P1IN & BIT3)){    // If P1.3 goes low after we've completed a sample, get ready to take another
        state = WAIT;
        __bic_SR_register_on_exit(LPM3_bits); // Begin running
    }
}

// ADC12 ISR
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector = ADC12_B_VECTOR
__interrupt void ADC12_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(ADC12_B_VECTOR))) ADC12_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch (__even_in_range(ADC12IV, ADC12IV__ADC12RDYIFG))
    {
        case ADC12IV__NONE:        break;   // Vector  0:  No interrupt
        case ADC12IV__ADC12OVIFG:  break;   // Vector  2:  ADC12MEMx Overflow
        case ADC12IV__ADC12TOVIFG: break;   // Vector  4:  Conversion time overflow
        case ADC12IV__ADC12HIIFG:  break;   // Vector  6:  ADC12BHI
        case ADC12IV__ADC12LOIFG:  break;   // Vector  8:  ADC12BLO
        case ADC12IV__ADC12INIFG:  break;   // Vector 10:  ADC12BIN
        case ADC12IV__ADC12IFG0:            // Vector 12:  ADC12MEM0 Interrupt
            adcVoltage = 1200 * (uint32_t) ADC12MEM0 / 4095;   // Millivolts. Order of operations important here to prevent overflow
            sampleBuffer[adcSamples] = adcVoltage;
            adcSamples++;
            if(adcSamples >= SIGNAL_LEN){
                adcSamples = 0;
                P6OUT &= ~BIT0; // Disable microphone
                state = FILTER;
            }
            __bic_SR_register_on_exit(LPM3_bits); // Begin running
            break;                          // Clear CPUOFF bit from 0(SR)
        case ADC12IV__ADC12IFG1:   break;   // Vector 14:  ADC12MEM1
        case ADC12IV__ADC12IFG2:   break;   // Vector 16:  ADC12MEM2
        case ADC12IV__ADC12IFG3:   break;   // Vector 18:  ADC12MEM3
        case ADC12IV__ADC12IFG4:   break;   // Vector 20:  ADC12MEM4
        case ADC12IV__ADC12IFG5:   break;   // Vector 22:  ADC12MEM5
        case ADC12IV__ADC12IFG6:   break;   // Vector 24:  ADC12MEM6
        case ADC12IV__ADC12IFG7:   break;   // Vector 26:  ADC12MEM7
        case ADC12IV__ADC12IFG8:   break;   // Vector 28:  ADC12MEM8
        case ADC12IV__ADC12IFG9:   break;   // Vector 30:  ADC12MEM9
        case ADC12IV__ADC12IFG10:  break;   // Vector 32:  ADC12MEM10
        case ADC12IV__ADC12IFG11:  break;   // Vector 34:  ADC12MEM11
        case ADC12IV__ADC12IFG12:  break;   // Vector 36:  ADC12MEM12
        case ADC12IV__ADC12IFG13:  break;   // Vector 38:  ADC12MEM13
        case ADC12IV__ADC12IFG14:  break;   // Vector 40:  ADC12MEM14
        case ADC12IV__ADC12IFG15:  break;   // Vector 42:  ADC12MEM15
        case ADC12IV__ADC12IFG16:  break;   // Vector 44:  ADC12MEM16
        case ADC12IV__ADC12IFG17:  break;   // Vector 46:  ADC12MEM17
        case ADC12IV__ADC12IFG18:  break;   // Vector 48:  ADC12MEM18
        case ADC12IV__ADC12IFG19:  break;   // Vector 50:  ADC12MEM19
        case ADC12IV__ADC12IFG20:  break;   // Vector 52:  ADC12MEM20
        case ADC12IV__ADC12IFG21:  break;   // Vector 54:  ADC12MEM21
        case ADC12IV__ADC12IFG22:  break;   // Vector 56:  ADC12MEM22
        case ADC12IV__ADC12IFG23:  break;   // Vector 58:  ADC12MEM23
        case ADC12IV__ADC12IFG24:  break;   // Vector 60:  ADC12MEM24
        case ADC12IV__ADC12IFG25:  break;   // Vector 62:  ADC12MEM25
        case ADC12IV__ADC12IFG26:  break;   // Vector 64:  ADC12MEM26
        case ADC12IV__ADC12IFG27:  break;   // Vector 66:  ADC12MEM27
        case ADC12IV__ADC12IFG28:  break;   // Vector 68:  ADC12MEM28
        case ADC12IV__ADC12IFG29:  break;   // Vector 70:  ADC12MEM29
        case ADC12IV__ADC12IFG30:  break;   // Vector 72:  ADC12MEM30
        case ADC12IV__ADC12IFG31:  break;   // Vector 74:  ADC12MEM31
        case ADC12IV__ADC12RDYIFG: break;   // Vector 76:  ADC12RDY
        default: break;
    }
}

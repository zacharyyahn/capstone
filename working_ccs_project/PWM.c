#include "PWM.h"


void PWM_Init () {
    // Currently configures for linear defense only
    // P6.0 -> LDef_IN, P6.1 -> LOff_IN

    // Sets as output
    P6->DIR |= 0x03;

    // Configure as IO
    P6->SEL0 &= ~0x03;
    P6->SEL1 &= ~0x03;

    // Configure TimerA0
    TIMER_A0->CCR[0] = 11999;  // sets CCR0 to period - 1

    // bit8 = 0 set compare mode, bit4 = 1 enables interrupt
    TIMER_A0->CCTL[1] = TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE;

    // Initialize CCR1 to zero
    TIMER_A0->CCR[1] = 0x0000;

    // bit9-8 = 10 to set SMCLK, bit5-4 = 01 to set up mode
    TIMER_A0->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1;

    // Enable interrupt 9 and 8 for timer A0 CCR0 and TAIFG; datasheet (p. 118)
    NVIC->ISER[0] |= 0x00000300;
    // Priority set in top three bits of register
    NVIC->IP[9] = (0x04 << 5);
    NVIC->IP[8] = (0x04 << 5);


}

// duty must be 0-11998 inclusive
void PWM_SetDutyLDef (uint16_t duty) {
    if (duty >= 11999) return;
    TIMER_A0->CCR[1] = duty;
}

void TA0_0_IRQHandler() {

    if(TIMER_A0->CCR[1] == 0) {
        P6->OUT &= ~0x01;
    } else {
        P6->OUT |= 0x01;
    }
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

void TA0_N_IRQHandler() {
    P6->OUT &= ~0x01;
    TIMER_A0->CCTL[1] &= ~TIMER_A_CCTLN_CCIFG;
}

/*
 * SwitchReader.c
 *
 *  Created on: Nov 14, 2022
 *      Author: jalon
 */

#include "SwitchReader.h"

void SwitchReader_Init() {

    // GPIO Configuration
    SWITCH_INPUT_PORT->DIR &= ~ALL_SWITCH_BITS;

    SWITCH_INPUT_PORT->SEL0 &= ~ALL_SWITCH_BITS;
    SWITCH_INPUT_PORT->SEL1 &= ~ALL_SWITCH_BITS;

    // SWITCH_INPUT_PORT->OUT

    // Halt timer A1 and configure
    TIMER_A1->CTL = TIMER_A_CTL_MC_0;

    // Enable interrupt on CCR0
    TIMER_A1->CCTL[0] = TIMER_A_CCTLN_CCIE;

    // 12MHz / (4 * CCR0) = 100 Hz; CCR0 = 30000 dec
    TIMER_A1->CCR[0] = 0x7530;

    // Enable interrupt 10 for TA1 CCR0; datasheet (p. 118)
    NVIC->ISER[0] |= 0x00000400;
    // Priority set in top three bits of register
    NVIC->IP[TA1_0_IRQn] = (0x04 << 5);

    // bit9-8 = 10 to set SMCLK, bit5-4 = 01 to set up mode, bit7 = 1 to set CLK divider to /4
    TIMER_A1->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1 | TIMER_A_CTL_ID_2;

    // Reset before beginning use
    TIMER_A1->CTL |= TIMER_A_CTL_CLR;

    // Initialize the inuput image to zero
    switch_image = 0x00;
}

void TA1_0_IRQHandler() {
    switch_image = SWITCH_INPUT_PORT->IN;
    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}


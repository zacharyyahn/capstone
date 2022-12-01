/*
 * Encoder.c
 *
 *  Created on: Nov 30, 2022
 *      Author: jalon
 */

#include "Encoder.h"

void Encoder_Init() {

    // Configure encoder signal pins as input
    LDEF_ENC_PORT->DIR &= ~(ENC_A_BIT | ENC_B_BIT);
    LOFF_ENC_PORT->DIR &= ~(ENC_A_BIT | ENC_B_BIT);
    RDEF_ENC_PORT->DIR &= ~(ENC_A_BIT | ENC_B_BIT);
    ROFF_ENC_PORT->DIR &= ~(ENC_A_BIT | ENC_B_BIT);

    LDEF_ENC_PORT->SEL0 &= ~(ENC_A_BIT | ENC_B_BIT);
    LOFF_ENC_PORT->SEL0 &= ~(ENC_A_BIT | ENC_B_BIT);
    RDEF_ENC_PORT->SEL0 &= ~(ENC_A_BIT | ENC_B_BIT);
    ROFF_ENC_PORT->SEL0 &= ~(ENC_A_BIT | ENC_B_BIT);

    LDEF_ENC_PORT->SEL1 &= ~(ENC_A_BIT | ENC_B_BIT);
    LOFF_ENC_PORT->SEL1 &= ~(ENC_A_BIT | ENC_B_BIT);
    RDEF_ENC_PORT->SEL1 &= ~(ENC_A_BIT | ENC_B_BIT);
    ROFF_ENC_PORT->SEL1 &= ~(ENC_A_BIT | ENC_B_BIT);

    // Enable interrupts on encoder ports
    LDEF_ENC_PORT->IE |= (ENC_A_BIT | ENC_B_BIT);
    LOFF_ENC_PORT->IE |= (ENC_A_BIT | ENC_B_BIT);
    RDEF_ENC_PORT->IE |= (ENC_A_BIT | ENC_B_BIT);
    ROFF_ENC_PORT->IE |= (ENC_A_BIT | ENC_B_BIT);

    // Init on rising edge and toggle in ISR
    LDEF_ENC_PORT->IES &= ~(ENC_A_BIT | ENC_B_BIT);
    LOFF_ENC_PORT->IES &= ~(ENC_A_BIT | ENC_B_BIT);
    RDEF_ENC_PORT->IES &= ~(ENC_A_BIT | ENC_B_BIT);
    ROFF_ENC_PORT->IES &= ~(ENC_A_BIT | ENC_B_BIT);

    // Enable interrupt in NVIC
    NVIC->ISER[1] |= (0x00000001 << (PORT1_IRQn - 32));
    NVIC->ISER[1] |= (0x00000001 << (PORT2_IRQn - 32));
    NVIC->ISER[1] |= (0x00000001 << (PORT3_IRQn - 32));
    NVIC->ISER[1] |= (0x00000001 << (PORT4_IRQn - 32));

    NVIC->IP[PORT1_IRQn] =  (0x04 << 5);
    NVIC->IP[PORT2_IRQn] =  (0x04 << 5);
    NVIC->IP[PORT3_IRQn] =  (0x04 << 5);
    NVIC->IP[PORT4_IRQn] =  (0x04 << 5);

}

void RDEF_IRQ() {
    P3->OUT |= BIT0;
    // Check current state vs. stored state to determine forward vs back
    // increment or decrement count
    uint8_t state = RDEF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
        case STATE_11:
            if (RDef_Encoder.state == STATE_10) {
                RDef_Encoder.count--;
            } else if (RDef_Encoder.state == STATE_01) {
                RDef_Encoder.count++;
            }
            break;
        case STATE_10:
            if (RDef_Encoder.state == STATE_00) {
                RDef_Encoder.count--;
            } else if (RDef_Encoder.state == STATE_11) {
                RDef_Encoder.count++;
            }
            break;
        case STATE_01:
            if (RDef_Encoder.state == STATE_11) {
                RDef_Encoder.count--;
            } else if (RDef_Encoder.state == STATE_00) {
                RDef_Encoder.count++;
            }
            break;
        case STATE_00:
            if (RDef_Encoder.state == STATE_01) {
                RDef_Encoder.count--;
            } else if (RDef_Encoder.state == STATE_10) {
                RDef_Encoder.count++;
            }
            break;
        default:
            break;
    }

    // Toggle IES
    RDEF_ENC_PORT->IES ^= (state ^ RDef_Encoder.state);

    // Update state
    RDef_Encoder.state = state;

    // Clear interrupt flag
    RDEF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
    P3->OUT &= ~BIT0;
}

void ROFF_IRQ() {

}

void LDEF_IRQ() {
    // Check current state vs. stored state to determine forward vs back
    // increment or decrement count
    uint8_t state = LDEF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
        case STATE_11:
            if (LDef_Encoder.state == STATE_10) {
                LDef_Encoder.count--;
            } else if (LDef_Encoder.state == STATE_01) {
                LDef_Encoder.count++;
            }
            break;
        case STATE_10:
            if (LDef_Encoder.state == STATE_00) {
                LDef_Encoder.count--;
            } else if (LDef_Encoder.state == STATE_11) {
                LDef_Encoder.count++;
            }
            break;
        case STATE_01:
            if (LDef_Encoder.state == STATE_11) {
                LDef_Encoder.count--;
            } else if (LDef_Encoder.state == STATE_00) {
                LDef_Encoder.count++;
            }
            break;
        case STATE_00:
            if (LDef_Encoder.state == STATE_01) {
                LDef_Encoder.count--;
            } else if (LDef_Encoder.state == STATE_10) {
                LDef_Encoder.count++;
            }
            break;
        default:
            break;
    }

    // Toggle IES
    LDEF_ENC_PORT->IES ^= (state ^ LDef_Encoder.state);

    // Update state
    LDef_Encoder.state = state;

    // Clear interrupt flag
    LDEF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
}

void LOFF_IRQ() {

}



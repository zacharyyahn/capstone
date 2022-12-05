/*
 * Encoder.c
 *
 *  Created on: Nov 30, 2022
 *      Author: jalon
 */

#include "Encoder.h"

struct encoder LDef_Encoder, LOff_Encoder, RDef_Encoder, ROff_Encoder;

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
    enum encoder_state state = RDEF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
    case STATE_11:
        switch (RDef_Encoder.state) {
        case STATE_10:
            RDef_Encoder.count--;
            break;
        case STATE_01:
            RDef_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_10:
		switch (RDef_Encoder.state) {
        case STATE_00:
            RDef_Encoder.count--;
			break;
        case STATE_11:
            RDef_Encoder.count++;
			break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_01:
		switch (RDef_Encoder.state) {
        case STATE_11:
            RDef_Encoder.count--;
			break;
        case STATE_00:
            RDef_Encoder.count++;
            break;
        default:
			// unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_00:
		switch (RDef_Encoder.state) {
        case STATE_01:
            RDef_Encoder.count--;
			break;
        case STATE_10:
            RDef_Encoder.count++;
			break;
        default:
			// unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    default:
        break;
    } // end switch new state

    // Toggle IES
    RDEF_ENC_PORT->IES ^= (state ^ RDef_Encoder.state);

    // Update state
    RDef_Encoder.state = state;

    // Clear interrupt flag
    RDEF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
    P3->OUT &= ~BIT0;
}

void ROFF_IRQ() {
    // Check current state vs. stored state to determine forward vs back
    // increment or decrement count
    enum encoder_state state = ROFF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
    case STATE_11:
        switch (ROff_Encoder.state) {
        case STATE_10:
            ROff_Encoder.count--;
            break;
        case STATE_01:
            ROff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_10:
        switch (ROff_Encoder.state) {
        case STATE_00:
            ROff_Encoder.count--;
            break;
        case STATE_11:
            ROff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_01:
        switch (ROff_Encoder.state) {
        case STATE_11:
            ROff_Encoder.count--;
            break;
        case STATE_00:
            ROff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_00:
        switch (ROff_Encoder.state) {
        case STATE_01:
            ROff_Encoder.count--;
            break;
        case STATE_10:
            ROff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    default:
        break;
    } // end switch new state

    // Toggle IES
    ROFF_ENC_PORT->IES ^= (state ^ ROff_Encoder.state);

    // Update state
    ROff_Encoder.state = state;

    // Clear interrupt flag
    ROFF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
}

void LDEF_IRQ() {
    // Check current state vs. stored state to determine forward vs back
    // increment or decrement count
    enum encoder_state state = LDEF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
    case STATE_11:
        switch (LDef_Encoder.state) {
        case STATE_10:
            LDef_Encoder.count--;
			break;
        case STATE_01:
            LDef_Encoder.count++;
			break;
		default:
			// unreachable unless we miss an encoder edge
			break;
        } // end switch old state
        break;
    case STATE_10:
        switch (LDef_Encoder.state) {
        case STATE_00:
            LDef_Encoder.count--;
			break;
        case STATE_11:
            LDef_Encoder.count++;
			break;
		default:
			// unreachable unless we miss an encoder edge
			break;
        } // end switch old state
        break;
    case STATE_01:
        switch (LDef_Encoder.state) {
        case STATE_11:
            LDef_Encoder.count--;
			break;
        case STATE_00:
            LDef_Encoder.count++;
			break;
		default:
			// unreachable unless we miss an encoder edge
			break;
        } // end switch old state
        break;
    case STATE_00:
        switch (LDef_Encoder.state) {
        case STATE_01:
            LDef_Encoder.count--;
			break;
        case STATE_10:
            LDef_Encoder.count++;
			break;
		default:
			// unreachable unless we miss an encoder edge
			break;
        } // end switch old state
        break;
    default:
        break;
    } // end switch new state

    // Toggle IES
    LDEF_ENC_PORT->IES ^= (state ^ LDef_Encoder.state);

    // Update state
    LDef_Encoder.state = state;

    // Clear interrupt flag
    LDEF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
}

void LOFF_IRQ() {
    // Check current state vs. stored state to determine forward vs back
    // increment or decrement count
    enum encoder_state state = LOFF_ENC_PORT->IN & (ENC_A_BIT | ENC_B_BIT);
    switch (state) {
    case STATE_11:
        switch (LOff_Encoder.state) {
        case STATE_10:
            LOff_Encoder.count--;
            break;
        case STATE_01:
            LOff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_10:
        switch (LOff_Encoder.state) {
        case STATE_00:
            LOff_Encoder.count--;
            break;
        case STATE_11:
            LOff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_01:
        switch (LOff_Encoder.state) {
        case STATE_11:
            LOff_Encoder.count--;
            break;
        case STATE_00:
            LOff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    case STATE_00:
        switch (LOff_Encoder.state) {
        case STATE_01:
            LOff_Encoder.count--;
            break;
        case STATE_10:
            LOff_Encoder.count++;
            break;
        default:
            // unreachable unless we miss an encoder edge
            break;
        } // end switch old state
        break;
    default:
        break;
    } // end switch new state

    // Toggle IES
    LOFF_ENC_PORT->IES ^= (state ^ LOff_Encoder.state);

    // Update state
    LOff_Encoder.state = state;

    // Clear interrupt flag
    LOFF_ENC_PORT->IFG &= ~(ENC_A_BIT | ENC_B_BIT);
}



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
}

uint8_t ReadSwitches() {
    // all switches are active low but optical are usually active
    return ((~SWITCH_INPUT_PORT->IN) & (LLSDEF1_BIT | LLSDEF2_BIT | LLSOFF1_BIT | LLSOFF2_BIT)) |
           (SWITCH_INPUT_PORT->IN & (RLSDEF_BIT | RLSOFF_BIT));
}

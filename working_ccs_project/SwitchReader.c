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
    // all switches are active low
    return (~SWITCH_INPUT_PORT->IN) & ALL_SWITCH_BITS;
}

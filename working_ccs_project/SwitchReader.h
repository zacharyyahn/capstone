/*
 * SwitchReader.h
 *
 *  Created on: Nov 14, 2022
 *      Author: jalon
 */

#ifndef SWITCHREADER_H_
#define SWITCHREADER_H_

#include "msp.h"

#define SWITCH_INPUT_PORT       P5
#define RLSDEF_BIT              0x01
#define RLSOFF_BIT              0x02
#define LLSDEF1_BIT             0x10
#define LLSDEF2_BIT             0x20
#define LLSOFF1_BIT             0x40
#define LLSOFF2_BIT             0x80
#define ALL_SWITCH_BITS         (RLSDEF_BIT | RLSOFF_BIT | LLSDEF1_BIT | LLSDEF2_BIT | LLSOFF1_BIT | LLSOFF2_BIT)

void SwitchReader_Init();
uint8_t ReadSwitches();

#endif /* SWITCHREADER_H_ */

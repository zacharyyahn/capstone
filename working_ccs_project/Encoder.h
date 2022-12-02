/*
 * Encoder.h
 *
 *  Created on: Nov 30, 2022
 *      Author: jalon
 */

#ifndef ENCODER_H_
#define ENCODER_H_

#include "msp.h"

#define RDEF_ENC_PORT P1
#define ROFF_ENC_PORT P2
#define LDEF_ENC_PORT P3
#define LOFF_ENC_PORT P4

#define RDEF_IRQ PORT1_IRQHandler
#define ROFF_IRQ PORT2_IRQHandler
#define LDEF_IRQ PORT3_IRQHandler
#define LOFF_IRQ PORT4_IRQHandler

#define ENC_A_BIT BIT6
#define ENC_B_BIT BIT7

enum encoder_state {
    STATE_11 = 0xC0,
    STATE_10 = 0x80,
    STATE_01 = 0x40,
    STATE_00 = 0x00,
};

struct encoder_t {
    int16_t count;
    enum encoder_state state;
};

struct encoder_t LDef_Encoder;
struct encoder_t LOff_Encoder;
struct encoder_t RDef_Encoder;
struct encoder_t ROff_Encoder;

void Encoder_Init();


#endif /* ENCODER_H_ */

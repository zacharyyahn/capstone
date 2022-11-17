/*
 * PWM.h
 *
 *  Created on: Nov 8, 2022
 *      Author: jalon
 */

#ifndef PWM_H_
#define PWM_H_

#include "msp.h"

// Pin and Port definitions for PWM
#define LINEAR_CONTROL_PORT         P6
#define ROTATIONAL_CONTROL_PORT     P4

#define LDEF_IN1_BIT                0x01
#define LDEF_IN2_BIT                0x02
#define LOFF_IN1_BIT                0x10
#define LOFF_IN2_BIT                0x20
#define RDEF_IN1_BIT                0x01
#define RDEF_IN2_BIT                0x02
#define ROFF_IN1_BIT                0x04
#define ROFF_IN2_BIT                0x08

#define LDEF_CCR_INDEX 1
#define LOFF_CCR_INDEX 2
#define RDEF_CCR_INDEX 3
#define ROFF_CCR_INDEX 4

// Direction bits for each motor; 1 = forwards
#define FORWARD 1
#define REVERSE 0

void PWM_Init();

void SetDuty_LDef(uint16_t duty);
void SetDuty_LOff(uint16_t duty);
void SetDuty_RDef(uint16_t duty);
void SetDuty_ROff(uint16_t duty);

void SetDir_LDef(uint8_t direction);
void SetDir_LOff(uint8_t direction);
void SetDir_RDef(uint8_t direction);
void SetDir_ROff(uint8_t direction);


#endif /* PWM_H_ */

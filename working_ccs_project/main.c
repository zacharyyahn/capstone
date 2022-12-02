/*
 * main.c
 *
 * Test project to receive UART input from simulator. Transmitted data
 * represents the eventual output of the raspberry pi following image processing.
 *
 * Technical Ref - www.ti.com - MSP432P4xx Technical Reference Manual
 * Datasheet - www.ti.com - MSP432P401R Mixed Signal MCs datasheet
 *
 */

#include "msp.h"
#include "UART.h"
#include "PWM.h"
#include "SwitchReader.h"
#include "Clock.h"
#include "CortexM.h"
#include "Encoder.h"

extern uint32_t desired_defense_position, desired_offense_position;
extern enum rotational_state defense_rotstate, offense_rotstate;

int main(void)
{
    // Stop watchdog timer; technical reference (p. 585)
    WDT_A->CTL = 0x0080 | 0x5A00;

    // configure P3.0 as output for debug
    P3->DIR |= BIT0;
    P3->SEL0 &= ~BIT0;
    P3->SEL1 &= ~BIT0;

    // Red LED at P1.0 as output for debug
    P1->DIR |= BIT0;
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;

    Clock_Init48MHz();
    PWM_Init();
    SwitchReader_Init();
    Encoder_Init();
    UART_A0_Init();

    SetDuty_RDef(0);
    SetDuty_LDef(0);
    SetDuty_ROff(0);
    SetDuty_LOff(0);


    EnableInterrupts();

    while (1) {
        if (desired_defense_position == 4000 && defense_rotstate == BLOCK &&
                desired_offense_position == 4000 && offense_rotstate == BLOCK) {
            P1->OUT |= BIT0;
        } else {
            P1->OUT &= ~BIT0;
        }
    }
}


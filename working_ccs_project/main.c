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
#include "StallWatchdog.h"

// commands from Pi
extern uint16_t desired_defense_position, desired_offense_position;
extern enum rotational_state defense_rotstate, offense_rotstate;

// actual motor states
extern struct encoder LDef_Encoder, LOff_Encoder, RDef_Encoder, ROff_Encoder;

// limit switches
uint8_t switch_image;

// encoder count ranges, measured by calibration routine
uint16_t linear_encoder_range;
uint16_t rotational_encoder_360_deg;

// top-level state
extern enum main_state_enum main_state;

unsigned int ProportionalControl (int error) {
    unsigned int abs_error = error >= 0 ? error : 0-error;
    if (abs_error < 5) {
        return 0;
    } else if (abs_error >= 200) {
        return 6000;
    } else {
        // min speed 4000 at 5 error, max speed 6000 at 200 error
        return 4000 + (abs_error - 5 ) * 2000 / 195;
    }
}

void CalibrateEncoders() {
    // find 0 position
    SetDir_LOff(REVERSE);
    SetDuty_LOff(4000);

    while (!(ReadSwitches() & LLSOFF1_BIT)) { }
    Stop_LOff();
    LOff_Encoder.count = 0;

    // find max position
    SetDir_LOff(FORWARD);
    SetDuty_LOff(4000);

    while (!(ReadSwitches() & LLSOFF2_BIT)) { }
    Stop_LOff();
    linear_encoder_range = LOff_Encoder.count;

    char out_buffer[2];
    out_buffer[0] = (char) linear_encoder_range;
    out_buffer[1] = (char) (linear_encoder_range >> 8);
    UART_ToPi(out_buffer);
}

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
    P1->OUT &= ~BIT0;

    Clock_Init48MHz();
    PWM_Init();
    SwitchReader_Init();
    Encoder_Init();
    UART_A0_Init();
    StallWatchdog_Init();

    EnableInterrupts();

 
    Stop_LOff();
    Stop_ROff();
    Stop_LDef();
    Stop_RDef();
    main_state = WAIT;


    while (1) {
        // wait for start signal from Pi
        while (main_state == WAIT) {
            Stop_LOff();
            Stop_ROff();
            Stop_LDef();
            Stop_RDef();
        }

        // calibration routine
        if (main_state != CALIBRATE) break;
        CalibrateEncoders();
        while (main_state == CALIBRATE) {
            SetDir_LOff(LOff_Encoder.count < (linear_encoder_range >> 1) ? FORWARD : REVERSE);
            SetDuty_LOff(ProportionalControl(LOff_Encoder.count - (linear_encoder_range >> 1)));
        }

        // play the game
        while (main_state == PLAY) {
            // calibrate encoder counts from limit switches
            switch_image = ReadSwitches();
            if (switch_image & RLSDEF_BIT)  RDef_Encoder.count = 0;
            if (switch_image & RLSOFF_BIT)  ROff_Encoder.count = 0;
            if (switch_image & LLSDEF1_BIT) LDef_Encoder.count = 0;
            if (switch_image & LLSDEF2_BIT) LDef_Encoder.count = linear_encoder_range;
            if (switch_image & LLSOFF1_BIT) LOff_Encoder.count = 0;
            if (switch_image & LLSOFF2_BIT) LOff_Encoder.count = linear_encoder_range;

            SetDir_LOff(LOff_Encoder.count < desired_offense_position ? FORWARD : REVERSE);
            SetDuty_LOff(ProportionalControl(LOff_Encoder.count - desired_offense_position));
        }
    }
    Stop_LOff();
    Stop_ROff();
    Stop_LDef();
    Stop_RDef();
}


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

// Motor control tuning macros
#define MIN_LINEAR_SPEED                3000        // Linear min duty (0 - 11998)
#define MAX_LINEAR_SPEED                10000        // Linear max duty (0 - 11998)
#define MIN_ROTATIONAL_SPEED            3000        // Rotational min duty (0 - 11998)
#define MAX_ROTATIONAL_SPEED            10000        // Rotational max duty (0 - 11998)
#define LINEAR_CONTROL_TOLERANCE        5           // Linear motor acceptable desired vs actual encoder delta
#define ROTATIONAL_CONTROL_TOLERANCE    10          // Rotational motor acceptable desired vs actual encoder delta
#define MAX_SPEED_ERROR_THRESHOLD       200         // Linear motor encoder delta above which motor will run at MAX


// Commands from Pi
extern uint16_t desired_defense_position, desired_offense_position;
extern enum rotational_state defense_rotstate, offense_rotstate;

// Top-level state
extern enum main_state_enum main_state;

// Actual motor states
extern struct encoder LDef_Encoder, LOff_Encoder, RDef_Encoder, ROff_Encoder;

// Stall state of motors
extern enum stall_state_enum stall_state;
extern int16_t stall_position;

// Limit switches
uint8_t switch_image;

// Encoder count ranges, measured by calibration routine
uint16_t loff_encoder_range, ldef_encoder_range;
uint16_t roff_encoder_360_deg, rdef_encoder_360_deg;

enum shoot_state {
    WIND_UP,
    KICK,
} offense_shoot_state, defense_shoot_state;

enum calibrate_state {
    FIND_MIN,
    FIND_MAX,
    EXIT_NOTCH,
    GOTO_DEFAULT,
} ldef_calibrate_state, loff_calibrate_state, rdef_calibrate_state, roff_calibrate_state;


// Function declarations
unsigned int LinearControl(int error);
unsigned int RotationalControl(int error);
void SetInitialStates();
void ConfigureDebugGPIO();
void CalibrateEncoders(uint8_t switch_image);

int main(void)
{
    // Stop watchdog timer; technical reference (p. 585)
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;

    // Inits
    Clock_Init48MHz();
    PWM_Init();
    SwitchReader_Init();
    Encoder_Init();
    UART_A0_Init();
    StallWatchdog_Init();
    SetInitialStates();
    ConfigureDebugGPIO();

    EnableInterrupts();

    // Should already be off
    Stop_All_Motors();

    // MSP should start in WAIT state
    main_state = WAIT;

    // Outer superloop, checks overall MSP state
    while (1) {

        /***************************** WAIT ******************************/

        // Wait for start signal from Pi
        while (main_state == WAIT) {
            Stop_All_Motors();
            SetInitialStates();
        }

        /*************************** CALIBRATE ***************************/

        // Calibration Routine
        // Linear motor max encoder count information to send to pi
        char out_buffer[4];
        int unsent_linear_encoder_ranges = 0;

        while (main_state == CALIBRATE) {

            // LDEF calibration
            switch (ldef_calibrate_state) {
            case FIND_MIN:
                // Reverse until switch at position 0 is hit; set count and transition to FIND_MAX
                if (!(ReadSwitches() & LLSDEF1_BIT)) {
                    SetDir_LDef(REVERSE);
                    SetDuty_LDef(MIN_LINEAR_SPEED);
                } else {
                    Stop_LDef();
                    LDef_Encoder.count = 0;
                    ldef_calibrate_state = FIND_MAX;
                }
                break;
            case FIND_MAX:
                // Go forward until switch at max position is hit; set count to send to pi and transition to GOTO_DEFAULT
                if (!(ReadSwitches() & LLSDEF2_BIT)) {
                    SetDir_LDef(FORWARD);
                    SetDuty_LDef(MIN_LINEAR_SPEED);
                } else {
                    Stop_LDef();
                    ldef_encoder_range = LDef_Encoder.count;

                    out_buffer[0] = (char) ldef_encoder_range;
                    out_buffer[1] = (char) (ldef_encoder_range >> 8);
                    unsent_linear_encoder_ranges++;

                    ldef_calibrate_state = GOTO_DEFAULT;
                }
                break;
            case GOTO_DEFAULT:
                // Move motor to center of assembly
                SetDir_LDef(LDef_Encoder.count < (ldef_encoder_range >> 1) ? FORWARD : REVERSE);
                SetDuty_LDef(LinearControl(LDef_Encoder.count - (ldef_encoder_range >> 1)));
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

            // LOFF calibration
            switch (loff_calibrate_state) {
            case FIND_MIN:
                // Reverse until switch at position 0 is hit; set count and transition to FIND_MAX
                if (!(ReadSwitches() & LLSOFF1_BIT)) {
                    SetDir_LOff(REVERSE);
                    SetDuty_LOff(MIN_LINEAR_SPEED);
                } else {
                    Stop_LOff();
                    LOff_Encoder.count = 0;
                    loff_calibrate_state = FIND_MAX;
                }
                break;
            case FIND_MAX:
                // Go forward until switch at max position is it; set count to send to pi and transition to GOTO_DEFAULT
                if (!(ReadSwitches() & LLSOFF2_BIT)) {
                    SetDir_LOff(FORWARD);
                    SetDuty_LOff(MIN_LINEAR_SPEED);
                } else {
                    Stop_LOff();
                    loff_encoder_range = LOff_Encoder.count;

                    out_buffer[2] = (char) loff_encoder_range;
                    out_buffer[3] = (char) (loff_encoder_range >> 8);
                    unsent_linear_encoder_ranges++;

                    loff_calibrate_state = GOTO_DEFAULT;
                }
                break;
            case GOTO_DEFAULT:
                // Move motor to center of assembly
                SetDir_LOff(LOff_Encoder.count < (loff_encoder_range >> 1) ? FORWARD : REVERSE);
                SetDuty_LOff(LinearControl(LOff_Encoder.count - (loff_encoder_range >> 1)));
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

            // RDEF calibration
            switch (rdef_calibrate_state) {
            case FIND_MIN:
                // Turn motor forwards (cw from robot player perspective) until switch position (pos 90deg) is reached
                // Zero count and transition to EXIT_NOTCH
                if (!(ReadSwitches() & RLSDEF_BIT)) {
                    SetDir_RDef(FORWARD);
                    SetDuty_RDef(MIN_ROTATIONAL_SPEED);
                } else {
                    RDef_Encoder.count = 0;
                    rdef_calibrate_state = EXIT_NOTCH;
                }
                break;
            case EXIT_NOTCH:
                // Turn motor until optical interrupter is no longer high; transition to FIND_MAX
                if (ReadSwitches() & RLSDEF_BIT) {
                    SetDir_RDef(FORWARD);
                    SetDuty_RDef(MIN_ROTATIONAL_SPEED);
                } else {
                    rdef_calibrate_state = FIND_MAX;
                }
                break;
            case FIND_MAX:
                // Complete one full revolution and save number of encoder counts; adjust count such that 0 pos is vertical players
                // Transition to GOTO_DEFAULT
                if (!(ReadSwitches() & RLSDEF_BIT)) {
                    SetDir_RDef(FORWARD);
                    SetDuty_RDef(MIN_ROTATIONAL_SPEED);
                } else {
                    rdef_encoder_360_deg = RDef_Encoder.count;
                    RDef_Encoder.count = rdef_encoder_360_deg >> 2;
                    rdef_calibrate_state = GOTO_DEFAULT;
                }
                break;
            case GOTO_DEFAULT:
                // Move players to vertical position
                SetDir_RDef(RDef_Encoder.count < 0 ? FORWARD : REVERSE);
                SetDuty_RDef(RotationalControl(RDef_Encoder.count));
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

            // ROFF calibration
            switch (roff_calibrate_state) {
            case FIND_MIN:
                // Turn motor forwards (cw from robot player perspective) until switch position (pos 90deg) is reached
                // Zero count and transition to EXIT_NOTCH
                if (!(ReadSwitches() & RLSOFF_BIT)) {
                    SetDir_ROff(FORWARD);
                    SetDuty_ROff(MIN_ROTATIONAL_SPEED);
                } else {
                    ROff_Encoder.count = 0;
                    roff_calibrate_state = EXIT_NOTCH;
                }
                break;
            case EXIT_NOTCH:
                // Turn motor until optical interrupter is no longer high; transition to FIND_MAX
                if (ReadSwitches() & RLSOFF_BIT) {
                    SetDir_ROff(FORWARD);
                    SetDuty_ROff(MIN_ROTATIONAL_SPEED);
                } else {
                    roff_calibrate_state = FIND_MAX;
                }
                break;
            case FIND_MAX:
                // Complete one full revolution and save number of encoder counts; adjust count such that 0 pos is vertical players
                // Transition to GOTO_DEFAULT
                if (!(ReadSwitches() & RLSOFF_BIT)) {
                    SetDir_ROff(FORWARD);
                    SetDuty_ROff(MIN_ROTATIONAL_SPEED);
                } else {
                    // Normalize encoder count to zero at vertical
                    roff_encoder_360_deg = ROff_Encoder.count;
                    ROff_Encoder.count = roff_encoder_360_deg >> 2;
                    roff_calibrate_state = GOTO_DEFAULT;
                }
                break;
            case GOTO_DEFAULT:
                // Move players to vertical position
                SetDir_ROff(ROff_Encoder.count < 0 ? FORWARD : REVERSE);
                SetDuty_ROff(RotationalControl(ROff_Encoder.count));
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

            // Send max encoder counts for linear motors if both ready
            if (unsent_linear_encoder_ranges == 2) {
               UART_ToPi(out_buffer);
               unsent_linear_encoder_ranges = 0;
            }
        }

        /*************************** GAMEPLAY ***************************/

        // Play the game, flip the bits ;P
        while (main_state == PLAY) {
            // Calibrate encoder counts from limit switches
            switch_image = ReadSwitches();
            CalibrateEncoders(switch_image);

            // Move LOFF motor to desired position
            SetDir_LOff(LOff_Encoder.count < desired_offense_position ? FORWARD : REVERSE);
            SetDuty_LOff(LinearControl(LOff_Encoder.count - desired_offense_position));

            // Move LDEF motor to desired position
            SetDir_LDef(LDef_Encoder.count < desired_defense_position ? FORWARD : REVERSE);
            SetDuty_LDef(LinearControl(LDef_Encoder.count - desired_defense_position));

            // Control ROFF based on desired offense_rotstate from pi
            // WIND_UP is set at end of each rotstate such that SHOOT state will always begin with a wind up
            switch (offense_rotstate) {
            case BLOCK:
                // Move players to vertical position
                SetDir_ROff(ROff_Encoder.count < 0 ? FORWARD : REVERSE);
                SetDuty_ROff(RotationalControl(ROff_Encoder.count));
                offense_shoot_state = WIND_UP;
                break;
            case READY:
                // Move players to pos 90deg
                SetDir_ROff(ROff_Encoder.count < (roff_encoder_360_deg >> 2) ? FORWARD : REVERSE);
                SetDuty_ROff(RotationalControl(ROff_Encoder.count - (roff_encoder_360_deg >> 2)));
                offense_shoot_state = WIND_UP;
                break;
            case FANCY_SHOOT:
            case SHOOT:
                // Switch on shoot state
                switch (offense_shoot_state) {
                case WIND_UP:
                    // Wind up until at 45deg pos - transition to KICK if in position
                    if (ROff_Encoder.count > (roff_encoder_360_deg >> 3)) {
                        offense_shoot_state = KICK;
                    } else {
                        SetDir_ROff(FORWARD);
                        SetDuty_ROff(MAX_ROTATIONAL_SPEED);
                    }
                    break;
                case KICK:
                    // Kick until at -45deg pos - transition to WIND_UP if kick is complete
                    if (ROff_Encoder.count < 0 - (roff_encoder_360_deg >> 3)) {
                        offense_shoot_state = WIND_UP;
                    } else {
                        SetDir_ROff(REVERSE);
                        SetDuty_ROff(MAX_ROTATIONAL_SPEED);
                    }
                    break;
                }
                break;

            case SPIN:
                // SPIN
                SetDuty_ROff(MAX_ROTATIONAL_SPEED);
                SetDir_ROff(REVERSE);
                offense_shoot_state = WIND_UP;
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

            // Control RDEF based on desired defense_rotstate from pi
            // WIND_UP is set at end of each rotstate such that SHOOT state will always begin with a wind up
            switch (defense_rotstate) {
            case BLOCK:
                // Move players to vertical position
                SetDir_RDef(RDef_Encoder.count < 0 ? FORWARD : REVERSE);
                SetDuty_RDef(RotationalControl(RDef_Encoder.count));
                defense_shoot_state = WIND_UP;
                break;
            case READY:
                // Move players to pos 90deg
                SetDir_RDef(RDef_Encoder.count < (rdef_encoder_360_deg >> 2) ? FORWARD : REVERSE);
                SetDuty_RDef(RotationalControl(RDef_Encoder.count - (rdef_encoder_360_deg >> 2)));
                defense_shoot_state = WIND_UP;
                break;
            case FANCY_SHOOT:
            case SHOOT:
                // Switch on shoot state
                switch (defense_shoot_state) {
                case WIND_UP:
                    // Wind up until at 45deg pos - transition to KICK if in position
                    if (RDef_Encoder.count > (rdef_encoder_360_deg >> 3)) {
                        defense_shoot_state = KICK;
                    } else {
                        SetDir_RDef(FORWARD);
                        SetDuty_RDef(MAX_ROTATIONAL_SPEED);
                    }
                    break;
                case KICK:
                    // Kick until at -45deg pos - transition to WIND_UP if kick is complete
                    if (RDef_Encoder.count < 0 - (rdef_encoder_360_deg >> 3)) {
                        defense_shoot_state = WIND_UP;
                    } else {
                        SetDir_RDef(REVERSE);
                        SetDuty_RDef(MAX_ROTATIONAL_SPEED);
                    }
                    break;
                }
                break;
            case SPIN:
                // SPIN
                SetDuty_RDef(MAX_ROTATIONAL_SPEED);
                SetDir_RDef(REVERSE);
                defense_shoot_state = WIND_UP;
                break;
            default:
                // Should be unreachable
                Stop_All_Motors();
                main_state = WAIT;
                break;
            }

        } // PLAY loop

        Stop_All_Motors();                          // Stops motor when entering STALL_RECOVERY from PLAY state
        while (main_state == STALL_RECOVERY) {
            int duty, recovery_target;
            switch (stall_state) {
            case RDEF_STALLED:
                recovery_target = (stall_position < (ldef_encoder_range >> 1)) ?
                                   stall_position + (ldef_encoder_range >> 1) : stall_position - (ldef_encoder_range >> 1);
                duty = LinearControl(LDef_Encoder.count - recovery_target);
                if (duty > 0) {
                    SetDir_LDef(LDef_Encoder.count < recovery_target ? FORWARD : REVERSE);
                    SetDuty_LDef(duty);
                } else {
                    main_state = PLAY;
                }
                break;
            case ROFF_STALLED:
                recovery_target = (stall_position < (loff_encoder_range >> 1)) ?
                                       stall_position + (loff_encoder_range >> 1) : stall_position - (loff_encoder_range >> 1);
                duty = LinearControl(LOff_Encoder.count - recovery_target);
                if (duty > 0) {
                    SetDir_LOff(LOff_Encoder.count < recovery_target ? FORWARD : REVERSE);
                    SetDuty_LOff(duty);
                } else {
                    main_state = PLAY;
                }
                break;
            case LDEF_STALLED:
                duty = RotationalControl(RDef_Encoder.count - (rdef_encoder_360_deg >> 2));
                if (duty > 0) {
                    SetDir_RDef(RDef_Encoder.count < (rdef_encoder_360_deg >> 2) ? FORWARD : REVERSE);
                    SetDuty_RDef(duty);
                } else {
                    main_state = PLAY;
                }
                break;
            case LOFF_STALLED:
                duty = RotationalControl(ROff_Encoder.count - (roff_encoder_360_deg >> 2));
                if (duty > 0) {
                    SetDir_ROff(ROff_Encoder.count < (roff_encoder_360_deg >> 2) ? FORWARD : REVERSE);
                    SetDuty_ROff(duty);
                } else {
                    main_state = PLAY;
                }
                break;
            case NONE_STALLED:
            case MULTIPLE_STALLED:
            default:
                Stop_All_Motors();
                main_state = WAIT;
            }
        }
    } // Superloop

    Stop_All_Motors();
} // Main function


/*************************** CONTROL ***************************/

unsigned int LinearControl (int error) {
    unsigned int abs_error = error >= 0 ? error : 0-error;
    if (abs_error < LINEAR_CONTROL_TOLERANCE) {
        return 0;
    } else if (abs_error >= MAX_SPEED_ERROR_THRESHOLD) {
        return MAX_LINEAR_SPEED;
    } else {
        // MIN_LINEAR_SPEED at LINEAR_CONTROL_TOLERANCE error, approx MAX_LINEAR_SPEED at MAX_SPEED_ERROR_THRESHOLD error
        return MIN_LINEAR_SPEED + (abs_error - LINEAR_CONTROL_TOLERANCE) *
                                  (MAX_LINEAR_SPEED - MIN_LINEAR_SPEED) / (MAX_SPEED_ERROR_THRESHOLD - LINEAR_CONTROL_TOLERANCE);
    }
}

unsigned int RotationalControl (int error) {
    unsigned int abs_error = error >= 0 ? error : 0-error;
    if (abs_error < ROTATIONAL_CONTROL_TOLERANCE) {
        return 0;
    } else {
        return MAX_ROTATIONAL_SPEED;
    }
}

/*************************** HELPER ***************************/

void CalibrateEncoders(uint8_t switch_image) {
    // Optical interrupters are actually high at 90deg forward; correct for use elsewhere
    if (switch_image & RLSDEF_BIT)  RDef_Encoder.count = rdef_encoder_360_deg >> 2;
    if (switch_image & RLSOFF_BIT)  ROff_Encoder.count = roff_encoder_360_deg >> 2;

    // Linear limit switches at 0 and max range
    if (switch_image & LLSDEF1_BIT) LDef_Encoder.count = 0;
    if (switch_image & LLSDEF2_BIT) LDef_Encoder.count = ldef_encoder_range;
    if (switch_image & LLSOFF1_BIT) LOff_Encoder.count = 0;
    if (switch_image & LLSOFF2_BIT) LOff_Encoder.count = loff_encoder_range;
}


/**************************** SETUP ****************************/

void SetInitialStates() {
    // Initialize and default to WIND_UP state
    offense_shoot_state = WIND_UP;
    defense_shoot_state = WIND_UP;

    // Start all motors in FIND_MIN calibration state
    ldef_calibrate_state = FIND_MIN;
    loff_calibrate_state = FIND_MIN;
    rdef_calibrate_state = FIND_MIN;
    roff_calibrate_state = FIND_MIN;

    // Assume no motors are stalled at initial state
    stall_state = NONE_STALLED;
}

void ConfigureDebugGPIO() {
    // configure P3.0 as output for debug
    P3->DIR |= BIT0;
    P3->SEL0 &= ~BIT0;
    P3->SEL1 &= ~BIT0;

    // red LED at P1.0 as output for debug
    P1->DIR |= BIT0;
    P1->SEL0 &= ~BIT0;
    P1->SEL1 &= ~BIT0;
    P1->OUT &= ~BIT0;
}



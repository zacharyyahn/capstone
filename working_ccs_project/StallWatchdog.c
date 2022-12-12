/*
 * StallWatchdog.c
 *
 *  Created on: Dec 7, 2022
 *      Author: jalon
 */

#include "PWM.h"
#include "StallWatchdog.h"
#include "UART.h"
#include "Encoder.h"

extern struct encoder LDef_Encoder, LOff_Encoder, RDef_Encoder, ROff_Encoder;
extern enum main_state_enum main_state;

struct motor_counts
{
    int16_t LDef;
    int16_t LOff;
    int16_t RDef;
    int16_t ROff;
};

struct motor_counts Previous_Encoder_Counts, Stall_Counts;
enum stall_state_enum stall_state;
int16_t stall_position;

void StallWatchdog_Init ()
{
    // Halt timer A1 and configure
    TIMER_A1->CTL = TIMER_A_CTL_MC_0;

    // Enable interrupt on CCR0
    TIMER_A1->CCTL[0] = TIMER_A_CCTLN_CCIE;

    // 12MHz / (8 * CCR0) = 50 Hz; CCR0 = 30000 dec
    TIMER_A1->CCR[0] = 0x7530;

    // Enable interrupt 10 for TA1 CCR0; datasheet (p. 118)
    NVIC->ISER[0] |= (0x00000001 << TA1_0_IRQn);

    // Priority set in top three bits of register
    NVIC->IP[TA1_0_IRQn] = (0x04 << 5);

    // bit9-8 = 10 to set SMCLK, bit5-4 = 01 to set up mode, bit7 = 1 to set CLK divider to /8
    TIMER_A1->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1 | TIMER_A_CTL_ID_3;

    // Reset before beginning use
    TIMER_A1->CTL |= TIMER_A_CTL_CLR;

    // Initialize encoder counts
    Previous_Encoder_Counts.LDef = LDef_Encoder.count;
    Previous_Encoder_Counts.LOff = LOff_Encoder.count;
    Previous_Encoder_Counts.RDef = RDef_Encoder.count;
    Previous_Encoder_Counts.ROff = RDef_Encoder.count;

    // Initialize stall counts to 0
    Stall_Counts.LDef = 0;
    Stall_Counts.LOff = 0;
    Stall_Counts.RDef = 0;
    Stall_Counts.ROff = 0;

    // Assume no motors stalled in initial state
    stall_state = NONE_STALLED;
}

// In wdog if a stallcount hits 10 check stallstate
// if stallstate is none set to motor stalled and reset count, mainstate to recovery
// if stallstae not none and not same motor, shutdown and goto wait
// if same motor appears stalled, we failed recovery and should goto wait
// reset to none in individual "not stalled" cases

void TA1_0_IRQHandler ()
{
    if (TIMER_A0->CCR[LDEF_CCR_INDEX])          // LDEF duty cycle is positive
    {
        // Abs of actual and previous encoder counts
        uint16_t LDef_Delta = (LDef_Encoder.count - Previous_Encoder_Counts.LDef) > 0 ?
                              (LDef_Encoder.count - Previous_Encoder_Counts.LDef) :
                              (Previous_Encoder_Counts.LDef - LDef_Encoder.count);
        Previous_Encoder_Counts.LDef = LDef_Encoder.count;

        // Check if not moving when should be moving
        if (LDef_Delta <= STATIONARY_TOLERANCE)
        {
            Stall_Counts.LDef++;
        }
        else
        {
            Stall_Counts.LDef = 0;
            if (LDEF_STALLED == stall_state)
            {
                stall_state = NONE_STALLED;
                P1->OUT &= ~BIT0;
            }
        }

        // Each stall count is 1/50s
        if (Stall_Counts.LDef >= 10)
        {
            // Stop all motors
            Stop_All_Motors();
            if (NONE_STALLED == stall_state)
            {
                stall_state = LDEF_STALLED;
                main_state = STALL_RECOVERY;
                Stall_Counts.LDef = 0;
            }
            else
            {
                main_state = WAIT;
                Stall_Counts.LDef = 0;
                stall_state = MULTIPLE_STALLED;
            }

            // Red LED on for debug
            P1->OUT |= BIT0;
        }
    }

    if (TIMER_A0->CCR[LOFF_CCR_INDEX]) // LOFF duty cycle positive
    {
        // Abs of actual and previous encoder counts
        uint16_t LOff_Delta = (LOff_Encoder.count - Previous_Encoder_Counts.LOff) > 0 ?
                              (LOff_Encoder.count - Previous_Encoder_Counts.LOff) :
                              (Previous_Encoder_Counts.LOff - LOff_Encoder.count);
        Previous_Encoder_Counts.LOff = LOff_Encoder.count;

        // Check if not moving when should be moving
        if (LOff_Delta <= STATIONARY_TOLERANCE)
        {
            Stall_Counts.LOff++;
        }
        else
        {
            Stall_Counts.LOff = 0;
            if (LOFF_STALLED == stall_state)
            {
                stall_state = NONE_STALLED;
                P1->OUT &= ~BIT0;
            }
        }

        // Each stall count is 1/50s
        if (Stall_Counts.LOff >= 10)
        {
            // Stop all motors
            Stop_All_Motors();
            if (NONE_STALLED == stall_state)
            {
                stall_state = LOFF_STALLED;
                main_state = STALL_RECOVERY;
                Stall_Counts.LOff = 0;
            }
            else
            {
                main_state = WAIT;
                Stall_Counts.LOff = 0;
                stall_state = MULTIPLE_STALLED;
            }

            // Red LED on for debug
            P1->OUT |= BIT0;
        }
    }

    if (TIMER_A0->CCR[RDEF_CCR_INDEX])
    {
        // Abs of actual and previous encoder counts
        uint16_t RDef_Delta = (RDef_Encoder.count - Previous_Encoder_Counts.RDef) > 0 ?
                              (RDef_Encoder.count - Previous_Encoder_Counts.RDef) :
                              (Previous_Encoder_Counts.RDef - RDef_Encoder.count);
        Previous_Encoder_Counts.RDef = RDef_Encoder.count;

        // Check if not moving when should be moving
        if (RDef_Delta <= STATIONARY_TOLERANCE)
        {
            Stall_Counts.RDef++;
        }
        else
        {
            Stall_Counts.RDef = 0;
            if (RDEF_STALLED == stall_state)
            {
                stall_state = NONE_STALLED;
                P1->OUT &= ~BIT0;
            }
        }

        // Each stall count is 1/50s
        if (Stall_Counts.RDef >= 10)
        {
            // Stop all motors
            Stop_All_Motors();
            if (NONE_STALLED == stall_state)
            {
                stall_state = RDEF_STALLED;
                stall_position = LDef_Encoder.count;
                main_state = STALL_RECOVERY;
                Stall_Counts.RDef = 0;
            }
            else
            {
                main_state = WAIT;
                Stall_Counts.RDef = 0;
                stall_state = MULTIPLE_STALLED;
            }

            // Red LED on for debug
            P1->OUT |= BIT0;
        }
    }

    if (TIMER_A0->CCR[ROFF_CCR_INDEX])
    {
        // Abs of actual and previous encoder counts
        uint16_t ROff_Delta = (ROff_Encoder.count - Previous_Encoder_Counts.ROff) > 0 ?
                              (ROff_Encoder.count - Previous_Encoder_Counts.ROff) :
                              (Previous_Encoder_Counts.ROff - ROff_Encoder.count);
        Previous_Encoder_Counts.ROff = ROff_Encoder.count;

        // Check if not moving when should be moving
        if (ROff_Delta <= STATIONARY_TOLERANCE)
        {
            Stall_Counts.ROff++;
        }
        else
        {
            Stall_Counts.ROff = 0;
            if (ROFF_STALLED == stall_state)
            {
                stall_state = NONE_STALLED;
                P1->OUT &= ~BIT0;
            }
        }

        // Each stall count is 1/50s
        if (Stall_Counts.ROff >= 10)
        {
            // Stop all motors
            Stop_All_Motors();
            if (NONE_STALLED == stall_state)
            {
                stall_state = ROFF_STALLED;
                stall_position = LOff_Encoder.count;
                main_state = STALL_RECOVERY;
                Stall_Counts.ROff = 0;
            }
            else
            {
                main_state = WAIT;
                Stall_Counts.ROff = 0;
                stall_state = MULTIPLE_STALLED;
            }

            // Red LED on for debug
            P1->OUT |= BIT0;
        }
    }

    TIMER_A1->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

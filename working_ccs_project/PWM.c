#include "PWM.h"

// Indicate which pin should be producing a PWM signal for a given motor. Should only be set using setters
uint8_t RDef_PWM_Pin, ROff_PWM_Pin, LDef_PWM_Pin, LOff_PWM_Pin;

void PWM_Init ()
{
    // P6.0 -> LDef_IN1, P6.1 -> LDef_IN2
    // P6.4 -> LOff_IN1, P6.5 -> LOff_IN2
    // Sets as output
    LINEAR_CONTROL_PORT->DIR |= LDEF_IN1_BIT | LDEF_IN2_BIT | LOFF_IN1_BIT | LOFF_IN2_BIT | LDEF_INH_BIT | LOFF_INH_BIT;

    // P4.0 -> RDef_IN1, P4.1 -> RDef_IN2
    // P4.2 -> ROff_IN1, P4.2 -> ROff_IN2
    // Sets as output
    ROTATIONAL_CONTROL_PORT->DIR |= RDEF_IN1_BIT | RDEF_IN2_BIT | ROFF_IN1_BIT | ROFF_IN2_BIT | R_SLP_BIT;

    // Configure as IO
    LINEAR_CONTROL_PORT->SEL0 &= ~(LDEF_IN1_BIT | LDEF_IN2_BIT | LOFF_IN1_BIT | LOFF_IN2_BIT | LDEF_INH_BIT | LOFF_INH_BIT);
    LINEAR_CONTROL_PORT->SEL1 &= ~(LDEF_IN1_BIT | LDEF_IN2_BIT | LOFF_IN1_BIT | LOFF_IN2_BIT | LDEF_INH_BIT | LOFF_INH_BIT);

    ROTATIONAL_CONTROL_PORT->SEL0 &= ~(RDEF_IN1_BIT | RDEF_IN2_BIT | ROFF_IN1_BIT | ROFF_IN2_BIT | R_SLP_BIT);
    ROTATIONAL_CONTROL_PORT->SEL1 &= ~(RDEF_IN1_BIT | RDEF_IN2_BIT | ROFF_IN1_BIT | ROFF_IN2_BIT | R_SLP_BIT);

    // Set both INH bits high (enabled)
    LINEAR_CONTROL_PORT->OUT |= (LDEF_INH_BIT | LOFF_INH_BIT);

    // Set sleep high (enabled)
    ROTATIONAL_CONTROL_PORT->OUT |= R_SLP_BIT;

    // Halt timer A and configure
    TIMER_A0->CTL = TIMER_A_CTL_MC_0;
    TIMER_A0->CCR[0] = 11999;  // sets CCR0 to period - 1


    // Each motor has its own CCR; configure control registers for each
    //      o LDef -> CCR1
    //      o LOff -> CCR2
    //      o RDef -> CCR3
    //      o ROff -> CCR4
    // bit8 = 0 set compare mode, bit4 = 1 enables interrupt
    TIMER_A0->CCTL[0] = TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[LDEF_CCR_INDEX] = TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[LOFF_CCR_INDEX] = TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[RDEF_CCR_INDEX] = TIMER_A_CCTLN_CCIE;
    TIMER_A0->CCTL[ROFF_CCR_INDEX] = TIMER_A_CCTLN_CCIE;

    // Initialize CCRn to zero
    TIMER_A0->CCR[LDEF_CCR_INDEX] = 0x0000;
    TIMER_A0->CCR[LOFF_CCR_INDEX] = 0x0000;
    TIMER_A0->CCR[RDEF_CCR_INDEX] = 0x0000;
    TIMER_A0->CCR[ROFF_CCR_INDEX] = 0x0000;

    // Enable interrupt 9 and 8 for timer A0 CCR0 and TAIFG; datasheet (p. 118)
    NVIC->ISER[0] |= 0x00000300;
    // Priority set in top three bits of register
    NVIC->IP[TA0_N_IRQn] = (0x04 << 5);
    NVIC->IP[TA0_0_IRQn] = (0x04 << 5);

    // Set initial motor direction to forwards
    SetDir_LDef(FORWARD);
    SetDir_LOff(FORWARD);
    SetDir_RDef(FORWARD);
    SetDir_ROff(FORWARD);

    // bit9-8 = 10 to set SMCLK, bit5-4 = 01 to set up mode
    TIMER_A0->CTL = TIMER_A_CTL_TASSEL_2 | TIMER_A_CTL_MC_1;

    // Reset before beginning use
    TIMER_A0->CTL |= TIMER_A_CTL_CLR;
}

// duty must be 0-11998 inclusive
void SetDuty_LDef (uint16_t duty)
{
    if (duty >= 11999) return;
    TIMER_A0->CCR[LDEF_CCR_INDEX] = duty;
}

void Stop_LDef ()
{
    SetDuty_LDef(0);
    LINEAR_CONTROL_PORT->OUT &= ~(LDEF_IN1_BIT | LDEF_IN2_BIT);
}

void SetDuty_LOff (uint16_t duty)
{
    if (duty >= 11999) return;
    TIMER_A0->CCR[LOFF_CCR_INDEX] = duty;
}

void Stop_LOff ()
{
    SetDuty_LOff(0);
    LINEAR_CONTROL_PORT->OUT &= ~(LOFF_IN1_BIT | LOFF_IN2_BIT);
}

void SetDuty_RDef (uint16_t duty)
{
    if (duty >= 11999) return;
    TIMER_A0->CCR[RDEF_CCR_INDEX] = duty;
}

void Stop_RDef ()
{
    SetDuty_RDef(0);
    ROTATIONAL_CONTROL_PORT->OUT &= ~(RDEF_IN1_BIT | RDEF_IN2_BIT);
}

void SetDuty_ROff (uint16_t duty)
{
    if (duty >= 11999) return;
    TIMER_A0->CCR[ROFF_CCR_INDEX] = duty;
}

void Stop_ROff ()
{
    SetDuty_ROff(0);
    ROTATIONAL_CONTROL_PORT->OUT &= ~(ROFF_IN1_BIT | ROFF_IN2_BIT);
}

void Stop_All_Motors ()
{
    Stop_LOff();
    Stop_ROff();
    Stop_LDef();
    Stop_RDef();
}

// Setter for direction flag for rotational defense motor
void SetDir_RDef (enum direction dir)
{
    switch (dir)
    {
        case FORWARD:
            ROTATIONAL_CONTROL_PORT->OUT &= ~RDEF_IN2_BIT;
            RDef_PWM_Pin = RDEF_IN1_BIT;
        break;
        case REVERSE:
            ROTATIONAL_CONTROL_PORT->OUT &= ~RDEF_IN1_BIT;
            RDef_PWM_Pin = RDEF_IN2_BIT;
        break;
        default:
        break;
    }
}

// Setter for direction flag for rotational offense motor
void SetDir_ROff (enum direction dir)
{
    switch (dir)
    {
        case FORWARD:
            ROTATIONAL_CONTROL_PORT->OUT &= ~ROFF_IN2_BIT;
            ROff_PWM_Pin = ROFF_IN1_BIT;
        break;
        case REVERSE:
            ROTATIONAL_CONTROL_PORT->OUT &= ~ROFF_IN1_BIT;
            ROff_PWM_Pin = ROFF_IN2_BIT;
        break;
        default:
        break;
    }
}

// Setter for direction flag for linear defense motor
void SetDir_LDef (enum direction dir)
{
        switch (dir)
        {
        case FORWARD:
            LINEAR_CONTROL_PORT->OUT &= ~LDEF_IN2_BIT;
            LDef_PWM_Pin = LDEF_IN1_BIT;
        break;
        case REVERSE:
            LINEAR_CONTROL_PORT->OUT &= ~LDEF_IN1_BIT;
            LDef_PWM_Pin = LDEF_IN2_BIT;
        break;
        default:
        break;
    }
}

// Setter for direction flag for linear offense motor
// NOTE: motor flipped mechanically from original orientation
void SetDir_LOff (enum direction dir)
{
    switch (dir)
    {
        case REVERSE:
            LINEAR_CONTROL_PORT->OUT &= ~LOFF_IN2_BIT;
            LOff_PWM_Pin = LOFF_IN1_BIT;
        break;
        case FORWARD:
            LINEAR_CONTROL_PORT->OUT &= ~LOFF_IN1_BIT;
            LOff_PWM_Pin = LOFF_IN2_BIT;
        break;
        default:
        break;
    }
}

void TA0_0_IRQHandler()
{
    // Linear defense reset
    if (0 == TIMER_A0->CCR[LDEF_CCR_INDEX])
    {
        // Special case where 0 duty cycle is desired; hard set both IN1 and IN2 to 0 regardless of dir
        LINEAR_CONTROL_PORT->OUT &= ~(LDEF_IN1_BIT | LDEF_IN2_BIT);
    }
    else
    {
        // Else, set each PWM signal for each motor high depending on direction
        LINEAR_CONTROL_PORT->OUT |= LDef_PWM_Pin;
    }

    // Linear offense reset
    if (0 == TIMER_A0->CCR[LOFF_CCR_INDEX])
    {
        LINEAR_CONTROL_PORT->OUT &= ~(LOFF_IN1_BIT | LOFF_IN2_BIT);
    }
    else
    {
        LINEAR_CONTROL_PORT->OUT |= LOff_PWM_Pin;
    }

    // Rotational defense reset
    if (0 == TIMER_A0->CCR[RDEF_CCR_INDEX])
    {
        ROTATIONAL_CONTROL_PORT->OUT &= ~(RDEF_IN1_BIT | RDEF_IN2_BIT);
    }
    else
    {
        ROTATIONAL_CONTROL_PORT->OUT |= RDef_PWM_Pin;
    }

    // Rotational offense reset
    if (0 == TIMER_A0->CCR[ROFF_CCR_INDEX])
    {
        ROTATIONAL_CONTROL_PORT->OUT &= ~(ROFF_IN1_BIT | ROFF_IN2_BIT);
    }
    else
    {
        ROTATIONAL_CONTROL_PORT->OUT |= ROff_PWM_Pin;
    }

    // Interrupt flag reset
    TIMER_A0->CCTL[0] &= ~TIMER_A_CCTLN_CCIFG;
}

void TA0_N_IRQHandler ()
{
    // Set low signal on active signal pin if interrupt flag for that register is set
    if (TIMER_A0->CCTL[LDEF_CCR_INDEX] & TIMER_A_CCTLN_CCIFG)
    {
        LINEAR_CONTROL_PORT->OUT &= ~LDef_PWM_Pin;
        TIMER_A0->CCTL[LDEF_CCR_INDEX] &= ~TIMER_A_CCTLN_CCIFG;
    }
    if (TIMER_A0->CCTL[LOFF_CCR_INDEX] & TIMER_A_CCTLN_CCIFG)
    {
        LINEAR_CONTROL_PORT->OUT &= ~LOff_PWM_Pin;
        TIMER_A0->CCTL[LOFF_CCR_INDEX] &= ~TIMER_A_CCTLN_CCIFG;
    }
    if (TIMER_A0->CCTL[RDEF_CCR_INDEX] & TIMER_A_CCTLN_CCIFG)
    {
        ROTATIONAL_CONTROL_PORT->OUT &= ~RDef_PWM_Pin;
        TIMER_A0->CCTL[RDEF_CCR_INDEX] &= ~TIMER_A_CCTLN_CCIFG;
    }
    if (TIMER_A0->CCTL[ROFF_CCR_INDEX] & TIMER_A_CCTLN_CCIFG)
    {
        ROTATIONAL_CONTROL_PORT->OUT &= ~ROff_PWM_Pin;
        TIMER_A0->CCTL[ROFF_CCR_INDEX] &= ~TIMER_A_CCTLN_CCIFG;
    }
}

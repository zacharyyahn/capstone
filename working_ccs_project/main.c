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
#include "Planning.h"
#include "CortexM.h"

extern char in_buffer[BUF_LINE][BUF_SIZE+1];
extern int latest_received_line;
extern int line_received;
extern uint8_t switch_image;

int main(void)
{
    // Stop watchdog timer; technical reference (p. 585)
    WDT_A->CTL = 0x0080 | 0x5A00;
    // UART_A0_Init();

    Clock_Init48MHz();
    PWM_Init();
    SwitchReader_Init();
    SetDuty_LDef(10000);

    EnableInterrupts();

    while (1) {
    }
}


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

extern char in_buffer[BUF_LINE][BUF_SIZE];

int main(void)
{
    // Stop watchdog timer; technical reference (p. 585)
    WDT_A->CTL = 0x0080 | 0x5A00;
    UART_A0_Init();

    while (1) {
        // Hi, I'm the main loop. I don't really do anything right now.
        // I'm busy receiving characters from the simulator!


    }
}


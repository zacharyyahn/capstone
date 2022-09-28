/*
 * UART.c
 *
 * Setup and usage functions for UART Rx
 * See main.c for reference to datasheet and technical reference used
 *
 */

#include "UART.h"

// Current indexes of write buffer and fileline
int char_index = 0;
int line_index = 0;

// Contains interrupt handler for char Rx that writes char to buffer
// Depending on usage, consider a more robust (FIFO?) structure for buffering lines
char in_buffer[BUF_LINE][BUF_SIZE];

// Initializes UART mode for eUSCI_A0; follows procedure from technical reference (p. 728)
void UART_A0_Init(void) {
    // Reset with UCSWRST bit per technical reference recommendation (p. 728)
    EUSCI_A0->CTLW0 = 0x0001;

    // Bits 6-7 = 11 to use SMCLK
    // Bit 0 = 1 to hold reset during config
    // Other settings default (0); technical reference (p. 745)
    EUSCI_A0->CTLW0 = 0x00C1;

    // UART BSL pins: P1.2 BSLRxD, P1.3 BSLTxD; datasheet (p. 8)
    // Configure pins in primary mode; SEL0 = 1, SEL1 = 0; datasheet (p. 139)
    P1->SEL0 |= 0x000C;
    P1->SEL1 &= ~0x000C;

    // Clear all but reserved bits from modulation ctrl register
    EUSCI_A0->MCTLW &= ~0xFFF1;

    // Increase deglitch time to 20ns from default 5ns; technical reference (p. 746)
    EUSCI_A0->CTLW1 |= 0x0001;

    // Set baud rate to 9600 (rel to 3MHz SMCLK)
    // Clock settings for SMCLK: DCOCLK at 3MHz default, no divider (technical reference p. 307)
    // N = clk_freq / Baud Rate
    // Value chosen from recommended table (p. 741 tech ref)
    EUSCI_A0->BRW = 312;

    // Enable interface
    EUSCI_A0->CTLW0 &= ~0x0001;

    // Enable interrupt 16; datasheet (p. 118)
    NVIC->ISER[0] |= 0x00010000;
    // Prioity set in top three bits of register
    NVIC->IP[16] = (0x04 << 5);

    // Enable receive Rx interrupt (bit 0); technical reference (p. 752)
    EUSCI_A0->IE &= ~0x000F;
    EUSCI_A0->IE |= 0x0001;


}

// Useful for finding COM port within CCS; not necessary for simulator
void UART_A0_OutChar(char letter) {
    while((EUSCI_A0->IFG & 0x02) == 0);
    EUSCI_A0->TXBUF = letter;
}


// Handler for EUSCIA0 interrupts
// Currently only supports Rx interrupts
void EUSCIA0_IRQHandler(void) {
    // Entered on Rx interrupt
    if (EUSCI_A0->IFG & 0x01) {

        // Rx char stored in RXBUF at time of flag set
        char c = ((char) (EUSCI_A0->RXBUF));
        if (c == '\n') {
            char_index = 0;
            line_index++;
        }

        // Bounds check
        if (char_index == BUF_SIZE) {
            char_index = 0;
        }
        if (line_index == BUF_LINE) {
            line_index = 0;
        }

        in_buffer[line_index][char_index] = c;
        char_index++;

        // Reset flag
        EUSCI_A0->IFG &= ~0x01;
    }
}



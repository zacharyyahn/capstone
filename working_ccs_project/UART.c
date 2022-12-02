/*
 * UART.c
 *
 * Setup and usage functions for UART Rx
 * See main.c for reference to datasheet and technical reference used
 *
 */

#include "UART.h"

// desired player state information from Pi
// scratch values are used while positions are being built across multiple bytes
uint32_t scratch_offense_position, scratch_defense_position;
uint32_t desired_offense_position, desired_defense_position;
enum rotational_state offense_rotstate, defense_rotstate;

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
    // Clock settings for SMCLK 12MHz (technical reference p. 307)
    // N = clk_freq / Baud Rate
    // Value chosen from recommended table (p. 741 tech ref)
    EUSCI_A0->BRW = 1250;

    // Enable interface
    EUSCI_A0->CTLW0 &= ~0x0001;

    // Enable interrupt 16; datasheet (p. 118)
    NVIC->ISER[0] |= 0x00010000;
    // Priority set in top three bits of register
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

        // Most significant 3 bits are the byte index. Byte order is (from 0)
        // 3 bytes of defense position, then 1 byte of defense rotational state,
        // then the same for offense position and rotational state.
        // Positions are sent least-significant 5-bit word first.
        switch (c >> 5) {
        case 0:
            // least significant 5-bit word of defense position
            scratch_defense_position = (uint32_t) (c & UART_DATA_BITMASK);
            break;
        case 1:
            // middle 5-bit word of defense position
            scratch_defense_position += ((uint32_t) (c & UART_DATA_BITMASK)) << 5;
            break;
        case 2:
            // most significant 5-bit word of defense position
            scratch_defense_position += ((uint32_t) (c & UART_DATA_BITMASK)) << 10;
            desired_defense_position = scratch_defense_position;
            break;
        case 3:
            // defense rotational state
            defense_rotstate = c & UART_DATA_BITMASK;
            break;
        case 4:
            // least significant 5-bit word of offense position
            scratch_offense_position = (uint32_t) (c & UART_DATA_BITMASK);
            break;
        case 5:
            // middle 5-bit word of offense position
            scratch_offense_position += ((uint32_t) (c & UART_DATA_BITMASK)) << 5;
            break;
        case 6:
            // most significant 5-bit word of offense position
            scratch_offense_position += ((uint32_t) (c & UART_DATA_BITMASK)) << 10;
            desired_offense_position = scratch_offense_position;
            break;
        case 7:
            // offense rotational state
            offense_rotstate = c & UART_DATA_BITMASK;
            break;
        default:
            // should never happen
            break;
        }

        // Reset flag
        EUSCI_A0->IFG &= ~0x01;
    }
}


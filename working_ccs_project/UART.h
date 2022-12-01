/*
 * UART.h
 *
 */

#include <stdint.h>
#include <stdio.h>
#include "msp.h"

enum rotational_state {
    BLOCK,
    READY,
    SHOOT,
    FANCY_SHOOT,
    SPIN,
};

// last 5 bits of a UART character carry the data, first 3 carry the index
#define UART_DATA_BITMASK   0b00011111

/*
 * Configures eUSCI_A0 in UART mode
 *
 * Baud rate 115,200
 * Rx interrupt enabled only
 */
void UART_A0_Init(void);
void UART_A0_OutChar(char c);


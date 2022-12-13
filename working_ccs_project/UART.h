/*
 * UART.h
 *
 */

#include <stdint.h>
#include <stdio.h>
#include "msp.h"

enum rotational_state
{
    BLOCK,
    READY,
    SHOOT,
    FANCY_SHOOT,
    SPIN,
};

// last 5 bits of a UART character carry the data, first 3 carry the index
#define UART_DATA_BITMASK   0x1F

// special byte codes to control top-level state
#define WAIT_CODE           0xFF
#define CALIBRATE_CODE      0xFE
#define PLAY_CODE           0xFD

enum main_state_enum {
    WAIT,
    CALIBRATE,
    PLAY,
    STALL_RECOVERY,
};

/*
 * Configures eUSCI_A0 in UART mode
 *
 * Baud rate 115,200
 * Rx interrupt enabled only
 */
void UART_ToPi(char* toSend);
void UART_A0_Init(void);
void UART_A0_OutChar(char c);


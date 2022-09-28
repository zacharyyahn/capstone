/*
 * UART.h
 *
 */

#include <stdint.h>
#include <stdio.h>
#include "msp.h"

#define BUF_SIZE 40
#define BUF_LINE 10

/*
 * Configures eUSCI_A0 in UART mode
 *
 * Baud rate 115,200
 * Rx interrupt enabled only
 */
void UART_A0_Init(void);
void UART_A0_OutChar(char c);


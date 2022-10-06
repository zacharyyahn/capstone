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
#include "Planning.h"

extern char in_buffer[BUF_LINE][BUF_SIZE+1];
extern int latest_received_line;
extern int line_received;

BallState ball_state;
PlayerSetState* rods[N_PLAYER_SETS];
BallState ball;
PlayerSetState p1;
PlayerSetState p2;

int main(void)
{
    // Stop watchdog timer; technical reference (p. 585)
    WDT_A->CTL = 0x0080 | 0x5A00;
    UART_A0_Init();

    // Define rods

    p1.y = 10;
    p1.x_min = 11;
    p1.x_max = 21;
    p1.num_players = 3;
    p1.players[0] = -8;
    p1.players[1] = 0;
    p1.players[2] = 8;
    p1.x = 16;
    p1.x_desired = p1.x;

    p2.y = 30;
    p2.x_min = 11;
    p2.x_max = 21;
    p2.num_players = 3;
    p2.players[0] = -8;
    p2.players[1] = 0;
    p2.players[2] = 8;
    p2.x = 16;
    p2.x_desired = p2.x;

    rods[0] = &p1;
    rods[1] = &p2;

    while (1) {
        // UART is listening for input, then when a full line is received,
        // line_received will be == 1, and the rod movement will be planned
        if (line_received) {
            line_received = 0; // allow new lines to be received and trigger this loop again once finished

            sscanf(in_buffer[latest_received_line], "%f %f %f %f", &ball.x, &ball.y, &ball.v_x, &ball.v_y);

            plan_rod_movement(ball);

        }

    }
}


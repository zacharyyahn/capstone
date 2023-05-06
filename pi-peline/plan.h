#ifndef PLAN_H
#define PLAN_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termio.h>
#include <fcntl.h>
#include <unistd.h>
#include "table.h"

// in mm/s
#define MAX_ONCOMING_SHOOT_SPEED        100.0
#define MIN_ONCOMING_PLAN_USE_VEL_SPEED 100.0

// special byte codes for commanding top-level msp state
#define WAIT_CODE       0xFF
#define CALIBRATE_CODE  0xFE
#define PLAY_CODE       0xFD

enum rotational_state {
    BLOCK,
    READY,
    SHOOT,
    FANCY_SHOOT,
    SPIN,
};

struct rod {
    // constants
    float x;                            // fixed x pos
    float travel;                       // in mm
    uint16_t encoder_travel;            // in encoder counts
    int num_players;                    // number of players in the player set
    float player_base[PLAYERS_PER_ROD]; // position of the players at one extreme positioning
                                        // range of player values are player_base[i] to player_base[i] + travel

    // updated variables
    
    // current *desired/output* y pos, measured as distance the 0th (top) player travels from its base position in real
    // space / table coordinates. This can be converted into something sensible for the MSP
    // to understand later
    float y;

    // action state of the rotational motor
    enum rotational_state rot_state;

    // player we chose to use on the previous frame
    int prev_player;
};

struct ball_state {
    float x;
    float y;
    float v_x;
    float v_y;
};

void init_plan(int no_msp_arg);

void start_msp();

void pause_msp();

void return_to_default();

void command_msp();

void plot_player_pos(uint8_t* filtered, int rod_num, float rod_pos);

void plan_rod_movement(struct ball_state *b, int have_ball_pos);

void shutdown_plan();

void print_players(struct rod *r, int rod_num);

#endif


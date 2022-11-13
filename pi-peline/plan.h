#ifndef PLAN_H
#define PLAN_H

#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>
#include "table.h"

// in mm/s
#define MAX_ONCOMING_SHOOT_SPEED        100.0

typedef enum {
    BLOCK,
    READY,
    SHOOT,
    FANCY_SHOOT,
    SPIN,
} rotational_state;

struct rod {
    // constants
    float x; // fixed x pos
    float travel;
    int num_players; // number of players in the player set
    float player_base[PLAYERS_PER_ROD]; // position of the players at one extreme positioning
                                    // range of player values are player_base[i] to player_base[i] + travel

    // updated variables
    
    // current *desired/output* y pos, measured as the 0th (top) player's position in real
    // space / table coordinates. This can be converted into something sensible for the MSP
    // to understand later
    float y;

    // action state of the rotational motor
    rotational_state rot_state;
};

struct ball_state {
    float x;
    float y;
    float v_x;
    float v_y;
};

void init_plan();

void plot_player_pos(__u8* filtered, int rod_num, float rod_pos);

void plan_rod_movement(struct ball_state *b, int have_ball_pos);

void print_players(struct rod *r, int rod_num);

#endif


/*
 * Planning.h
 *
 *  Created on: Oct 3, 2022
 *      Author: colemanjenkins
 */

#ifndef PLANNING_H_
#define PLANNING_H_

#include <stdint.h>
#include <stdio.h>
#include "msp.h"

#define MAX_PLAYERS 3
#define N_PLAYER_SETS 2

#define X_EXTENT 32 // width of the field
#define Y_EXTENT 50 // height of the field

// x measured as center player's x position
typedef struct PlayerSetState {
    // constants
    float y; // fixed y pos
    float x_min; // minimum possible x pos
    float x_max; // maximum possible x pos
    float num_players; // number of players in the player set
    float players[MAX_PLAYERS]; // relative positions of the players relative to the center
                                // eg. [-5.6, 0, 5.6] where there are 3 players and 5.6 apart

    // updated variables
    float x; // current *actual* x pos
    float x_desired; // current *desired/output* x pos
} PlayerSetState;

typedef struct BallState {
    float x;
    float y;
    float v_x;
    float v_y;
} BallState;

void plan_rod_movement(BallState b);

#endif /* PLANNING_H_ */

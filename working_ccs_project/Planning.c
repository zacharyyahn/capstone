/*
 * Planning.c
 *
 *  Created on: Oct 3, 2022
 *      Author: colemanjenkins
 */
#include "Planning.h"
#include <stdlib.h>

#define RLIM_INFINITY 100000000.0

extern BallState ball_state;
extern PlayerSetState* rods[N_PLAYER_SETS];

// estimate of how long it will take the player to move a horizontal distance
float time_to_move(float dist) {
    // example
    return 0.8*dist;
}

// ISR to plan where to move the rods
void plan_rod_movement(BallState b) {
    if (b.v_y == 0) return; // ball isn't moving

    uint8_t i, j;

    // first, check if ball is traveling towards the player set
    int useable[N_PLAYER_SETS];
    for (i = 0; i < N_PLAYER_SETS; i++) {
        useable[i] = 0;
        if (b.v_y > 0) {
            if (rods[i]->y > b.y) useable[i] = 1;
        } else if (b.v_y < 0) {
            if (rods[i]->y < b.y) useable[i] = 1;
        }
    }

    // find best player in each rod to defend
    for (i = 0; i < N_PLAYER_SETS; i++) {
        if (useable[i]) {
            float est_ball_x_pos = b.x - (b.y - rods[i]->y)*b.v_x/b.v_y;
            printf("LOG: est ball position: %f\n", est_ball_x_pos);
            float min_dist = RLIM_INFINITY;
            int min_player = -1;
            float curr_dist;

            for (j = 0; j < rods[i]->num_players; j++) {
                // check if the player can move to where the ball is ending
                if (rods[i]->x_min + rods[i]->players[j] <= est_ball_x_pos &&
                    est_ball_x_pos <= rods[i]->x_max + rods[i]->players[j]) {

                    curr_dist = est_ball_x_pos - (rods[i]->x + rods[i]->players[j]);
                    if (curr_dist < 0) curr_dist *= -1; // get abs

                    if (curr_dist < min_dist) {
                        min_dist = curr_dist;
                        min_player = j;
                    }

                    printf("LOG: Player (%d, %d) within range. Distance of %f\n", i, j, curr_dist);
                }
            }

            if (min_dist < RLIM_INFINITY) {
                // set the desired x value of the rod to move
                rods[i]->x_desired = est_ball_x_pos - rods[i]->players[min_player];
                printf("LOG: Moving rod %d for player %d to %f\n", i, min_player, rods[i]->x_desired);
            }
        }
    }
}

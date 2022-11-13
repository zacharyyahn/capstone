#include "plan.h"

struct rod rods[NUM_RODS];

void init_plan() {
    // common attributes
    int r, p;
    for (r = 0; r < NUM_RODS; r++) {
        rods[r].num_players = PLAYERS_PER_ROD;
        rods[r].travel = TABLE_HEIGHT - (2 * PLAYER_EDGE_MARGIN + (rods[r].num_players - 1) * PLAYER_SPACING);
        for (p = 0; p < rods[r].num_players; p++) {
            rods[r].player_base[p] = PLAYER_EDGE_MARGIN + p * PLAYER_SPACING;
        }
    }

    // unique x positions
    rods[0].x = DEFENSE_X;
    rods[1].x = OFFENSE_X;
}

//TODO: show where the players will move
void plot_player_pos(__u8* filtered, int rod_num, float rod_pos) {
    
}

// Plan where to move the rods 
void plan_rod_movement(struct ball_state *b, int have_ball_pos) {
    // check bounds
    if (b->x > TABLE_LENGTH || b->x < 0 || b->y > TABLE_HEIGHT || b->y < 0) {
        have_ball_pos = 0;
    }
    if (!have_ball_pos) {
        // return all rods to default position if we don't see a ball
        for (int i = 0; i < NUM_RODS; i++) {
            rods[i].y = PLAYER_EDGE_MARGIN + 0.5 * rods[i].travel;
            rods[i].rot_state = BLOCK;
        }
        // TODO: send command to MSP
        return;
    }
    
    // decide general action state of each rod
    int r;
    for (r = 0; r < NUM_RODS; r++) {
        if (b->x < rods[r].x - PLAYER_FORWARD_REACH) {
            // ball is ahead of player, too far to kick
            rods[r].rot_state = BLOCK;
        } else if (b->x > rods[r].x + PLAYER_BACKWARD_REACH) {
            // ball is behind player, too far to kick
            rods[r].rot_state = READY;
        } else if (b->x <= rods[r].x && b->x >= rods[r].x - PLAYER_FORWARD_REACH) {
            // ball is in front of player, within kicking reach
            if (b->v_x > MAX_ONCOMING_SHOOT_SPEED) {
                // if ball approaching too fast, just block
                rods[r].rot_state = BLOCK;
            } else {
                // otherwise we can shoot
                rods[r].rot_state = SHOOT;
            }
        } else if (b->x > rods[r].x && b->x <= rods[r].x + PLAYER_BACKWARD_REACH) {
            // ball is behind player, within kicking reach
            rods[r].rot_state = FANCY_SHOOT;
        } else {
            // should be unreachable
            printf("ERROR: unknown action state for rod %d\n", r);
        }
    }

    // TODO: implement action state of each rod


    // TODO: send command to MSP 
}

void print_players(struct rod *r, int rod_num) {
    printf("----------- Rod %d State -----------\n", rod_num);
    printf("Desired position: (%f, %f)\n", r->x, r->y);
    printf("Travel length: %f\n", r->travel);
    printf("Player base positions: [");
    for (int i = 0; i < r->num_players; i++) {
        printf("%f", r->player_base[i]);
        if (i < r->num_players - 1) {
            printf(", ");
        }
    }
    printf("]\n----------- Rod %d State -----------\n", rod_num);
}


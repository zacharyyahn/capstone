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

void return_to_default() {
    for (int i = 0; i < NUM_RODS; i++) {
        rods[i].y = 0.5 * rods[i].travel;
        rods[i].rot_state = BLOCK;
    } 
}

// find the y position of the intersection between a rod and the ball's
// path, extrapolating from the ball's velocity
float rod_intersect_from_vel (struct ball_state *b, float rod_x) {
    float intersect_y = b->y + (b->v_y / b->v_x) * (rod_x - b->x);
    if (intersect_y < 0) return 0;
    if (intersect_y > TABLE_HEIGHT) return TABLE_HEIGHT;
    return intersect_y;
}

// find the y position of the intersection between a rod and a potential
// shot on our goal, drawing a line from the ball to the center of the goal
float rod_intersect_to_goal (struct ball_state *b, float rod_x) {
    float goal_x = TABLE_LENGTH;
    float goal_y = TABLE_HEIGHT / 2;
    return goal_y - (goal_x - rod_x) * (goal_y - b->y) / (goal_x - b->x);
}

// choose a player on rod r to cover position y, and
// set the rod position to move the player there
void move_player_to_y (struct rod *r, float y) {
    if (r->num_players != 3) {
        // not bothering to generalize to other num_players
        printf("ERROR: move_player_to_y requires exactly 3 players, rod has %d\n", r->num_players);
        return;
    }

    int chosen_player;
    if (y >= r->player_base[1] - PLAYER_FOOT_RADIUS && y <= r->player_base[1] + r->travel + PLAYER_FOOT_RADIUS) {
        chosen_player = 1;
    } else if (y <= r->player_base[0] + r->travel + PLAYER_FOOT_RADIUS) {
        chosen_player = 0;
    } else if (y >= r->player_base[2] - PLAYER_FOOT_RADIUS) {
        chosen_player = 2;
    } else {
        // should be unreachable
        printf("ERROR: could not choose a player for y position %f\n", y);
        return;
    }

    r->y = y - r->player_base[chosen_player];
    
    // might assign an r->y outside bounds because player's feet don't reach all the way to the edge of the table
    if (r->y < 0) r->y = 0;
    if (r->y > r->travel) r->y = r->travel;
}

// TODO: send the desired state to the MSP
void command_msp() {
    for (int i = 0; i < NUM_RODS; i++) {
        printf("rod %d:\tstate: ", i);
        switch (rods[i].rot_state) {
        case BLOCK:
            printf("BLOCK\t");
            break;
        case READY:
            printf("READY\t");
            break;
        case SHOOT:
            printf("SHOOT\t");
            break;
        case FANCY_SHOOT:
            printf("FANCY\t");
            break;
        case SPIN:
            printf("SPIN \t");
            break;
        default:
            break;
        }
        printf("y: %f\n", rods[i].y);
    }
    printf("\n");
}

// Plan where to move the rods 
void plan_rod_movement(struct ball_state *b, int have_ball_pos) {
    // check bounds
    if (b->x > TABLE_LENGTH || b->x < 0 || b->y > TABLE_HEIGHT || b->y < 0) {
        have_ball_pos = 0;
    }
    if (!have_ball_pos) {
        // return all rods to default position if we don't see a ball
        return_to_default();
        command_msp();
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

    // implement action state of each rod
    for (r = 0; r < NUM_RODS; r++) {
        switch (rods[r].rot_state) {
        case BLOCK:
            if (b->v_x >= MIN_ONCOMING_PLAN_USE_VEL_SPEED) {
                // if ball is moving towards us, use velocity extrapolation
                move_player_to_y(&rods[r], rod_intersect_from_vel(b, rods[r].x));
            } else {
                // otherwise, draw line to goal
                move_player_to_y(&rods[r], rod_intersect_to_goal(b, rods[r].x));
            }
            break;
        case READY:
            if (b->v_x <= -1 * MIN_ONCOMING_PLAN_USE_VEL_SPEED) {
                // if ball is moving towards us, use velocity extrapolation
                // ball is behind us, so v_x needs to be negative
                move_player_to_y(&rods[r], rod_intersect_from_vel(b, rods[r].x));
            } else {
                // otherwise, just use ball's current position
                move_player_to_y(&rods[r], b->y);
            }
            break;
        case SHOOT:
        case FANCY_SHOOT:
        case SPIN:
            // aim at ball's current position
            move_player_to_y(&rods[r], b->y);
            break;
        default:
            // should be unreachable
            printf("ERROR: unknown action state %d of rod %d\n", rods[r].rot_state, r);
            break;
        }
    }

    command_msp();
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

//TODO: show where the players will move
void plot_player_pos(__u8* filtered, int rod_num, float rod_pos) {
    
}


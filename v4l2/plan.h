#include <stdio.h>
#include <stdlib.h>
#include <linux/types.h>

#define MAX_PLAYERS 3
#define N_PLAYER_SETS 2

#define RLIM_INFINITY 100000

// x measured as center player's x position
typedef struct PlayerSetState {
    // constants
    float x; // fixed x pos
    float y_min; // minimum possible y pos
    float y_max; // maximum possible y pos
    float range;
    float num_players; // number of players in the player set
    float players[MAX_PLAYERS]; // relative positions of the players relative to the origin (top left)
                                // eg. [0, 2.8, 5.6] where there are 3 players and 2.8 apart

    // updated variables
    float y; // current *actual* y pos
    float y_desired; // current *desired/output* x pos
} PlayerSetState;

typedef struct BallState {
    float x;
    float y;
    float v_x;
    float v_y;
} BallState;

float time_to_move(float dist);

void plot_player_pos(__u8* filtered, int rod_num, float rod_pos);

void plan_rod_movement(BallState b);

void print_players(PlayerSetState p, int rod_num);

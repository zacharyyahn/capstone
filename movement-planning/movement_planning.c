#include <stdio.h>
#include <stdlib.h>

#define MAX_PLAYERS 3
#define N_PLAYER_SETS 2

const float x_extent = 32; // width of the field
const float y_extent = 50; // height of the field

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

BallState ball_state;
PlayerSetState* rods[N_PLAYER_SETS];

// estimate of how long it will take the player to move a horizontal distance
float time_to_move(float dist) {
    // example
    return 0.8*dist;
}

// ISR to plan where to move the rods 
void plan_rod_movement(BallState b) {
    if (b.v_y == 0) return; // ball isn't moving
    
    // first, check if ball is travelling towards the player set 
    int useable[N_PLAYER_SETS];
    for (uint8_t i = 0; i < N_PLAYER_SETS; i++) {
        useable[i] = 0;
        if (b.v_y > 0) {
            if (rods[i]->y > b.y) useable[i] = 1;
        } else if (b.v_y < 0) {
            if (rods[i]->y < b.y) useable[i] = 1;
        } 
    }

    // find best player in each rod to defend
    for (uint8_t i = 0; i < N_PLAYER_SETS; i++) {
        if (useable[i]) {
            float est_ball_x_pos = b.x - (b.y - rods[i]->y)*b.v_x/b.v_y;
            printf("LOG: est ball position: %f\n", est_ball_x_pos);
            float min_dist = RLIM_INFINITY;
            int min_player = -1;
            float curr_dist;

            for (uint8_t j = 0; j < rods[i]->num_players; j++) {
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

void print_players(PlayerSetState p, int rod_num) {
    printf("----------- Rod %d State -----------\n", rod_num);
    printf("Current position: (%f, %f)\n", p.x, p.y);
    printf("Desired position: (%f, %f)\n", p.x_desired, p.y);
    printf("Center player x range: [%f, %f]\n", p.x_min, p.x_max);
    printf("Players: [");
    for (int i = 0; i < p.num_players; i++) {
        printf("%f", p.players[i]);
        if (i < p.num_players - 1) {
            printf(", ");
        }
    }
    printf("]\n----------- Rod %d State -----------\n", rod_num);
}

int main(int argc, char const *argv[]) {
    PlayerSetState p1;
    p1.y = 10;
    p1.x_min = 11;
    p1.x_max = 21;
    p1.num_players = 3;
    p1.players[0] = -8;
    p1.players[1] = 0;
    p1.players[2] = 8;
    p1.x = 16;
    p1.x_desired = p1.x;

    PlayerSetState p2;
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

    BallState ball;

    // ball.x = 1;
    // ball.y = 0;
    // ball.v_x = -1;
    // ball.v_y = 1;

    // // [init path planning interrrupt]
    // char example[10] = "12 9";

    // sscanf(example, "%f %f %f %f", &ball.x, &ball.y, &ball.v_x, &ball.v_y);

    // printf("%f %f %f %f\n", ball.x, ball.y, ball.v_x, ball.v_y);

    // exit(1);

    while (1) {
        // read ball state

        /* this will be in an interrupt */
        print_players(p1, 1);
        print_players(p2, 2);

        printf("\nIncoming ball state (x, y, v_x, v_y): ");
        scanf("%f %f %f %f", &ball.x, &ball.y, &ball.v_x, &ball.v_y);
        printf("\n");
        
        plan_rod_movement(ball);
        /* end interrrupt */

        // motor PID control
        
    }
}
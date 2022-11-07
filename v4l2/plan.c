#include "plan.h"

#define MOTOR_RESPONSE_MS 100
#define MAX_X 350
#define MAX_Y 280

const float x_extent = 32; // width of the field
const float y_extent = 50; // height of the field



//PlayerSetState* rods[N_PLAYER_SETS];

struct PlayerSetState defense_rod = {
	300, 20, 260, 80, 3, {20,100,180}
};

struct PlayerSetState offense_rod = {
	150, 20, 260, 80, 3, {20,100,180}
};

PlayerSetState* rods[N_PLAYER_SETS] = {&offense_rod, &defense_rod};


// estimate of how long it will take the player to move a horizontal distance
float time_to_move(float dist) {
    // example
    return 0.8*dist;
}

//TODO: show where the players will move
void plot_player_pos(__u8* filtered, int rod_num, float rod_pos) {
	
}

// Plan where to move the rods 
void plan_rod_movement(BallState b) {
	//check bounds
	if (b.x > MAX_X || b.x < 0 || b.y > MAX_Y || b.y < 0) {
		return;
	}
	
	if (b.v_x == 0) return; // ball isn't moving, maybe need to zero out
	//printf("LOG: Ball is at x:%f, y:%f with vel x:%f, y%f\n", b.x, b.y, b.v_x, b.v_y);
	BallState future;
	future.x = b.x + b.v_x * MOTOR_RESPONSE_MS / 1000; //convert to mm
	future.y = b.y + b.v_y * MOTOR_RESPONSE_MS / 1000;
	future.v_x = b.v_x;
	future.v_y = b.v_y;
	
    // first, check if ball is travelling towards the player set 
    int useable[N_PLAYER_SETS];
    for (__u8 i = 0; i < N_PLAYER_SETS; i++) {
        useable[i] = 0;
        if (b.v_x > 0) {
            if (rods[i]->x > b.x) useable[i] = 1;
        } else if (b.v_x < 0) {
            if (rods[i]->x < b.y) useable[i] = 1;
        } 
    }

    // find best player in each rod to defend
    for (__u8 i = 0; i < N_PLAYER_SETS; i++) {
        if (useable[i]) {
            //float est_ball_x_pos = b.x - (b.y - rods[i]->y)*b.v_x/b.v_y;
            //printf("LOG: est ball position: %f\n", future.x);
            float min_dist = RLIM_INFINITY;
            int min_player = -1;
            float curr_dist;
            float curr_dist_x;
            float curr_dist_y;

            for (__u8 j = 0; j < rods[i]->num_players; j++) {
                // check if the player can move to where the ball is ending
                if (future.y >= rods[i]->players[j] && future.y <= rods[i]->players[j] + rods[i]->range) {

					//calculate distance to that player, assuming rod is centered
					curr_dist_x = future.x - rods[i]->x;
					curr_dist_y = future.y - (rods[i]->players[j] + rods[i]->range/2);
                    curr_dist = curr_dist_x + curr_dist_y;
                    if (curr_dist < 0) curr_dist *= -1; // get abs

                    if (curr_dist < min_dist) {
                        min_dist = curr_dist;
                        min_player = j;
                    }

                    //printf("LOG: Player (%d, %d) within range. Distance of %f\n", i, j, curr_dist);
                }
            }

            if (min_dist < RLIM_INFINITY) {
                // set the desired x value of the rod to move
                // want to move to the current pos (assume center) + difference between current pos and future pos
                rods[i]->y_desired = future.y;
                //printf("LOG: Moving rod %d for player %d to %f\n", i, min_player, rods[i]->y_desired);
            }
        }
    }
}

void print_players(PlayerSetState p, int rod_num) {
    printf("----------- Rod %d State -----------\n", rod_num);
    printf("Current position: (%f, %f)\n", p.x, p.y);
    printf("Desired position: (%f, %f)\n", p.x, p.y_desired);
    printf("Center player x range: [%f, %f]\n", p.y_min, p.y_max);
    printf("Players: [");
    for (int i = 0; i < p.num_players; i++) {
        printf("%f", p.players[i]);
        if (i < p.num_players - 1) {
            printf(", ");
        }
    }
    printf("]\n----------- Rod %d State -----------\n", rod_num);
}
/*
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

        print_players(p1, 1);
        print_players(p2, 2);

        printf("\nIncoming ball state (x, y, v_x, v_y): ");
        scanf("%f %f %f %f", &ball.x, &ball.y, &ball.v_x, &ball.v_y);
        printf("\n");
        
        plan_rod_movement(ball);

        // motor PID control
        
    }
}
*/

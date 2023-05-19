#include "display.h"
#include "vision.h"
#include "replay.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#define SELECTED_COLOR          0x00ff00
#define REACHABLE_COLOR         0x0000ff
#define UNREACHABLE_COLOR       0xff0000

// globals
extern uint8_t target_y;
extern uint8_t target_u;
extern uint8_t target_v;

extern int loss_contrast_booster;

extern int ball_exists_loss_threshold;
extern int ball_exists_calibrate;

extern struct xy left_corner_center;
extern struct xy right_corner_center;
extern uint8_t left_corner_y;
extern uint8_t left_corner_u;
extern uint8_t left_corner_v;
extern uint8_t right_corner_y;
extern uint8_t right_corner_u;
extern uint8_t right_corner_v;
extern int corner_loss_threshold;
extern int corner_threshold_calibrate;

extern int quit;
extern int do_output;
extern int fun_mode;

extern struct rod rods[NUM_RODS];
// end globals


enum {
    SET_BALL,
    SET_LEFT_CORNER,
    SET_RIGHT_CORNER,
    NONE,
} click_mode = NONE;

enum {
    COLOR,
    LOSS,
    FILTER,
} output_mode = COLOR;

int play_video = 0;
struct timespec next_frame_time;
int show_hud = 0;

SDL_Window *screen;
SDL_Renderer *renderer;
SDL_Texture *color_texture;
uint8_t greyscale_buffer[BUF_SIZE];


int draw_frame (uint8_t *image, uint8_t *losses, uint8_t *filtered) {
    uint8_t *framebuffer, *greyscale_source;

    switch (output_mode) {
    case COLOR:
        framebuffer = image;
        break;
    case LOSS:
        framebuffer = greyscale_buffer;
        greyscale_source = losses;
        break;
    case FILTER:
        framebuffer = greyscale_buffer;
        greyscale_source = filtered;
        break;
    default:
        dprintf(2, "unknown output mode\n");
        return 1;
    }

    if ((output_mode == LOSS) || (output_mode == FILTER)) {
        for (int i = 0; i < WIDTH * HEIGHT; i++) {
            framebuffer[i << 1] = greyscale_source[i];
            framebuffer[(i << 1) + 1] = 128;
        }
    }

    SDL_UpdateTexture(color_texture, NULL, framebuffer, WIDTH * BYTES_PER_PIXEL);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, color_texture, NULL, NULL);
    return 0;
}

void draw_point_of_color (SDL_Renderer *ren, int x, int y, uint32_t color) {
    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color;
    if (SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE)) {
        dprintf(2, "Error setting point color: %s\n", SDL_GetError());
    }
    if (SDL_RenderDrawPoint(ren, x, y)) {
        dprintf(2, "Error drawing point: %s\n", SDL_GetError());
    }
}

void draw_line_of_color (SDL_Renderer *ren, int x1, int y1, int x2, int y2, uint32_t color) {
    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color;
    if (SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE)) {
        dprintf(2, "Error setting line color: %s\n", SDL_GetError());
    }
    if (SDL_RenderDrawLine(ren, x1, y1, x2, y2)) {
        dprintf(2, "Error drawing line: %s\n", SDL_GetError());
    }
}

void draw_lines_of_color (SDL_Renderer *ren, const SDL_Point *points, int count, uint32_t color) {
    uint8_t r = color >> 16;
    uint8_t g = color >> 8;
    uint8_t b = color;
    if (SDL_SetRenderDrawColor(ren, r, g, b, SDL_ALPHA_OPAQUE)) {
        dprintf(2, "Error setting lines color: %s\n", SDL_GetError());
    }
    if (SDL_RenderDrawLines(ren, points, count)) {
        dprintf(2, "Error drawing lines: %s\n", SDL_GetError());
    }
}

void draw_player_with_color (SDL_Renderer *ren, int r, int p, uint32_t color) {
    // draw the rod itself and the extremes of where the players can reach
    struct xyf base_mid_table, base_back_table, base_front_table, extended_mid_table, extended_back_table, extended_front_table;
    SDL_Point  base_mid_image, base_back_image, base_front_image, extended_mid_image, extended_back_image, extended_front_image;

    base_mid_table.x = rods[r].x;
    base_mid_table.y = rods[r].player_base[p] - PLAYER_FOOT_RADIUS;

    base_back_table.x = base_mid_table.x - PLAYER_BACKWARD_REACH;
    base_back_table.y = base_mid_table.y;

    base_front_table.x = base_mid_table.x + PLAYER_FORWARD_REACH;
    base_front_table.y = base_mid_table.y;

    extended_mid_table.x = rods[r].x;
    extended_mid_table.y = rods[r].player_base[p] + rods[r].travel + PLAYER_FOOT_RADIUS;

    extended_back_table.x = extended_mid_table.x - PLAYER_BACKWARD_REACH;
    extended_back_table.y = extended_mid_table.y;

    extended_front_table.x = extended_mid_table.x + PLAYER_FORWARD_REACH;
    extended_front_table.y = extended_mid_table.y;

    table_to_image_position(&base_mid_table,   &base_mid_image);
    table_to_image_position(&base_back_table,  &base_back_image);
    table_to_image_position(&base_front_table, &base_front_image);
    table_to_image_position(&extended_mid_table,   &extended_mid_image);
    table_to_image_position(&extended_back_table,  &extended_back_image);
    table_to_image_position(&extended_front_table, &extended_front_image);

    draw_line_of_color(ren, base_mid_image.x, base_mid_image.y, extended_mid_image.x, extended_mid_image.y, color);
    draw_line_of_color(ren, base_back_image.x, base_back_image.y, base_front_image.x, base_front_image.y, color);
    draw_line_of_color(ren, extended_back_image.x, extended_back_image.y, extended_front_image.x, extended_front_image.y, color);
}

void draw_hud(struct ball_state *b) {
    // draw ball position
    struct xyf ball_pos_table;
    SDL_Point  ball_pos_image;
    ball_pos_table.x = b->x;
    ball_pos_table.y = b->y;

    table_to_image_position(&ball_pos_table, &ball_pos_image);
    draw_point_of_color(renderer, ball_pos_image.x, ball_pos_image.y, 0x000000);

    // draw projected interceptions
    for (int r = 0; r < NUM_RODS; r++) {
        struct xyf intersect_point_table;
        SDL_Point  intersect_point_image;
        intersect_point_table.x = rods[r].x;
        intersect_point_table.y = rods[r].intercept_y;

        table_to_image_position(&intersect_point_table, &intersect_point_image);
        draw_line_of_color(renderer, ball_pos_image.x, ball_pos_image.y, intersect_point_image.x, intersect_point_image.y, 0x000000);
    }

    // draw players. order them so that selected players are drawn on top and unreachable players on bottom
    for (int r = 0; r < NUM_RODS; r++) {
        for (int p = 0; p < rods[r].num_players; p++) {
            if (rods[r].player_selection_state[p] == UNREACHABLE) {
                draw_player_with_color(renderer, r, p, UNREACHABLE_COLOR);
            }
        }
        for (int p = 0; p < rods[r].num_players; p++) {
            if (rods[r].player_selection_state[p] == REACHABLE) {
                draw_player_with_color(renderer, r, p, REACHABLE_COLOR);
            }
        }
        for (int p = 0; p < rods[r].num_players; p++) {
            if (rods[r].player_selection_state[p] == SELECTED) {
                draw_player_with_color(renderer, r, p, SELECTED_COLOR);
            }
        }
    }
}

int init_SDL() {
    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    screen = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_INPUT_GRABBED);
    if (screen == NULL) {
        dprintf(2, "Error creating SDL window: %s\n", SDL_GetError());
        exit(1);
    }
    
    renderer = SDL_CreateRenderer(screen, -1, 0);
    if (renderer == NULL) {
        dprintf(2, "Error creating SDL renderer: %s\n", SDL_GetError());
    }

    color_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_YUY2, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    if (color_texture == NULL) {
        dprintf(2, "Error creating SDL texture: %s\n", SDL_GetError());
    }

    return 0;
}

// somewhat ugly workaround to handle the first event differently in replay mode
void handle_one_event (uint8_t *buf, uint8_t *losses, int replay, SDL_Event *event) {
    switch(event->type) {
    case SDL_MOUSEBUTTONDOWN:
        if (event->button.button == SDL_BUTTON_LEFT) {
            int pixel_i = event->button.y*WIDTH + event->button.x;
            int color_i = (pixel_i & 0x0001) == 0 ? pixel_i : pixel_i-1;

            switch (click_mode) {
            case SET_BALL:
                target_y = buf[pixel_i << 1];
                target_u = buf[(color_i << 1) + 1];
                target_v = buf[(color_i << 1) + 3];
                printf("setting target YUV to %d, %d, %d\n", target_y, target_u, target_v);
                break;
            case SET_LEFT_CORNER:
                left_corner_y = buf[pixel_i << 1];
                left_corner_u = buf[(color_i << 1) + 1];
                left_corner_v = buf[(color_i << 1) + 3];
                left_corner_center.x = event->button.x;
                left_corner_center.y = event->button.y;
                printf("setting left corner YUV to %d, %d, %d and center to (%d, %d)\n",
                        left_corner_y, left_corner_u, left_corner_v, left_corner_center.x, left_corner_center.y);
                break;
            case SET_RIGHT_CORNER:
                right_corner_y = buf[pixel_i << 1];
                right_corner_u = buf[(color_i << 1) + 1];
                right_corner_v = buf[(color_i << 1) + 3];
                right_corner_center.x = event->button.x;
                right_corner_center.y = event->button.y;
                printf("setting right corner YUV to %d, %d, %d and center to (%d, %d)\n",
                        right_corner_y, right_corner_u, right_corner_v, right_corner_center.x, right_corner_center.y);
                break;
            default:
                break;
            }  // switch click_mode
            click_mode = NONE;

        } else if (event->button.button == SDL_BUTTON_RIGHT) {
            int pixel_i = event->button.y*WIDTH + event->button.x;
            int color_i = (pixel_i & 0x0001) == 0 ? pixel_i : pixel_i-1;
            printf("pixel coordinates: (%d, %d)\n", event->button.x, event->button.y);
            printf("pixel YUV values are %d, %d, %d\n", buf[pixel_i << 1], buf[(color_i << 1) + 1], buf[(color_i << 1) + 3]);
            printf("pixel loss value is %d\n", losses[pixel_i]);
        }
        break;

    case SDL_QUIT:
        quit = 1;
        break;

    case SDL_KEYDOWN:
        switch (event->key.keysym.sym) {
        case SDLK_q:
            quit = 1;
            break;
        case SDLK_RETURN:
            start_msp();
            break;
        case SDLK_w:
            pause_msp();
            break;

        case SDLK_LEFT:
            if (replay) {
                prev_replay_frame(buf);
            }
            break;
        case SDLK_RIGHT:
            if (replay) {
                next_replay_frame(buf);
            }
            break;
        case SDLK_HOME:
            if (replay) {
                replay_seek_home(buf);
            }
            break;
        case SDLK_END:
            if (replay) {
                replay_seek_end(buf);
            }
            break;
        case SDLK_SPACE:
            if (!replay) {
                do_output = !do_output;
            } else {
                play_video = !play_video;
                if (play_video) {
                    clock_gettime(CLOCK_MONOTONIC, &next_frame_time);
                    next_frame_time.tv_nsec += 33333333;
                    if (next_frame_time.tv_nsec >= 1000000000) {
                        next_frame_time.tv_nsec -= 1000000000;
                        next_frame_time.tv_sec += 1;
                    }
                }
            }
            break;

        case SDLK_b:
            click_mode = click_mode != SET_BALL ? SET_BALL : NONE;
            break;
        case SDLK_l:
            click_mode = click_mode != SET_LEFT_CORNER ? SET_LEFT_CORNER : NONE;
            break;
        case SDLK_r:
            click_mode = click_mode != SET_RIGHT_CORNER ? SET_RIGHT_CORNER : NONE;
            break;
        case SDLK_f:
            fun_mode = !fun_mode;
            break;

        case SDLK_0:
            loss_contrast_booster = 0;
            break;
        case SDLK_1:
            loss_contrast_booster = 1;
            break;
        case SDLK_2:
            loss_contrast_booster = 2;
            break;
        case SDLK_3:
            loss_contrast_booster = 3;
            break;
        case SDLK_4:
            loss_contrast_booster = 4;
            break;
        case SDLK_5:
            loss_contrast_booster = 5;
            break;

        case SDLK_t:
            corner_threshold_calibrate = !corner_threshold_calibrate;
            ball_exists_calibrate = 0;
            break;
        case SDLK_e:
            ball_exists_calibrate = !ball_exists_calibrate;
            corner_threshold_calibrate = 0;
            loss_contrast_booster = 0;
            break;
        case SDLK_UP:
            if (corner_threshold_calibrate) corner_loss_threshold++;
            if (ball_exists_calibrate) ball_exists_loss_threshold++;
            break;
        case SDLK_DOWN:
            if (corner_threshold_calibrate) corner_loss_threshold--;
            if (ball_exists_calibrate) ball_exists_loss_threshold--;
            break;

        case SDLK_PAGEUP:
            switch (output_mode) {
            case LOSS:
                output_mode = COLOR;
                loss_contrast_booster = 0;
                corner_threshold_calibrate = 0;
                ball_exists_calibrate = 0;
                break;
            case FILTER:
                output_mode = LOSS;
                break;
            default:
                break;
            } // switch output_mode
            break;

        case SDLK_PAGEDOWN:
            switch (output_mode) {
            case COLOR:
                output_mode = LOSS;
                break;
            case LOSS:
                output_mode = FILTER;
                loss_contrast_booster = 0;
                corner_threshold_calibrate = 0;
                // ball_exists_calibrate = 0;
                break;
            default:
                break;
            } // switch output_mode
            break;

        case SDLK_h:
            show_hud = !show_hud;
            break;

        default:
            break;
        }  // switch key
    default:
        break;
    } // switch event type
}

void handle_SDL_events (uint8_t *buf, uint8_t *losses, int replay) {
    SDL_Event event;
    int ok_but_like_an_actual_event_for_real = 0;

    // block for at least one event if in replay mode
    // don't wait if we're playing the video
    while (replay && !play_video && !ok_but_like_an_actual_event_for_real) {
        SDL_WaitEvent(&event);
        handle_one_event(buf, losses, replay, &event);
        if ((event.type == SDL_MOUSEBUTTONDOWN) || (event.type == SDL_QUIT) || (event.type == SDL_KEYDOWN)) {
            ok_but_like_an_actual_event_for_real = 1;
        }
    }
    
    // handle remaining events
    while (SDL_PollEvent(&event)) {
        handle_one_event(buf, losses, replay, &event);
    }

    // to play video, wait 33 ms then advance the frame
    if (play_video) {
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_frame_time, NULL);
        next_frame_time.tv_nsec += 33333333;
        if (next_frame_time.tv_nsec >= 1000000000) {
            next_frame_time.tv_nsec -= 1000000000;
            next_frame_time.tv_sec += 1;
        }
        if (get_replay_index() < get_num_replay_frames()-1) {
            next_replay_frame(buf);
        } else {
            play_video = 0;
        }
    }
}

int output_SDL (uint8_t *image, uint8_t *losses, uint8_t *filtered, struct ball_state *b, int replay) {
    char caption[100];
    
    if (click_mode == SET_BALL) {
        snprintf(caption, sizeof(caption), "click mode: set ball color");
    } else if (click_mode == SET_LEFT_CORNER) {
        snprintf(caption, sizeof(caption), "click mode: set left corner");
    } else if (click_mode == SET_RIGHT_CORNER) {
        snprintf(caption, sizeof(caption), "click mode: set right corner");
    } else if (corner_threshold_calibrate) {
        snprintf(caption, sizeof(caption), "corner loss threshold: %d", corner_loss_threshold);
    } else if (ball_exists_calibrate) {
        snprintf(caption, sizeof(caption), "ball pixel loss threshold: %d", ball_exists_loss_threshold);
    } else if (replay) {
        snprintf(caption, sizeof(caption), "frame %d of %d, seq = %d, t = %d ms",
                                           get_replay_index(), get_num_replay_frames(), get_replay_seq(), get_replay_msec());
    } else {
        snprintf(caption, sizeof(caption), "live feed");
    }

    SDL_SetWindowTitle(screen, caption);

    if (draw_frame(image, losses, filtered)) {
        return 1;
    }

    if (show_hud) {
        draw_hud(b);
    }
    
    SDL_RenderPresent(renderer);

    return 0;
}

void cleanup_SDL () {
    SDL_DestroyTexture(color_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(screen);
}


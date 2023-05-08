/*

    yay - fast and simple yuv viewer

    (c) 2005-2010 by Matthias Wientapper
    (m.wientapper@gmx.de)

    Support of multiple formats added by Cuero Bugot.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#include "yay.h"
#include "plan.h"
#include "vision.h"
#include "replay.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"
#include <SDL/SDL.h>


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

SDL_Surface     *screen;
SDL_Rect        video_rect;
SDL_Overlay     *my_overlay;
const SDL_VideoInfo* info = NULL;

uint8_t *y_data, *cr_data, *cb_data;
uint8_t bpp = 0;
int cfidc = 1;

static const uint8_t SubWidthC[4] =
{
    0, 2, 2, 1
};
static const uint8_t MbWidthC[4] =
{
    0, 8, 8, 16
};
static const uint8_t MbHeightC[4] =
{
    0, 8, 16, 16
};


int load_frame_to_buffers(uint8_t *buf) {
    /* Fill in video data */
    if (cfidc == 0) {
        int i;
        for (i = 0; i < WIDTH*HEIGHT; i++) {
            y_data[i] = buf[i];
        }
        return 0;
    } else if (cfidc == 2) {
        int i;

        for(i = 0; i < WIDTH*HEIGHT*2; i+=4) {
            y_data[i/2]  = buf[i];
            cb_data[i/4] = buf[i+1];
            y_data[i/2+1]= buf[i+2];
            cr_data[i/4] = buf[i+3];
        }
        return 0;
    } else {
        printf("I don't know how to print this -f option D:\n");
        return 1;
    }
}

void convert_chroma_to_420()
{
    int i, j;
    //printf("%dx%d\n",WIDTH, HEIGHT);
    if (cfidc >0) {
        for(j=0; j<HEIGHT/2; j++)
            for(i=0; i<WIDTH/2; i++) {
                my_overlay->pixels[1][j*my_overlay->pitches[1]+i] = cr_data[i*MbWidthC[cfidc]/8+j*(WIDTH/SubWidthC[cfidc])*MbHeightC[cfidc]/8];
                my_overlay->pixels[2][j*my_overlay->pitches[2]+i] = cb_data[i*MbWidthC[cfidc]/8+j*(WIDTH/SubWidthC[cfidc])*MbHeightC[cfidc]/8];
            }
    } else {
        for (i = 0; i < HEIGHT/2; i++) {
            memset(my_overlay->pixels[1]+i*my_overlay->pitches[1], 128, WIDTH/2);
            memset(my_overlay->pixels[2]+i*my_overlay->pitches[2], 128, WIDTH/2);
        }
    }
}

void draw_frame(){
    uint16_t i;

    /* Fill in pixel data - the pitches array contains the length of a line in each plane*/
    SDL_LockYUVOverlay(my_overlay);

    // we cannot be sure, that buffers are contiguous in memory
    if (WIDTH != my_overlay->pitches[0]) {
        for (i = 0; i < HEIGHT; i++) {
            memcpy(my_overlay->pixels[0]+i*my_overlay->pitches[0], y_data+i*WIDTH, WIDTH);
        }
    } else {
        memcpy(my_overlay->pixels[0], y_data, WIDTH*HEIGHT);
    }

    if (cfidc == 1) {
        if (WIDTH != my_overlay->pitches[1]) {
            for (i = 0; i < HEIGHT/2; i++) {
                memcpy(my_overlay->pixels[1]+i*my_overlay->pitches[1], cr_data+i*WIDTH/2, WIDTH/2);
            }
        } else {
            memcpy(my_overlay->pixels[1], cr_data, WIDTH*HEIGHT/4);
        }

        if (WIDTH != my_overlay->pitches[2]) {
            for (i = 0; i < HEIGHT/2; i++) {
                memcpy(my_overlay->pixels[2]+i*my_overlay->pitches[2], cb_data+i*WIDTH/2, WIDTH/2);
            }
        } else {
            memcpy(my_overlay->pixels[2], cb_data, WIDTH*HEIGHT/4);
        }
    }
    convert_chroma_to_420();

    SDL_UnlockYUVOverlay(my_overlay);

    video_rect.x = 0;
    video_rect.y = 0;
    video_rect.w = WIDTH;
    video_rect.h = HEIGHT;

    SDL_DisplayYUVOverlay(my_overlay, &video_rect);
}

int init_SDL() {
    cfidc = output_mode == COLOR ? 2 : 0;

    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    info = SDL_GetVideoInfo();
    if (!info) {
        fprintf(stderr, "SDL ERROR Video query failed: %s\n", SDL_GetError() );
        SDL_Quit(); exit(0);
    }

    uint32_t vflags;
    bpp = info->vfmt->BitsPerPixel;
    if(info->hw_available)
        vflags = SDL_HWSURFACE;
    else
        vflags = SDL_SWSURFACE;

    if ((screen = SDL_SetVideoMode(WIDTH, HEIGHT, bpp, vflags)) == 0) {
        fprintf(stderr, "SDL ERROR Video mode set failed: %s\n", SDL_GetError() );
        SDL_Quit(); exit(0);
    }

    // DEBUG output
    // printf("SDL Video mode set successfully. \nbbp: %d\nHW: %d\nWM: %d\n",
    // 	info->vfmt->BitsPerPixel, info->hw_available, info->wm_available);

    SDL_EnableKeyRepeat(500, 10);

    my_overlay = SDL_CreateYUVOverlay(WIDTH, HEIGHT, SDL_YV12_OVERLAY, screen);
    if (!my_overlay) { //Couldn't create overlay?
        fprintf(stderr, "Couldn't create overlay\n"); //Output to stderr and quit
        exit(1);
    }

    /* should allocate memory for y_data, cr_data, cb_data here */
    y_data  = malloc(WIDTH * HEIGHT * sizeof(uint8_t));
    cb_data = malloc(WIDTH * HEIGHT * sizeof(uint8_t) / 2);
    cr_data = malloc(WIDTH * HEIGHT * sizeof(uint8_t) / 2);

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
    case SDL_VIDEOEXPOSE:
        SDL_DisplayYUVOverlay(my_overlay, &video_rect);
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
                cfidc = 2;
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
                cfidc = 0;
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

int output_SDL (uint8_t *image, uint8_t *losses, uint8_t *filtered, int replay) {
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
        snprintf(caption, sizeof(caption), " ");
    }

    SDL_WM_SetCaption(caption, NULL);

    switch (output_mode) {
    case COLOR:
        if (load_frame_to_buffers(image)) return 1;
        break;
    case LOSS:
        if (load_frame_to_buffers(losses)) return 1;
        break;
    case FILTER:
        if (load_frame_to_buffers(filtered)) return 1;
        break;
    default:
        printf("unknown output mode");
        return 1;
    }
    draw_frame();
    return 0;
}

void cleanup_SDL () {
    SDL_FreeYUVOverlay(my_overlay);
    free(y_data);
    free(cb_data);
    free(cr_data);
}


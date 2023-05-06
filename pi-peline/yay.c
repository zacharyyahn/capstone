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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"
#include <SDL/SDL.h>


// globals
#define WIDTH 640
#define HEIGHT 480
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
} click_mode = SET_BALL;

enum {
    COLOR,
    LOSS,
    FILTER,
} output_mode = COLOR;

SDL_Surface     *screen;
SDL_Event       event;
SDL_Rect        video_rect;
SDL_Overlay     *my_overlay;
const SDL_VideoInfo* info = NULL;

uint32_t width = 0;
uint32_t height = 0;
char *vfilename;
FILE *fpointer;
uint8_t *y_data, *cr_data, *cb_data, *tmp_data;
uint16_t frame = 0;
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

int load_frame(uint8_t *buf){
    /* Fill in video data */
    if (cfidc == 0) {
        int i;
        for (i = 0; i < width*height; i++) {
            y_data[i] = buf[i];
        }
        return 0;
    } else if (cfidc == 2) {
        int i;

        for(i = 0; i < width*height*2; i+=4) {
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
    //printf("%dx%d\n",width, height);
    if (cfidc >0) {
        for(j=0; j<height/2; j++)
            for(i=0; i<width/2; i++) {
                my_overlay->pixels[1][j*my_overlay->pitches[1]+i] = cr_data[i*MbWidthC[cfidc]/8+j*(width/SubWidthC[cfidc])*MbHeightC[cfidc]/8];
                my_overlay->pixels[2][j*my_overlay->pitches[2]+i] = cb_data[i*MbWidthC[cfidc]/8+j*(width/SubWidthC[cfidc])*MbHeightC[cfidc]/8];
            }
    } else {
        for (i = 0; i < height/2; i++) {
            memset(my_overlay->pixels[1]+i*my_overlay->pitches[1], 128, width/2);
            memset(my_overlay->pixels[2]+i*my_overlay->pitches[2], 128, width/2);
        }
    }
}

void draw_frame(){
    uint16_t i;

    /* Fill in pixel data - the pitches array contains the length of a line in each plane*/
    SDL_LockYUVOverlay(my_overlay);

    // we cannot be sure, that buffers are contiguous in memory
    if (width != my_overlay->pitches[0]) {
        for (i = 0; i < height; i++) {
            memcpy(my_overlay->pixels[0]+i*my_overlay->pitches[0], y_data+i*width, width);
        }
    } else {
        memcpy(my_overlay->pixels[0], y_data, width*height);
    }

    if (cfidc == 1) {
        if (width != my_overlay->pitches[1]) {
            for (i = 0; i < height/2; i++) {
                memcpy(my_overlay->pixels[1]+i*my_overlay->pitches[1], cr_data+i*width/2, width/2);
            }
        } else {
            memcpy(my_overlay->pixels[1], cr_data, width*height/4);
        }

        if (width != my_overlay->pitches[2]) {
            for (i = 0; i < height/2; i++) {
                memcpy(my_overlay->pixels[2]+i*my_overlay->pitches[2], cb_data+i*width/2, width/2);
            }
        } else {
            memcpy(my_overlay->pixels[2], cb_data, width*height/4);
        }
    }
    convert_chroma_to_420();

    SDL_UnlockYUVOverlay(my_overlay);

    video_rect.x = 0;
    video_rect.y = 0;
    video_rect.w = width;
    video_rect.h = height;

    SDL_DisplayYUVOverlay(my_overlay, &video_rect);
}

void print_usage(){
    fprintf(stdout, "Usage: yay [-s <widht>x<heigh>] [-f format] [-p] filename.yuv\n\t format can be: 0-Y only, 1-YUV420, 2-YUV422, 3-YUV444\n\t specify '-p' to enable semi-planar mode\n");
}

int init_SDL() {
    width = WIDTH;
    height = HEIGHT;
    cfidc = output_mode == COLOR ? 2 : 0;
    vfilename = "-";

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

    if ((screen = SDL_SetVideoMode(width, height, bpp, vflags)) == 0) {
        fprintf(stderr, "SDL ERROR Video mode set failed: %s\n", SDL_GetError() );
        SDL_Quit(); exit(0);
    }

    // DEBUG output
    // printf("SDL Video mode set successfully. \nbbp: %d\nHW: %d\nWM: %d\n",
    // 	info->vfmt->BitsPerPixel, info->hw_available, info->wm_available);

    SDL_EnableKeyRepeat(500, 10);

    my_overlay = SDL_CreateYUVOverlay(width, height, SDL_YV12_OVERLAY, screen);
    if (!my_overlay) { //Couldn't create overlay?
        fprintf(stderr, "Couldn't create overlay\n"); //Output to stderr and quit
        exit(1);
    }

    /* should allocate memory for y_data, cr_data, cb_data here */
    y_data  = malloc(width * height * sizeof(uint8_t));
    cb_data = malloc(width * height * sizeof(uint8_t) / 2);
    cr_data = malloc(width * height * sizeof(uint8_t) / 2);
    tmp_data = malloc(width * height * sizeof(uint8_t) * 2);

    return 0;
}

void handle_SDL_events (uint8_t *buf, uint8_t *losses) {
    while (SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                int pixel_i = event.button.y*WIDTH + event.button.x;
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
                    left_corner_center.x = event.button.x;
                    left_corner_center.y = event.button.y;
                    printf("setting left corner YUV to %d, %d, %d and center to (%d, %d)\n",
                            left_corner_y, left_corner_u, left_corner_v, left_corner_center.x, left_corner_center.y);
                    break;
                case SET_RIGHT_CORNER:
                    right_corner_y = buf[pixel_i << 1];
                    right_corner_u = buf[(color_i << 1) + 1];
                    right_corner_v = buf[(color_i << 1) + 3];
                    right_corner_center.x = event.button.x;
                    right_corner_center.y = event.button.y;
                    printf("setting right corner YUV to %d, %d, %d and center to (%d, %d)\n",
                            right_corner_y, right_corner_u, right_corner_v, right_corner_center.x, right_corner_center.y);
                    break;
                default:
                    break;
                }  // switch click_mode
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                int pixel_i = event.button.y*WIDTH + event.button.x;
                int color_i = (pixel_i & 0x0001) == 0 ? pixel_i : pixel_i-1;
                printf("pixel coordinates: (%d, %d)\n", event.button.x, event.button.y);
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
            switch (event.key.keysym.sym) {
            case SDLK_q:
                quit = 1;
                break;
            case SDLK_RETURN:
                start_msp();
                break;
            case SDLK_w:
                pause_msp();
                break;
            case SDLK_SPACE:
                do_output = !do_output;
                break;
            case SDLK_b:
                click_mode = SET_BALL;
                break;
            case SDLK_l:
                click_mode = SET_LEFT_CORNER;
                break;
            case SDLK_r:
                click_mode = SET_RIGHT_CORNER;
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

            case SDLK_LEFT:
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

            case SDLK_RIGHT:
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
}

int output_SDL (uint8_t *image, uint8_t *losses, uint8_t *filtered) {
    char caption[64];
    int threshold = corner_threshold_calibrate ? corner_loss_threshold : ball_exists_loss_threshold;
    switch (click_mode) {
    case SET_BALL:
        sprintf(caption, "mode: ball, threshold: %d", threshold);
        SDL_WM_SetCaption(caption, NULL);
        break;
    case SET_LEFT_CORNER:
        sprintf(caption, "mode: left corner, threshold: %d", threshold);
        SDL_WM_SetCaption(caption, NULL);
        break;
    case SET_RIGHT_CORNER:
        sprintf(caption, "mode: right corner, threshold: %d", threshold);
        SDL_WM_SetCaption(caption, NULL);
        break;
    default:
        SDL_WM_SetCaption("mode: unknown", NULL);
        break;
    }

    switch (output_mode) {
    case COLOR:
        if (load_frame(image)) return 1;
        break;
    case LOSS:
        if (load_frame(losses)) return 1;
        break;
    case FILTER:
        if (load_frame(filtered)) return 1;
        break;
    default:
        printf("unknown output mode");
        return 1;
    }
    draw_frame();
    frame++;
    return 0;
}

void cleanup_SDL () {
    SDL_FreeYUVOverlay(my_overlay);
    free(y_data);
    free(cb_data);
    free(cr_data);
    free(tmp_data);
}

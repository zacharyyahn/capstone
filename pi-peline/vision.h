#ifndef VISION_H
#define VISION_H

#include <time.h>
#include <sys/time.h>
#include <SDL2/SDL_rect.h>

#define WIDTH                   640
#define HEIGHT                  480
#define PIXEL_FORMAT            V4L2_PIX_FMT_YUYV
#define BYTES_PER_PIXEL         2
#define BUF_SIZE                WIDTH*HEIGHT*BYTES_PER_PIXEL

struct xy {
    int x;
    int y;
};

struct xyf {
    float x;
    float y;
};

struct frameinfo {
    int32_t seq;
    time_t tv_sec;
    suseconds_t tv_usec;
};

// awful to couple this part of the code to SDL but I'm lazy
void table_to_image_position (struct xyf *table_pos, SDL_Point *image_pos);

#endif


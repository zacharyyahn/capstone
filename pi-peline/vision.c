#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include "videodev2.h"
#include "table.h"
#include "yay.h"
#include "plan.h"
#include "vision.h"

#define WIDTH                   640
#define HEIGHT                  480
#define PIXEL_FORMAT            V4L2_PIX_FMT_YUYV
#define BYTES_PER_PIXEL         2
#define BUF_SIZE                WIDTH*HEIGHT*BYTES_PER_PIXEL

#define CORNER_RADIUS           25

#define MAX_INTERPOLATED_FRAMES 3


// globals
__u8 target_y = 255;
__u8 target_u = 128;
__u8 target_v = 128;

int loss_contrast_booster = 0;

int ball_exists_loss_threshold = 12;
int ball_exists_expected_pixels = 50;
int ball_exists_calibrate = 0;

struct xy left_corner_center = {0, 0};
struct xy right_corner_center = {0, 0};
__u8 left_corner_y = 255;
__u8 left_corner_u = 128;
__u8 left_corner_v = 128;
__u8 right_corner_y = 255;
__u8 right_corner_u = 128;
__u8 right_corner_v = 128;
int corner_loss_threshold = 15;
int corner_threshold_calibrate = 0;

int quit = 0;
int do_output = 1;
// end globals


void loss_function (__u8 *image, __u8 *losses) {
    int i, C_loss;
    for (i = 0; i < BUF_SIZE; i += 4) {
        C_loss = (target_u > image[i+1] ? target_u - image[i+1] : image[i+1] - target_u) +
                 (target_v > image[i+3] ? target_v - image[i+3] : image[i+3] - target_v);
        losses[i >> 1]       = ((target_y > image[i]   ? target_y - image[i]   : image[i]   - target_y) + C_loss) >> 2;
        losses[(i >> 1) + 1] = ((target_y > image[i+2] ? target_y - image[i+2] : image[i+2] - target_y) + C_loss) >> 2;

        if (loss_contrast_booster) {
            if (losses[i >> 1] >= (256 >> loss_contrast_booster)) {
            losses[i >> 1] = 255;
        } else {
            losses[i >> 1] <<= loss_contrast_booster;
        }
            losses[i >> 1] = 255 - losses[i >> 1];

            if (losses[(i >> 1) + 1] >= (256 >> loss_contrast_booster)) {
            losses[(i >> 1) + 1] = 255;
        } else {
            losses[(i >> 1) + 1] <<= loss_contrast_booster;
        }
        losses[(i >> 1) + 1] = 255 - losses[(i >> 1) + 1];
        }
    }
}

void find_corners (__u8 *image, struct xy *bottom_left, struct xy *bottom_right, __u8 *losses) {
    int bottom_left_start_y  = (left_corner_center.y - CORNER_RADIUS) > 0 ?
                               left_corner_center.y - CORNER_RADIUS : 0;
    int bottom_left_end_y    = (left_corner_center.y + CORNER_RADIUS) <= HEIGHT ?
                               left_corner_center.y + CORNER_RADIUS : HEIGHT;
    int bottom_left_start_x  = (left_corner_center.x - CORNER_RADIUS) > 0 ?
                               left_corner_center.x - CORNER_RADIUS : 0;
    int bottom_left_end_x    = (left_corner_center.x + CORNER_RADIUS) <= WIDTH ?
                               left_corner_center.x + CORNER_RADIUS : WIDTH;

    int bottom_right_start_y = (right_corner_center.y - CORNER_RADIUS) > 0 ?
                               right_corner_center.y - CORNER_RADIUS : 0;
    int bottom_right_end_y   = (right_corner_center.y + CORNER_RADIUS) <= HEIGHT ?
                               right_corner_center.y + CORNER_RADIUS : HEIGHT;
    int bottom_right_start_x = (right_corner_center.x - CORNER_RADIUS) > 0 ?
                               right_corner_center.x - CORNER_RADIUS : 0;
    int bottom_right_end_x   = (right_corner_center.x + CORNER_RADIUS) <= WIDTH ?
                               right_corner_center.x + CORNER_RADIUS : WIDTH;

    // account for odd / evenness
    bottom_left_start_y  = (bottom_left_start_y % 2)  == 0 ? bottom_left_start_y  : bottom_left_start_y - 1;
    bottom_left_start_x  = (bottom_left_start_x % 2)  == 0 ? bottom_left_start_x  : bottom_left_start_x - 1;
    bottom_right_start_y = (bottom_right_start_y % 2) == 0 ? bottom_right_start_y : bottom_right_start_y - 1;
    bottom_right_start_x = (bottom_right_start_x % 2) == 0 ? bottom_right_start_x : bottom_right_start_x - 1;
    
    // track min-loss position in the return structs
    bottom_left->x = bottom_left_end_x - 1;
    bottom_left->y = bottom_left_start_y;
    bottom_right->x = bottom_right_start_x;
    bottom_right->y = bottom_right_start_y;


    // bottom-left corner
    int i, j, pix0, pix1, C_loss;
    for (i = bottom_left_start_y; i < bottom_left_end_y; i++) {
        for (j = bottom_left_start_x; j < bottom_left_end_x; j += 2) {
            pix0 = (i*WIDTH + j) << 1;
            pix1 = pix0 + 2;

            C_loss = (left_corner_u > image[pix0+1] ? left_corner_u - image[pix0+1] : image[pix0+1] - left_corner_u) +
                     (left_corner_v > image[pix0+3] ? left_corner_v - image[pix0+3] : image[pix0+3] - left_corner_v);

            // check leftmost pixel first
            if ((left_corner_y > image[pix0]   ? left_corner_y - image[pix0]   : image[pix0]   - left_corner_y) + C_loss < corner_loss_threshold) {
                if (i - j > bottom_left->y - bottom_left->x) {
                    bottom_left->x = j;
                    bottom_left->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j] = 130;
            }
            // only check rightmost pixel if leftmost isn't a match
            else if ((left_corner_y > image[pix1]   ? left_corner_y - image[pix1]   : image[pix1]   - left_corner_y) + C_loss < corner_loss_threshold) {
                if (i - (j + 1) > bottom_left->y - bottom_left->x) {
                    bottom_left->x = j + 1;
                    bottom_left->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j+1] = 130;
            }
            
        }
    }

    //bottom-right corner
    for (i = bottom_right_start_y; i < bottom_right_end_y; i++) {
        for (j = bottom_right_start_x; j < bottom_right_end_x; j += 2) {
            pix0 = (i*WIDTH + j) << 1;
            pix1 = pix0 + 2;
            
            C_loss = (right_corner_u > image[pix0+1] ? right_corner_u - image[pix0+1] : image[pix0+1] - right_corner_u) +
                     (right_corner_v > image[pix0+3] ? right_corner_v - image[pix0+3] : image[pix0+3] - right_corner_v);

            // check rightmost pixel first
            if ((right_corner_y > image[pix1]   ? right_corner_y - image[pix1]   : image[pix1]   - right_corner_y) + C_loss < corner_loss_threshold) {
                if (i + (j + 1) > bottom_right->y + bottom_right->x) {
                    bottom_right->x = j + 1;
                    bottom_right->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j+1] = 130;
            }
            // only check leftmost pixel if rightmost isn't a match
            else if ((right_corner_y > image[pix0]   ? right_corner_y - image[pix0]   : image[pix0]   - right_corner_y) + C_loss < corner_loss_threshold) {
                if (i + j > bottom_right->y + bottom_right->x) {
                    bottom_right->x = j;
                    bottom_right->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j] = 130;
            }
            
        }
    }

    losses[bottom_left->x + WIDTH*bottom_left->y] = 0xFF;
    losses[bottom_right->x + WIDTH*bottom_right->y] = 0xFF;
}

// returns 1 and sets ball position if a ball is found, otherwise returns 0
int find_center (__u8 *losses, __u8 *exists, struct xyf *ball_pos, struct xy *bottom_left, struct xy *bottom_right) {
    struct xy top_left, top_right;
    top_left.x = bottom_left->x + TABLE_HEIGHT / TABLE_LENGTH * (bottom_right->y - bottom_left->y);
    top_left.y = bottom_left->y + TABLE_HEIGHT / TABLE_LENGTH * (bottom_left->x - bottom_right->x);
    top_right.x = bottom_right->x + TABLE_HEIGHT / TABLE_LENGTH * (bottom_right->y - bottom_left->y);
    top_right.y = bottom_right->y + TABLE_HEIGHT / TABLE_LENGTH * (bottom_left->x - bottom_right->x);   

    int min_y = top_left.y < top_right.y ? top_left.y : top_right.y;
    int max_y = bottom_left->y > bottom_right->y ? bottom_left->y : bottom_right->y;
    int min_x = top_left.x < bottom_left->x ? top_left.x : bottom_left->x;
    int max_x = top_right.x > bottom_right->x ? top_right.x : bottom_right->x;

    if (min_y < 0) min_y = 0;
    if (min_x < 0) min_x = 0;
    if (max_y >= HEIGHT) max_y = HEIGHT-1;
    if (max_x >= WIDTH) max_x = WIDTH-1;

    memset(exists, 0x00, WIDTH*HEIGHT);
    int num_pixels = 0;
    int x_sum = 0, y_sum = 0;
    int i, j;
    for (i = min_y; i <= max_y; i++) {
        for (j = min_x; j <= max_x; j++) {
            if (losses[i*WIDTH + j] < ball_exists_loss_threshold) {
                num_pixels++;
                x_sum += j;
                y_sum += i;
                exists[i*WIDTH + j] = 0x80;
            } else {
                exists[i*WIDTH + j] = 0x40;
            }
        }
    }

    if (ball_exists_calibrate) printf("num of ball pixels found: %d\n", num_pixels);

    if (num_pixels >= ball_exists_expected_pixels) {    
        ball_pos->x = ((float) x_sum) / num_pixels;
        ball_pos->y = ((float) y_sum) / num_pixels;
        exists[(int) ball_pos->x + (int) ball_pos->y * WIDTH] = 0xFF;
        return 1;
    }

    return 0;
}

void relative_position (struct xy *bottom_left, struct xy *bottom_right, struct xyf *ball_pos, struct xyf *rel_pos) {
    float d_squared = (bottom_right->y - bottom_left->y) * (bottom_right->y - bottom_left->y) +
                      (bottom_right->x - bottom_left->x) * (bottom_right->x - bottom_left->x);
    rel_pos->x = TABLE_LENGTH / d_squared * ((bottom_right->x - bottom_left->x) * (ball_pos->x - bottom_left->x) +
                                             (bottom_right->y - bottom_left->y) * (ball_pos->y - bottom_left->y));
    rel_pos->y = TABLE_LENGTH / d_squared * ((bottom_left->y - bottom_right->y) * (ball_pos->x - bottom_left->x) +
                                             (bottom_right->x - bottom_left->x) * (ball_pos->y - bottom_left->y))
                 + TABLE_HEIGHT;
}

void read_settings() {
    FILE *settings = fopen("colors.txt", "r");
    if (!settings) {
        if (errno == ENOENT) return;
        perror("error reading settings file");
        exit(1);
    }
    fscanf(settings, "target_y: %hhd\ntarget_u: %hhd\ntarget_v: %hhd\nleft_corner_center: (%d, %d)\n"
                     "right_corner_center: (%d, %d)\nleft_corner_y: %hhd\nleft_corner_u: %hhd\n"
                     "left_corner_v: %hhd\nright_corner_y: %hhd\nright_corner_u: %hhd\nright_corner_v: %hhd\n",
           &target_y, &target_u, &target_v, &left_corner_center.x, &left_corner_center.y, &right_corner_center.x,
           &right_corner_center.y, &left_corner_y, &left_corner_u, &left_corner_v, &right_corner_y,
           &right_corner_u, &right_corner_v);
    if (fclose(settings)) {
        perror("error closing settings file");
        exit(1);
    }
}

void write_settings() {
    int settings_fd = open("colors.txt", O_WRONLY | O_TRUNC | O_CREAT, 00644);
    if (settings_fd < 0) {
        perror("error writing settings to file");
        exit(1);
    }
    dprintf(settings_fd, "target_y: %d\ntarget_u: %d\ntarget_v: %d\nleft_corner_center: (%d, %d)\n"
                          "right_corner_center: (%d, %d)\nleft_corner_y: %d\nleft_corner_u: %d\n"
                          "left_corner_v: %d\nright_corner_y: %d\nright_corner_u: %d\nright_corner_v: %d\n",
            target_y, target_u, target_v, left_corner_center.x, left_corner_center.y, right_corner_center.x,
            right_corner_center.y, left_corner_y, left_corner_u, left_corner_v, right_corner_y,
            right_corner_u, right_corner_v);
    if (close(settings_fd)) {
        perror("error closing settings file");
        exit(1);
    }
}

int main () {
    //init_plan();
    init_SDL();
    read_settings();

    /******************* SET UP IMAGE PROCESSING *******************/
    __u8 losses[WIDTH*HEIGHT];
    memset(&losses, 0, sizeof(losses));
    __u8 exists[WIDTH*HEIGHT];
    memset(&exists, 0, sizeof(exists));
    __u8 filtered[WIDTH*HEIGHT];
    memset(&filtered, 0xFF, sizeof(filtered));


    /******************* OPEN CAMERA *******************/
    int v0 = open("/dev/video0", O_RDWR);
    if (v0 < 0) {
        perror("couldn't open video0");
        return -1;
    }

    int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(v0, VIDIOC_STREAMOFF, &buf_type);


    /******************* SET IMAGE FORMAT *******************/
    struct v4l2_format fmt;
    fmt.type = buf_type;
    if (ioctl(v0, VIDIOC_G_FMT, &fmt)) {
        perror("error querying image format");
        return -1;
    }

    fmt.fmt.pix.width = WIDTH;
    fmt.fmt.pix.height = HEIGHT;
    fmt.fmt.pix.pixelformat = PIXEL_FORMAT;
    fmt.fmt.pix.bytesperline = BYTES_PER_PIXEL*WIDTH;
    fmt.fmt.pix.sizeimage = BUF_SIZE;
    if (ioctl(v0, VIDIOC_S_FMT, &fmt)) {
        perror("error setting image format");
    }

    assert(fmt.fmt.pix.width == WIDTH);
    assert(fmt.fmt.pix.height == HEIGHT);
    assert(fmt.fmt.pix.pixelformat == PIXEL_FORMAT);
    assert(fmt.fmt.pix.bytesperline == BYTES_PER_PIXEL*WIDTH);
    assert(fmt.fmt.pix.sizeimage == BUF_SIZE);


    /******************* SET CAMERA CONTROLS *******************/
    struct v4l2_ext_controls ecs;
    struct v4l2_ext_control  ec[1];
    memset(&ecs, 0, sizeof(ecs));
    memset(&ec, 0, sizeof(ec));
    ecs.which = V4L2_CTRL_WHICH_CUR_VAL;
    ecs.count = sizeof(ec) / sizeof(ec[0]);
    ecs.controls = ec;

    ec[0].id = V4L2_CID_EXPOSURE_AUTO;
    ec[0].value = V4L2_EXPOSURE_APERTURE_PRIORITY;

    if (ioctl(v0, VIDIOC_S_EXT_CTRLS, &ecs)) {
        perror("error setting camera controls");
        return -1;
    }
    ec[0].value = 0;

    if (ioctl(v0, VIDIOC_G_EXT_CTRLS, &ecs)) {
        perror("error querying camera controls");
        return -1;
    }
    assert(ec[0].value == V4L2_EXPOSURE_APERTURE_PRIORITY);


    /******************* SET STREAM PARAMS *******************/
    struct v4l2_streamparm parms;
    memset(&parms, 0, sizeof(parms));
    parms.type = buf_type;
    if (ioctl(v0, VIDIOC_G_PARM, &parms)) {
        perror("error querying stream params");
    }

    parms.parm.capture.timeperframe.numerator = 1;
    parms.parm.capture.timeperframe.denominator = 30;
    if (ioctl(v0, VIDIOC_S_PARM, &parms)) {
        perror("error setting stream params");
    }
    printf("stream capabilites: %#06x\ttimeperframe: %d/%d\n",
            parms.parm.capture.capability, parms.parm.capture.timeperframe.numerator, parms.parm.capture.timeperframe.denominator);


    /******************* REQUEST BUFFERS *******************/
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.type = buf_type;
    req.memory = V4L2_MEMORY_USERPTR;
    req.count = 2;
    if (ioctl(v0, VIDIOC_REQBUFS, &req)) {
        perror("error requesting buffers");
        return -1;
    }
    printf("buffer capabilities: %#010x\n", req.capabilities);
    assert(req.count == 2);


    /******************* BUFFER 0 SETUP *******************/
    __u8 buf0[BUF_SIZE];
    assert(sizeof(buf0) == BUF_SIZE);
    memset(&buf0, 0, sizeof(buf0));

    struct v4l2_buffer bs0;
    memset(&bs0, 0, sizeof(bs0));
    bs0.index = 0;
    bs0.type = buf_type;
    bs0.memory = V4L2_MEMORY_USERPTR;
    bs0.m.userptr = (unsigned long) &buf0;
    bs0.length = BUF_SIZE;


    /******************* BUFFER 1 SETUP *******************/
    __u8 buf1[BUF_SIZE];
    assert(sizeof(buf1) == BUF_SIZE);
    memset(&buf1, 0, sizeof(buf1));

    struct v4l2_buffer bs1;
    memset(&bs1, 0, sizeof(bs1));
    bs1.index = 1;
    bs1.type = buf_type;
    bs1.memory = V4L2_MEMORY_USERPTR;
    bs1.m.userptr = (unsigned long) &buf1;
    bs1.length = BUF_SIZE;


    /******************* START STREAM *******************/
    if (ioctl(v0, VIDIOC_QBUF, &bs0)) {
        perror("error enqueueing buffer 0");
        return -1;
    }
    if (ioctl(v0, VIDIOC_QBUF, &bs1)) {
        perror("error enqueueing buffer 1");
        return -1;
    }
    if (ioctl(v0, VIDIOC_STREAMON, &buf_type)) {
        perror("error starting stream");
        return -1;
    }


    // main loop: dequeue, process, then re-enqueue each buffer until q is pressed
    struct v4l2_buffer *cur_buf;
    struct timeval prev_timestamp = {-1, -1};
    int dt;
    struct xy bottom_left, bottom_right;
    struct xyf ball_pos, rel_pos;
    struct xyf vel = {0, 0};
    struct xyf prev_pos = {0, 0};
    struct ball_state b;
    int have_prev_pos = 0;
    int interpolated_frames = 0;
    struct timespec start_time, end_time;
    for (cur_buf = &bs0; !quit; cur_buf = (cur_buf == &bs0) ? &bs1 : &bs0) {
        if (ioctl(v0, VIDIOC_DQBUF, cur_buf)) {
            perror("error dequeueing buffer");
            return -1;
        }
        dt = (cur_buf->timestamp.tv_sec - prev_timestamp.tv_sec)*1000000 + (cur_buf->timestamp.tv_usec - prev_timestamp.tv_usec);
        prev_timestamp = cur_buf->timestamp;

        loss_function((__u8 *) cur_buf->m.userptr, losses);
        find_corners((__u8 *) cur_buf->m.userptr, &bottom_left, &bottom_right, losses);
        
        if (find_center(losses, exists, &ball_pos, &bottom_left, &bottom_right)) {
            relative_position(&bottom_left, &bottom_right, &ball_pos, &rel_pos);
            if (have_prev_pos) {
                vel.x = (rel_pos.x - prev_pos.x) / dt;  // units are mm/us
                vel.y = (rel_pos.y - prev_pos.y) / dt;
            }
            prev_pos = rel_pos;
            have_prev_pos = 1;
            interpolated_frames = 0;
        } else if (interpolated_frames < MAX_INTERPOLATED_FRAMES) {
            // assume velocity is unchanged
            rel_pos.x = prev_pos.x + vel.x * dt;
            rel_pos.y = prev_pos.y + vel.y * dt;
            prev_pos = rel_pos;
            have_prev_pos = 1;
            interpolated_frames++;
        } else {
            have_prev_pos = 0;
        }

        b.x = rel_pos.x;
        b.y = rel_pos.y;
        b.v_x = vel.x * 1000000;  // convert velocity to mm/s
        b.v_y = vel.y * 1000000;
       // plan_rod_movement(&b, have_prev_pos);

        if (do_output && output_SDL((__u8 *) cur_buf->m.userptr, losses, exists)) return -1;
        handle_SDL_events((__u8 *) cur_buf->m.userptr, losses);

        if (ioctl(v0, VIDIOC_QBUF, cur_buf)) {
            perror("error enqueueing buffer 0");
            return -1;
        }
    }  // end main loop

    //shutdown_plan();

    if (ioctl(v0, VIDIOC_STREAMOFF, &buf_type)) {
        perror("error stopping stream");
    }

    if (close(v0)) {
        perror("error closing device video0");
    }

    cleanup_SDL();
    write_settings();

    return 0;
}

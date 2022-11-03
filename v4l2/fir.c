#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "videodev2.h"
#include "yay.h"

#define WIDTH                   640
#define HEIGHT                  480
#define PIXEL_FORMAT            V4L2_PIX_FMT_YUYV
#define BYTES_PER_PIXEL         2
#define BUF_SIZE                WIDTH*HEIGHT*BYTES_PER_PIXEL
#define EXPOSURE_TIME           20    // in hundreds of microseconds

#define TARGET_Y                target_y
#define TARGET_U                target_u
#define TARGET_V                target_v
#define KERNEL_SIZE             20
#define AVERAGING_BITSHIFT      8

#define CORNER_WIDTH            300
#define CORNER_HEIGHT           200
#define CORNER_LOSS_THRESHOLD   corner_loss_threshold
#define CORNER_Y                corner_y
#define CORNER_U                corner_u
#define CORNER_V                corner_v

#define TABLE_LENGTH            300.0   // mm

// globals
__u8 target_y = 255;
__u8 target_u = 128;
__u8 target_v = 128;

int loss_contrast_booster = 0;

__u8 corner_y = 255;
__u8 corner_u = 128;
__u8 corner_v = 128;
int corner_loss_threshold = 12;
int corner_threshold_calibrate = 0;

int quit = 0;
// end globals

struct xy {
    int x;
    int y;
};

struct xyf {
    float x;
    float y;
};

long find_corners (__u8 *image, struct xy *top_left, struct xy *top_right, __u8 *losses) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

    // track min-loss position in the return structs
    top_left->x = 0;
    top_left->y = 0;
    top_right->x = WIDTH-1;
    top_right->y = 0;

    // top-left corner
    int i, j, pix0, pix1, C_loss;
    for (i = 0; i < CORNER_HEIGHT; i++) {
        for (j = 0; j < CORNER_WIDTH; j += 2) {
            pix0 = (i*WIDTH + j) << 1;
            pix1 = pix0 + 2;

            C_loss = (CORNER_U > image[pix0+1] ? CORNER_U - image[pix0+1] : image[pix0+1] - CORNER_U) +
                     (CORNER_V > image[pix0+3] ? CORNER_V - image[pix0+3] : image[pix0+3] - CORNER_V);

            // check rightmost pixel first
            if (     (CORNER_Y > image[pix1]   ? CORNER_Y - image[pix1]   : image[pix1]   - CORNER_Y) + C_loss < CORNER_LOSS_THRESHOLD) {
                if (i + (j + 1) > top_left->x + top_left->y) {
                    top_left->x = j + 1;
                    top_left->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j+1] = 130;
            }
            // only check leftmost pixel if rightmost isn't a match
            else if ((CORNER_Y > image[pix0]   ? CORNER_Y - image[pix0]   : image[pix0]   - CORNER_Y) + C_loss < CORNER_LOSS_THRESHOLD) {
                if (i + j > top_left->x + top_left->y) {
                    top_left->x = j;
                    top_left->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j] = 130;
            }
        }
    }

    //top-right corner
    for (i = 0; i < CORNER_HEIGHT; i++) {
        for (j = WIDTH - CORNER_WIDTH; j < WIDTH; j += 2) {
            pix0 = (i*WIDTH + j) << 1;
            pix1 = pix0 + 2;

            C_loss = (CORNER_U > image[pix0+1] ? CORNER_U - image[pix0+1] : image[pix0+1] - CORNER_U) +
                     (CORNER_V > image[pix0+3] ? CORNER_V - image[pix0+3] : image[pix0+3] - CORNER_V);

            // check leftmost pixel first
            if (     (CORNER_Y > image[pix0]   ? CORNER_Y - image[pix0]   : image[pix0]   - CORNER_Y) + C_loss < CORNER_LOSS_THRESHOLD) {
                if (i - j > top_right->y - top_right->x) {
                    top_right->x = j;
                    top_right->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j] = 130;
            }
            // only check rightmost pixel if leftmost isn't a match
            else if ((CORNER_Y > image[pix1]   ? CORNER_Y - image[pix1]   : image[pix1]   - CORNER_Y) + C_loss < CORNER_LOSS_THRESHOLD) {
                if (i - (j + 1) > top_right->y - top_right->x) {
                    top_right->x = j + 1;
                    top_right->y = i;
                }
                if (corner_threshold_calibrate) losses[i*WIDTH + j+1] = 130;
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

long loss_function (__u8 *image, __u8 *losses) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    int i, C_loss;
    for (i = 0; i < BUF_SIZE; i += 4) {
        C_loss = (TARGET_U > image[i+1] ? TARGET_U - image[i+1] : image[i+1] - TARGET_U) +
                 (TARGET_V > image[i+3] ? TARGET_V - image[i+3] : image[i+3] - TARGET_V);
        losses[i >> 1]       = ((TARGET_Y > image[i]   ? TARGET_Y - image[i]   : image[i]   - TARGET_Y) + C_loss) >> 2;
        losses[(i >> 1) + 1] = ((TARGET_Y > image[i+2] ? TARGET_Y - image[i+2] : image[i+2] - TARGET_Y) + C_loss) >> 2;

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
    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

//Detect if a ball exists based on how many pixels have a loss greater than
//253 when using inverted loss (255 is the best, 0 is the worst)
int ball_exists(__u8 *losses) {
    int num_pixels = 0;
    int i;
    int threshold = 12; //will decide once we have painted ball
    int expected_pixels = 300; //will decide once we have painted ball
    for (i = 0; i < HEIGHT * WIDTH; i++) {
        if (losses[i] < threshold) num_pixels++;
    }
    //dprintf(2, "num of ball pixels found: %d\n", num_pixels);
    if (num_pixels > expected_pixels) return 1;

    return 0;
}

long filter (__u8 *losses, __u8 *filtered) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

    int li, lj, fi, fj;
    long sum;
    for (li = 0; li < HEIGHT-KERNEL_SIZE; li++) {
        // iterate over full kernel for first pixel of the row
        sum = 0;
        for (fi = li; fi < li+KERNEL_SIZE; fi++) {
            for (fj = 0; fj < KERNEL_SIZE; fj++) {
                sum += losses[fi*WIDTH + fj];
            }
        }
        filtered[(li+(KERNEL_SIZE>>1))*WIDTH + (KERNEL_SIZE>>1)] = (__u8) (sum >> AVERAGING_BITSHIFT);

        // circular buffer for rest of the row
        for (lj = 1; lj < WIDTH-KERNEL_SIZE; lj++) {
            for (fi = li; fi < li+KERNEL_SIZE; fi++) {
                sum += losses[fi*WIDTH + lj+KERNEL_SIZE] - losses[fi*WIDTH + lj];
            }
            filtered[(li+(KERNEL_SIZE>>1))*WIDTH + (lj+(KERNEL_SIZE>>1))] = (__u8) (sum >> AVERAGING_BITSHIFT);
        }
    }

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

long argmin (__u8 *filtered, struct xy *pos) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

    int min_i = 0;
    __u8 min = filtered[0];
    int i;
    for (i = 1; i < WIDTH*HEIGHT; i++) {
        if (filtered[i] < min) {
            min = filtered[i];
            min_i = i;
        }
    }
    pos->x = min_i % WIDTH;
    pos->y = min_i / WIDTH;
    filtered[min_i] = 0xFF;

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

long relative_position (struct xy *top_left, struct xy *top_right, struct xy *ball_pos, struct xyf *rel_pos) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);

    float d_squared = (top_right->y - top_left->y) * (top_right->y - top_left->y) +
                      (top_right->x - top_left->x) * (top_right->x - top_left->x);
    rel_pos->x = TABLE_LENGTH / d_squared * ((top_right->x - top_left->x) * (ball_pos->x - top_left->x) +
                                             (top_right->y - top_left->y) * (ball_pos->y - top_left->y));
    rel_pos->y = TABLE_LENGTH / d_squared * ((top_left->y - top_right->y) * (ball_pos->x - top_left->x) +
                                             (top_right->x - top_left->x) * (ball_pos->y - top_left->y));

    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

int main () {
    init_SDL();

    /******************* SET UP IMAGE PROCESSING *******************/
    __u8 losses[WIDTH*HEIGHT];
    memset(&losses, 0, sizeof(losses));
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
//    ec[1].id = V4L2_CID_EXPOSURE_ABSOLUTE;
//    ec[1].value = EXPOSURE_TIME;

    if (ioctl(v0, VIDIOC_S_EXT_CTRLS, &ecs)) {
        perror("error setting camera controls");
        return -1;
    }
    ec[0].value = 0;
//    ec[1].value = 0;
    if (ioctl(v0, VIDIOC_G_EXT_CTRLS, &ecs)) {
        perror("error querying camera controls");
        return -1;
    }
    assert(ec[0].value == V4L2_EXPOSURE_APERTURE_PRIORITY);
//    assert(ec[1].value == EXPOSURE_TIME);


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
    dprintf(2, "stream capabilites: %#06x\ttimeperframe: %d/%d\n",
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
    dprintf(2, "buffer capabilities: %#010x\n", req.capabilities);
    dprintf(2, "driver allocated %d buffers\n", req.count);


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
    int last_ms = 0;
    struct xy prev_vel, this_vel, prev_pos;
    struct xy ball_pos, top_left, top_right;
    struct xyf rel_pos;
    struct timespec start_time, end_time;
    for (cur_buf = &bs0; !quit; cur_buf = (cur_buf == &bs0) ? &bs1 : &bs0) {
        if (ioctl(v0, VIDIOC_DQBUF, cur_buf)) {
            perror("error dequeueing buffer");
            return -1;
        }
        //dprintf(2, "dt from camera: %ld\n", cur_buf->timestamp.tv_usec/1000-last_ms);
        last_ms = cur_buf->timestamp.tv_usec/1000;

        clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
        loss_function((__u8 *) cur_buf->m.userptr, losses); // dprintf(2, "loss function time (ms): %d\t", loss_function(cur_buf->m.userptr, losses) / 1000000);

        if (1) {//(ball_exists(losses)) {
            filter(losses, filtered); //dprintf(2, "filter time (ms): %d\t", filter(losses, filtered) / 1000000);
            argmin(filtered, &ball_pos); //dprintf(2, "argmin time (ms): %d\t", argmin(filtered, &ball_pos) / 1000000);
            find_corners((__u8 *) cur_buf->m.userptr, &top_left, &top_right, losses); //dprintf(2, "corner time (ms): %ld\n", find_corners(cur_buf->m.userptr, &top_left, &top_right) / 1000000);
            relative_position(&top_left, &top_right, &ball_pos, &rel_pos);
            dprintf(2, "found relative ball position (%f, %f)\n", rel_pos.x, rel_pos.y);
            losses[top_left.x + WIDTH*top_left.y] = 0xFF;
            losses[top_right.x + WIDTH*top_right.y] = 0xFF;
            if (prev_pos.x != 0 && prev_pos.y != 0) {
                this_vel.x = ball_pos.x - prev_pos.x;
                this_vel.y = ball_pos.y - prev_pos.y;
            }
            prev_vel.x = this_vel.x;
            prev_vel.y = this_vel.y;
            prev_pos.x = ball_pos.x;
            prev_pos.y = ball_pos.y;
	    } else {
	        ball_pos.x = prev_pos.x + prev_vel.x;
            ball_pos.y = prev_pos.y + prev_vel.y;
            filtered[WIDTH * ball_pos.y + ball_pos.x] = 0xFF;
            this_vel = prev_vel;
	        prev_pos = ball_pos;
	    }
	    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
        //dprintf(2, "ball pos: (%d, %d)\ttotal calculation time (ms): %ld\n", ball_pos.x, ball_pos.y, (end_time.tv_sec - start_time.tv_sec)*1000 + (end_time.tv_nsec - start_time.tv_nsec)/1000000);
        //dprintf(2, "top left corner at (%d, %d)\ttop right corner at (%d, %d)\n", top_left.x, top_left.y, top_right.x, top_right.y);

	    if (output_SDL((__u8 *) cur_buf->m.userptr, losses, filtered)) return -1;
            handle_SDL_events((__u8 *) cur_buf->m.userptr, losses);

	    if (ioctl(v0, VIDIOC_QBUF, cur_buf)) {
            perror("error enqueueing buffer 0");
            return -1;
        }
    }  // end main loop

    if (ioctl(v0, VIDIOC_STREAMOFF, &buf_type)) {
        perror("error stopping stream");
    }

    if (close(v0)) {
        perror("error closing device video0");
    }

    cleanup_SDL();

    return 0;
}

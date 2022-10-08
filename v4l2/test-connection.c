#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "videodev2.h"

#define WIDTH           640
#define HEIGHT          480
#define PIXEL_FORMAT    V4L2_PIX_FMT_YUYV
#define BYTES_PER_PIXEL 2
#define BUF_SIZE        WIDTH*HEIGHT*BYTES_PER_PIXEL
#define EXPOSURE_TIME	20	// in hundreds of microseconds

#define TARGET_Y	255
#define TARGET_U	128
#define TARGET_V	128

long loss_function (__u8 *image, __u8 *losses) {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &start_time);
    int i, C_loss;
    for (i = 0; i < BUF_SIZE; i += 4) {
        C_loss = (TARGET_U > image[i+1] ? TARGET_U - image[i+1] : image[i+1] - TARGET_U) +
		(TARGET_V > image[i+3] ? TARGET_V - image[i+3] : image[i+3] - TARGET_U);
        losses[i >> 1] = ((TARGET_Y > image[i] ? TARGET_Y - image[i] : image[i] - TARGET_Y) + C_loss) >> 2;
        losses[(i >> 1) + 1] = ((TARGET_Y > image[i+2] ? TARGET_Y - image[i+2] : image[i+2] - TARGET_Y) + C_loss) >> 2;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, &end_time);
    return (end_time.tv_sec - start_time.tv_sec)*1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
}

int main (void) {
    /******************* SET UP IMAGE PROCESSING *******************/
    __u8 losses[WIDTH*HEIGHT];
    memset(&losses, 0, sizeof(losses));


    /******************* OPEN CAMERA *******************/
    int v0 = open("/dev/video0", O_RDWR);
    if (v0 < 0) {
        perror("couldn't open video0");
        return -1;
    }
    int fd = 1; // open("test.yuv", O_WRONLY | O_CREAT | O_TRUNC, 00644);
    if (fd < 0) {
        perror("why");
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
    //ec[1].id = V4L2_CID_EXPOSURE_ABSOLUTE;
    //ec[1].value = EXPOSURE_TIME;

    if (ioctl(v0, VIDIOC_S_EXT_CTRLS, &ecs)) {
        perror("error setting camera controls");
        return -1;
    }
    ec[0].value = 0;
    //ec[1].value = 0;
    if (ioctl(v0, VIDIOC_G_EXT_CTRLS, &ecs)) {
        perror("error querying camera controls");
        return -1;
    }
    assert(ec[0].value = V4L2_EXPOSURE_MANUAL);
    //assert(ec[1].value = EXPOSURE_TIME);


    /******************* SET STREAM PARAMS *******************/
    struct v4l2_streamparm parms;
    memset(&parms, 0, sizeof(parms));
    parms.type = buf_type;
    if (ioctl(v0, VIDIOC_G_PARM, &parms)) {
        perror("error querying stream params");
    }
    dprintf(2, "stream capabilites: %#06x\ttimeperframe: %d/%d\n", 
            parms.parm.capture.capability, parms.parm.capture.timeperframe.numerator, parms.parm.capture.timeperframe.denominator);
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
    
    // dequeue and enqueue each buffer several times
    // this is where the math will happen in the main loop
    int i;
    int last_ms = 0;
    for (i = 0; 1; i++) {
        if (ioctl(v0, VIDIOC_DQBUF, &bs0)) {
            perror("error dequeueing buffer 0");
            return -1;
        }
        //dprintf(2, "dt from camera: %d\n", bs0.timestamp.tv_usec/1000-last_ms);
	//last_ms = bs0.timestamp.tv_usec/1000;
	dprintf(2, "loss function time (ms): %d\n", loss_function((__u8*) &buf0, (__u8*) &losses) / 1000000);
	if (write(fd, losses, sizeof(losses)) == -1) {
            perror("aosnet");
        }
        
	if (ioctl(v0, VIDIOC_QBUF, &bs0)) {
            perror("error enqueueing buffer 0");
            return -1;
        }

        if (ioctl(v0, VIDIOC_DQBUF, &bs1)) {
            perror("error dequeueing buffer 1");
            return -1;
        }
        //dprintf(2, "dt from camera: %d\n", bs1.timestamp.tv_usec/1000-last_ms);
        //last_ms = bs1.timestamp.tv_usec/1000;
	dprintf(2, "loss function time (ms): %d\n", loss_function((__u8*) &buf1, (__u8*) &losses) / 1000000);
        if (write(fd, losses, sizeof(losses)) == -1) {
            perror("aosnet");
        }
        if (ioctl(v0, VIDIOC_QBUF, &bs1)) {
            perror("error enqueueing buffer 1");
            return -1;
        }
    }
   

    if (close(v0)) {
        perror("error on closing device");
    }
    close(fd);

    return 0;
}

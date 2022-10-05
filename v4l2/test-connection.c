#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "videodev2.h"

#define WIDTH           640
#define HEIGHT          480
#define PIXEL_FORMAT    V4L2_PIX_FMT_YUYV
#define BYTES_PER_PIXEL 2
#define BUF_SIZE        WIDTH*HEIGHT*BYTES_PER_PIXEL

int main (void) {
    int v0 = open("/dev/video0", O_RDWR);
    if (v0 < 0) {
        perror("couldn't open video0");
        return -1;
    }

    int buf_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v0, VIDIOC_STREAMOFF, &buf_type)) {
        perror("error during streamoff");
    }

    /******************* VERIFY FORMAT *******************/
    struct v4l2_format fmt;
    fmt.type = buf_type;
    if (ioctl(v0, VIDIOC_G_FMT, &fmt)) {
        perror("error querying current format");
        return -1;
    }
    assert(fmt.fmt.pix.width == WIDTH);
    assert(fmt.fmt.pix.height == HEIGHT);
    assert(fmt.fmt.pix.pixelformat == PIXEL_FORMAT);
    assert(fmt.fmt.pix.bytesperline == BYTES_PER_PIXEL*WIDTH);
    assert(fmt.fmt.pix.sizeimage == BUF_SIZE);

    /******************* REQUEST BUFFERS *******************/
    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.type = buf_type;
    req.memory = V4L2_MEMORY_USERPTR;
    req.count = 2;
    if (ioctl(v0, VIDIOC_REQBUFS, &req)) {
        perror("error requesting buffers");
    }
    printf("buffer capabilities: %#010x\n", req.capabilities);
    printf("driver allocated %d buffers\n", req.count);


    /******************* BUFFER 0 SETUP *******************/
    __u8 buf0[BUF_SIZE];
    assert(sizeof(buf0) == BUF_SIZE);
    memset(&buf0, 0, sizeof(buf0));
    printf("buf0 address: %#010lx\n", (unsigned long) &buf0);

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
    printf("buf1 address: %#010lx\n", (unsigned long) &buf1);

    struct v4l2_buffer bs1;
    memset(&bs1, 0, sizeof(bs1));
    bs1.index = 1;
    bs1.type = buf_type;
    bs1.memory = V4L2_MEMORY_USERPTR;
    bs1.m.userptr = (unsigned long) &buf1;
    bs1.length = BUF_SIZE;

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
    for (i = 0; i < 5; i++) {
        if (ioctl(v0, VIDIOC_DQBUF, &bs0)) {
            perror("error dequeueing buffer 0");
            return -1;
        }
        printf("bs0 userptr address: %#010lx\tbuffer address: %#010lx\tfirst pixel: %#06x",
                bs0.m.userptr, (unsigned long) &buf0, buf0[0]);
        if (ioctl(v0, VIDIOC_QBUF, &bs0)) {
            perror("error enqueueing buffer 0");
            return -1;
        }

        if (ioctl(v0, VIDIOC_DQBUF, &bs1)) {
            perror("error dequeueing buffer 1");
            return -1;
        }
        printf("bs1 user ptr address: %#010lx\tbuffer address: %#010lx\tfirst pixel: %#06x",
                bs1.m.userptr, (unsigned long) &buf1, buf1[0]);
        if (ioctl(v0, VIDIOC_QBUF, &bs1)) {
            perror("error enqueueing buffer 1");
            return -1;
        }
    }

    if (close(v0)) {
        perror("error on closing device");
    }

    return 0;
}

#include "replay.h"
#include "vision.h"
#include <stdio.h>

FILE *replay_fptr = NULL;
int replay_index = 0;
int num_frames = 0;
struct frameinfo replay_frameinfo;
struct frameinfo prev_frameinfo = {-1, -1, -1};
struct frameinfo first_frameinfo;

int init_replay(FILE *fptr, uint8_t *buf) {
    replay_fptr = fptr;
    if (fread(&num_frames, sizeof(num_frames), 1, replay_fptr) != 1) {
        dprintf(2, "Error reading number of frames from replay file\n");
        return 1;
    }

    if (load_frame_from_replay(buf)) {
        return 1;
    }
    first_frameinfo = replay_frameinfo;

    return 0;
}

int load_frame_from_replay(uint8_t *buf) {
    if (fseek(replay_fptr, sizeof(num_frames) + replay_index * (sizeof(struct frameinfo) + BUF_SIZE), SEEK_SET)) {
        perror("error seeking to frame");
        return 1;
    }
    if (fread(&replay_frameinfo, sizeof(replay_frameinfo), 1, replay_fptr) != 1) {
        dprintf(2, "error reading frame info\n");
        return 1;
    }
    if (fread(buf, 1, BUF_SIZE, replay_fptr) != BUF_SIZE) {
        dprintf(2, "error reading frame\n");
        return 1;
    }
    return 0;
}

int get_num_replay_frames() {
    return num_frames;
}

int get_replay_index() {
    return replay_index;
}

int get_replay_seq() {
    return replay_frameinfo.seq;
}

void get_replay_timestamps(struct timeval *cur, struct timeval *prev) {
    cur->tv_sec = replay_frameinfo.tv_sec;
    cur->tv_usec = replay_frameinfo.tv_usec;
    prev->tv_sec = prev_frameinfo.tv_sec;
    prev->tv_usec = prev_frameinfo.tv_usec;
}

int get_replay_msec() {
    return (float) (replay_frameinfo.tv_sec - first_frameinfo.tv_sec) * 1000 +
           (float) (replay_frameinfo.tv_usec - first_frameinfo.tv_usec) / 1000;
}

int prev_replay_frame(uint8_t *buf) {
    if (replay_index > 0) {
        replay_index--;
        if (replay_index == 0) {
            prev_frameinfo.seq = -1;
            prev_frameinfo.tv_sec = -1;
            prev_frameinfo.tv_usec = -1;
        } else {
            if (fseek(replay_fptr, sizeof(num_frames) + (replay_index-1) * (sizeof(struct frameinfo) + BUF_SIZE), SEEK_SET)) {
                perror("error seeking to previous frame");
                return 1;
            }
            if (fread(&prev_frameinfo, sizeof(prev_frameinfo), 1, replay_fptr) != 1) {
                dprintf(2, "error reading previous frame info\n");
                return 1;
            }
        }
        return load_frame_from_replay(buf);
    }
    return 0;
}

int next_replay_frame(uint8_t *buf) {
    if (replay_index < num_frames-1) {
        replay_index++;
        prev_frameinfo = replay_frameinfo;
        return load_frame_from_replay(buf);
    }
    return 0;
}

int replay_seek_home(uint8_t *buf) {
    replay_index = 0;
    prev_frameinfo.seq = -1;
    prev_frameinfo.tv_sec = -1;
    prev_frameinfo.tv_usec = -1;
    return load_frame_from_replay(buf);
}

int replay_seek_end(uint8_t *buf) {
    replay_index = num_frames - 1;
    if (fseek(replay_fptr, sizeof(num_frames) + (replay_index-1) * (sizeof(struct frameinfo) + BUF_SIZE), SEEK_SET)) {
        perror("error seeking to previous frame");
        return 1;
    }
    if (fread(&prev_frameinfo, sizeof(prev_frameinfo), 1, replay_fptr) != 1) {
        dprintf(2, "error reading previous frame info\n");
        return 1;
    }
    return load_frame_from_replay(buf);
}


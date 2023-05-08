#ifndef REPLAY_H
#define REPLAY_H

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>

int init_replay(FILE *fptr, uint8_t *buf);
int load_frame_from_replay(uint8_t *buf);
int get_num_replay_frames();
int get_replay_index();
int get_replay_seq();
void get_replay_timestamps(struct timeval *cur, struct timeval *prev);
int get_replay_msec();
int next_replay_frame(uint8_t *buf);
int prev_replay_frame(uint8_t *buf);
int replay_seek_home(uint8_t *buf);
int replay_seek_end(uint8_t *buf);

#endif


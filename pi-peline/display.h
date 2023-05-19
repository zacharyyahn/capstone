#ifndef YAY_H
#define YAY_H

#include <stdint.h>
#include <stdio.h>
#include "plan.h"

int init_SDL ();
int output_SDL (uint8_t *image, uint8_t *losses, uint8_t *filtered, struct ball_state *b, int replay);
void handle_SDL_events (uint8_t *buf, uint8_t *losses, int replay);
void cleanup_SDL ();

#endif


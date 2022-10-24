#include <linux/types.h>

int init_SDL (int argc, char *argv[]);
int output_SDL (__u8 *buf);
void handle_SDL_events (__u8 *buf, __u8 *losses);
void cleanup_SDL ();


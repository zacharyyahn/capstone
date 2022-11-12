#include <linux/types.h>

int init_SDL ();
int output_SDL (__u8 *image, __u8 *losses, __u8 *filtered);
void handle_SDL_events (__u8 *buf, __u8 *losses);
void cleanup_SDL ();


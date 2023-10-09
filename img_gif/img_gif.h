#include "../headers.h"
#include "../windows.h"

#ifndef IMG_GIF
#define IMG_GIF
#include <gif_lib.h>

void st_Init_GIF(struct_window *);
void *st_Win_Play_GIF_Video(void *_this_win_handle);
#endif
#include "../headers.h"
#ifdef WITH_MP3LIB
#include "../windows.h"

#ifndef SND_MP3
#define SND_MP3

void st_Init_MP3(struct_window *);
void *st_Win_Play_MP3(void *_this_win_handle);

#endif
#endif
#ifdef WITH_WAVLIB
#include "../headers.h"
#include "../windows.h"

#ifndef SND_WAV
#define SND_WAV

void st_Init_WAV(struct_window *);
void *st_Win_Play_WAV(void *_this_win_handle);

#endif
#endif
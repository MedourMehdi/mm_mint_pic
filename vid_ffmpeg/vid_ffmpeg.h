#include "../headers.h"
#include "../windows.h"

#ifndef VID_FFMPEG
#define VID_FFMPEG
#ifdef WITH_FFMPEG

bool st_Check_FF_Ext(const char* this_ext);
void st_Init_FF_Media(struct_window *);
void *st_Win_Play_FF_Media(void *_this_win_handle);
#endif /*WITH_FFMPEG*/
#endif
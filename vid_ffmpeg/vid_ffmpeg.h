#include "../headers.h"
#include "../windows.h"

#ifndef VID_FFMPEG
#define VID_FFMPEG
#ifdef WITH_FFMPEG

bool st_check_ffmpeg_ext(const char* this_ext);
void st_Init_ffmpeg(struct_window *);
void *st_Win_Play_ffmpeg_Video(void *_this_win_handle);
#endif /*WITH_FFMPEG*/
#endif
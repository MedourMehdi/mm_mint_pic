#include "../headers.h"
#include "../windows.h"

#ifndef VID_WEBP
#define VID_WEBP

void st_Write_WEBP(u_int8_t* src_buffer, int width, int height, const char* filename);

void st_Init_Vid_WEBP(struct_window *);
void *st_Win_Play_WEBP_Video(void *_this_win_handle);

bool st_Detect_Webp_Animated(int16_t this_win_handle);
#endif
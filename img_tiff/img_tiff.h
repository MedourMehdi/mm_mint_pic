#include "../headers.h"
#include "../windows.h"

#ifndef IMG_TIFF
#define IMG_TIFF

#ifdef WITH_TIFF
void st_Init_TIFF(struct_window *);
void st_Write_TIFF(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif

#endif
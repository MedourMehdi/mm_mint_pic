#include "../headers.h"
#include "../windows.h"

#ifndef IMG_WEBP
#define IMG_WEBP

void st_Init_WEBP(struct_window *);
void st_Write_WEBP(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif
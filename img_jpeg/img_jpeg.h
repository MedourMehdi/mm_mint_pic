#include "../headers.h"
#include "../windows.h"

#ifndef IMG_JPEG
#define IMG_JPEG

void st_Init_JPEG(struct_window *);
void st_Write_JPEG(u_int8_t* src_buffer, int width, int height, const char* filename);

#endif
#include "../headers.h"
#include "../windows.h"

#ifndef IMG_BMP
#define IMG_BMP

void st_Init_BMP(struct_window *);
void st_Write_BMP(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif
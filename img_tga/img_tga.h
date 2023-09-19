#include "../headers.h"
#include "../windows.h"

#ifndef IMG_TGA
#define IMG_TGA

void st_Init_TGA(struct_window *);
void st_Write_TGA(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif
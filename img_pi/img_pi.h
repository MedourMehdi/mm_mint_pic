#include "../headers.h"
#include "../windows.h"

#ifndef IMG_DEGAS
#define IMG_DEGAS

void st_Init_Degas(struct_window *);
void st_Write_Degas(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif
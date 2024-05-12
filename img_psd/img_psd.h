#include "../headers.h"
#include "../windows.h"

#ifdef WITH_PSD
#ifndef IMG_PSD
#define IMG_PSD

void st_Init_PSD(struct_window *);
void st_Write_PSD(u_int8_t* src_buffer, int width, int height, const char* filename);
#endif
#endif
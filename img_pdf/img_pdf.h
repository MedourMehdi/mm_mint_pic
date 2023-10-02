#include "../headers.h"
#include "../windows.h"

#ifndef IMG_PDF
#define IMG_PDF

void st_Init_PDF(struct_window *);
void st_Write_PDF(u_int8_t* src_buffer, int width, int height, const char* filename);

#endif
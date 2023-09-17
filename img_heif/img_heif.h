#include "../headers.h"
#include "../windows.h"

#ifndef IMG_HEIF
#define IMG_HEIF
#include <libheif/heif.h>

void st_Init_HEIF(struct_window *);
void st_Write_HEIF(uint8_t* bits, int width, int height, const char* filename);
#endif
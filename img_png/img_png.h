#include "../headers.h"
#include "../windows.h"

#ifndef IMG_PNG
#define IMG_PNG

#ifndef PNG_HEADER_LOADED
    #define PNG_HEADER_LOADED
    #include <png.h>
#endif

// void st_Win_Print_PNG(int16_t this_win_handle);
void st_Init_PNG(struct_window *this_win); 
int16_t st_Save_PNG(const char* filename, int width, int height, int bitdepth, int colortype, unsigned char* data, int pitch, int transform = PNG_TRANSFORM_IDENTITY);
#endif
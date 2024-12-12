#include "../headers.h"

#ifndef COLOR_HEADERS
#define COLOR_HEADERS

void st_VDI_SavePalette_LAB(int16_t max_colors);

void classic_ARGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height, int16_t max_colors);

void classic_RGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height, int16_t max_colors);
void rgb2lab_RGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height, int16_t max_colors);

/*
u_int16_t palette_16[16] =
{
    0x0FFF,
    0x0F00,
    0x00F0,
    0x0FF0,
    0x000F,
    0x0F0F,
    0x00FF,
    0x0DDD,
    0x0333,
    0x0F33,
    0x03F3,
    0x0FF3,
    0x033F,
    0x0F3F,
    0x03FF,
    0x0000,
};
*/

#endif
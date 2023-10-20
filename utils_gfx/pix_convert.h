#include "../headers.h"

#ifndef PIXELCONVERTION_HEADERS
#define PIXELCONVERSION_HEADERS

void st_Convert_RGB888_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_RGB565_to_RGB888(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_RGB565_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_ARGB_to_RGB565(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_ARGB_to_RGB888(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_RGB_to_8bits_Indexed(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_Indexed_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_GRAY_to_MONO(MFDB* src_mfdb, MFDB* dst_mfdb);
void st_Convert_Mono_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb);

u_int8_t* st_Convert_RGBA_to_ARGB(u_int8_t* src, u_int16_t width, u_int16_t height);

u_int16_t ARGB_to_RGB565(u_int8_t *ARGBPixel);
u_int8_t RGB_to_8bits_Indexed(u_int8_t *ARGBPixel);
u_int32_t Indexed_to_ARGB(u_int8_t index);
u_int32_t ARGB_to_GRAY(u_int8_t *ARGBPixel);
u_int8_t GRAY_to_MONO(u_int8_t *ARGBPixel);

void st_Color_Transparency_ARGB(u_int32_t* dst_buffered_image, u_int32_t* color, const u_int16_t width, const u_int16_t height);
void st_Color_Transparency_RGB565(u_int16_t* dst_buffered_image, u_int32_t* color, const u_int16_t width, const u_int16_t height);
void st_Color_Transparency_RGB888(u_int8_t* dst_buffered_image, u_int32_t* color, const u_int16_t width, const u_int16_t height);

void st_Rescale_ARGB( MFDB* wi_original_mfdb, MFDB* wi_rendered_mfdb, int16_t dst_width, int16_t dst_height);
void st_Rotate_ARGB( MFDB* wi_original_mfdb, MFDB* wi_rendered_mfdb, int16_t degree);
MFDB* st_MFDB32_To_MFDB16(MFDB* MFDB32);
MFDB* st_MFDB32_To_MFDB24(MFDB* MFDB32);
MFDB* st_MFDB32_To_MFDB8bpp(MFDB* MFDB32);
MFDB* st_MFDB24_To_MFDB32(MFDB* MFDB24);
MFDB* st_MFDB8bpp_to_MFDB32(MFDB* MFDB8);
MFDB* st_MFDB32_To_MFDBGRAY(MFDB* MFDB32);
MFDB* st_MFDB32_To_MFDB1bpp(MFDB* MFDB32);
MFDB* st_MFDB1bpp_to_MFDB32(MFDB* MFDB1bpp);
MFDB* st_MFDB32_To_MFDB4bpp(MFDB* MFDB32);
MFDB* st_MFDB4bpp_to_MFDB32(MFDB* MFDB4bpp, int16_t* this_palette);

void* st_Floyd_Dithering(MFDB* MFDB24, int16_t bpp);
void* st_Sierra_Dithering(MFDB* MFDB24, int16_t bpp);

u_int32_t st_Blend_Pix(u_int32_t background, u_int32_t foreground);
#endif
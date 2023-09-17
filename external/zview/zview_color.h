/* Modified by M.Medour - 2023/09 */

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ZVIEWCOLORS_HEADERS
#define ZVIEWCOLORS_HEADERS

typedef struct {
	uint8_t satur;
	uint8_t red, green, blue;
} SRGB;

uint16_t zview_Get_Closest_Value_sRGB(uint8_t* RGB_ptr);
void zview_VDI_SavePalette_sRGB(int16_t (*_vdi_palette)[3]);
void zview_Build_Cube216(int16_t *pixel_val);
void zview_Dither_RGB_to_8bits(uint8_t* src_ptr, uint8_t* dst_ptr, int16_t width, int16_t height);
void zview_Set_Max_Color(uint16_t color_nb);

#endif

#ifdef __cplusplus
}
#endif

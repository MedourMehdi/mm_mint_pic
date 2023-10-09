/* Modified by M.Medour - 2023/09 */

#include "zview_color.h"
#include <mintbind.h>
#include <string.h>

#ifndef mul_3_fast
#define mul_3_fast(x) ((x << 1) + x)
#endif

#ifndef mul_6_fast
#define mul_6_fast(x) ((x << 2) + (x << 1))
#endif

#ifndef div_1000_fast
#define  div_1000_fast(x) ((x >> 10) + (x >> 15) - (x >> 17) + (x >> 21) + (x >> 24) + (x >> 26) - (x >> 29));
#endif

#ifndef MFDB_STRIDE
#define MFDB_STRIDE(w) (((w) + 15) & -16)
#endif

uint8_t		zview_Saturation(uint8_t * rgb);
uint16_t	zview_Remap_Color (long value, SRGB *screen_colortab);
uint32_t	zview_Color_Lookup ( uint32_t rgb, SRGB *screen_colortab, int16_t *trans);
void       	zview_Save_sRGB_Colors(int16_t (*_vdi_palette)[3], SRGB *screen_colortab);

uint16_t max_color;

SRGB		main_screen_colortab[256];
uint32_t 	cube216[216];

void zview_Set_Max_Color(uint16_t color_nb){
	max_color = color_nb;
}

uint16_t zview_Remap_Color (long value, SRGB *screen_colortab){
	int16_t red   = ((uint8_t *)&value)[1];
	int16_t green = ((uint8_t *)&value)[2];
	int16_t blue  = ((uint8_t *)&value)[3];
    
	int16_t satur = zview_Saturation(((uint8_t *)&value) + 1);
	
	int16_t  best_fit  = 0;
	uint16_t best_err  = 0xFFFFu;
	
	int16_t i = 0;
	
	value = ( value & 0x00FFFFFFl) | (( long)satur << 24);
	
	do 
	{
		if ( *( long*)&screen_colortab[i] == value) 
		{
			/* gotcha! */
			best_fit = i;
			break;
		} 
		else 
		{
			uint16_t err = 
			( red > screen_colortab[i].red ? red - screen_colortab[i].red : screen_colortab[i].red - red)
			+ (green > screen_colortab[i].green ? green - screen_colortab[i].green : screen_colortab[i].green - green)
			+ (blue  > screen_colortab[i].blue  ? blue  - screen_colortab[i].blue  : screen_colortab[i].blue - blue)
			+ (satur > screen_colortab[i].satur ? satur - screen_colortab[i].satur : screen_colortab[i].satur - satur);
			
			if (err <= best_err) 
			{
				best_err = err;
				best_fit = i;
			}
		}
	} while (++i < max_color);
	
	return best_fit;
}

uint8_t zview_Saturation(uint8_t * rgb) {
	if (rgb[0] >= rgb[1]) {
		if (rgb[1] >= rgb[2]) {
			return ( rgb[0]   - rgb[2] );
			}
		else if (rgb[0] > rgb[2]) {
			return ( rgb[0]   - rgb[1] );
			}
		else {
			return ( rgb[2]  - rgb[1] );
			}
	} 
	else if (rgb[1] >= rgb[2]) {
		if (rgb[0] >= rgb[2]) {
	    	return ( rgb[1] - rgb[2] );
			}
		else {
			return ( rgb[1] - rgb[0] );
			}
	} 
	else {
		return (rgb[2]  - rgb[0]);
		}
}

void zview_Save_sRGB_Colors(int16_t (*_vdi_palette)[3], SRGB *screen_colortab){
	uint16_t  i;
	
	for( i = 0; i < max_color; i++)
	{
		screen_colortab[i].red   = div_1000_fast( ((((long)_vdi_palette[i][0] << 8 ) - _vdi_palette[i][0]) + 500) );
		screen_colortab[i].green = div_1000_fast( ((((long)_vdi_palette[i][1] << 8 ) - _vdi_palette[i][1]) + 500) );
		screen_colortab[i].blue  = div_1000_fast( ((((long)_vdi_palette[i][2] << 8 ) - _vdi_palette[i][2]) + 500) );		
		screen_colortab[i].satur = zview_Saturation ( &screen_colortab[i].red);
	}
}

uint16_t zview_Get_Closest_Value_sRGB(uint8_t* RGB_ptr) {
    return zview_Remap_Color(((RGB_ptr[0] << 16) | (RGB_ptr[1] << 8) | RGB_ptr[2]), main_screen_colortab);
}

void zview_Build_Cube216(int16_t *pixel_val){
		uint32_t *dst;
		uint32_t r, g, b;
		dst = cube216;

		for (r = 0x000000uL; r <= 0xFF0000uL; r += 0x330000uL) 
		{
			for (g = 0x000000uL; g <= 0x00FF00uL; g += 0x003300uL) 
			{
				for (b = 0x000000uL; b <= 0x0000FFuL; b += 0x000033uL) 
				{
					*(dst++) = zview_Color_Lookup ( r | g | b, main_screen_colortab, pixel_val);
				}
			}
		}
}

uint32_t zview_Color_Lookup(uint32_t rgb, SRGB *screen_colortab, int16_t *trans){
	uint8_t idx = ((rgb & ~0xFFuL) == ~0xFFuL ? rgb : zview_Remap_Color (rgb, screen_colortab));
	return ( (( long)(trans ? trans[idx] : idx) << 24) | (*(long*)&screen_colortab[idx] & 0x00FFFFFFuL));
}

static inline uint8_t zview_Dither_True ( uint8_t * rgb, int16_t * err, int8_t ** buf){
	int8_t 	*dth = *buf;
	uint16_t  r    = (( err[0] += ( int16_t)dth[0] + rgb[0]) <= 42 ? 0 : err[0] >= 213 ? 5 : mul_3_fast(err[0]) >> 7);
	uint16_t  g    = (( err[1] += ( int16_t)dth[1] + rgb[1]) <= 42 ? 0 : err[1] >= 213 ? 5 : mul_3_fast(err[1]) >> 7);
	uint16_t  b    = (( err[2] += ( int16_t)dth[2] + rgb[2]) <= 42 ? 0 : err[2] >= 213 ? 5 : mul_3_fast(err[2]) >> 7);
	uint8_t * irgb = ( uint8_t*)&cube216[ mul_6_fast( ( mul_6_fast(r) + g) ) + b];
	
	err[0] -= irgb[1];
	dth[0] =  ( err[0] <= -254 ? ( err[0] =- 127) : err[0] >= +254 ? ( err[0] =+ 127) : ( err[0] >>= 1));
	err[1] -= irgb[2];
	dth[1] =  ( err[1] <= -254 ? ( err[1] =- 127) : err[1] >= +254 ? ( err[1] =+ 127) : ( err[1] >>= 1));
	err[2] -= irgb[3];
	dth[2] =  ( err[2] <= -254 ? ( err[2] = -127) : err[2] >= +254 ? ( err[2] = +127) : ( err[2] >>= 1));
	( *buf) += 3;
	
	return irgb[0];
}

void zview_Dither_RGB_to_8bits(uint8_t* src_ptr, uint8_t* dst_ptr, int16_t width, int16_t height){
    uint32_t totalPixels = mul_3_fast(MFDB_STRIDE(width) * height);
	int16_t err[3] = { 0, 0, 0 };
	int8_t *dth = (int8_t*)Mxalloc(totalPixels, 3);
	if(dth == NULL){
		printf("Not enough memory\n");
	}
	memset(dth, 0, totalPixels);
    uint32_t i = 0;
    while(i < totalPixels){
        *dst_ptr++ = zview_Dither_True(&src_ptr[i], err, &dth);
        i = i + 3;
    }
    Mfree(dth);
}

void zview_VDI_SavePalette_sRGB(int16_t (*_vdi_palette)[3]) {
	if(_vdi_palette != NULL){
		zview_Save_sRGB_Colors(_vdi_palette, main_screen_colortab);
	}
}
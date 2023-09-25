#include "pix_convert.h"
#include <libyuv.h>
#include "../external/zview/zview_color.h"
#include "../external/dither/dither.h"
#include "../utils/utils.h"
#include "planar.h"

#include "color.h"

#include "../utils_rsc/progress.h"

bool zview_Color_Init = false;
bool rgb2lab_Color_Init = false;

#define R8(R5) (( R5 * 527 + 23 ) >> 6)
#define G8(G6) (( G6 * 259 + 33 ) >> 6)
#define B8(B5) (( B5 * 527 + 23 ) >> 6)

#define OPAQUE 0xFF
#define TRANSPARENT 0

#define ALPHA(rgb) (u_int8_t)(rgb >> 24)
#define RED(rgb)   (u_int8_t)(rgb >> 16)
#define GREEN(rgb) (u_int8_t)(rgb >> 8)
#define BLUE(rgb)  (u_int8_t)(rgb)

#define UNMULTIPLY(color, alpha) ((0xFF * color) / alpha)
#define BLEND(back, front, alpha) div_255_fast( (front * alpha) + (back * (255 - alpha)) )
#define ARGB(a, r, g, b) ((a << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))

#define	GRAY( r,g,b )   (0.3 * r + 0.59 * g + 0.11 * b)

/* Convert incoming bitmap from RGB565 to RGB8888 */
void st_Convert_RGB565_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb){
    int16_t x, y;
    u_int8_t* pTempBitmap = (u_int8_t*)dst_mfdb->fd_addr;
    u_int8_t* pBitmap = (u_int8_t*)src_mfdb->fd_addr;

    int16_t iWidth = MFDB_STRIDE(src_mfdb->fd_w);
    int16_t iHeight = src_mfdb->fd_h;
    int16_t iPitch = iWidth << 1;

    u_int8_t    *d;
    u_int16_t   *s;
    u_int16_t   us;
    u_int8_t    c;

    for (y=0; y<iHeight; y++) {
       d = &pTempBitmap[(y * iWidth) << 2];
       s = (unsigned short *)&pBitmap[y * iPitch];
       for (x = 0; x < iWidth; x++) {
          c = 0xFF;
          *d++ = c;
          us = *s++;
          c = ((us & 0xf800) >> 8) | ((us & 0xe000) >> 13); // red
          *d++ = c;
          c = ((us & 0x7e0) >> 3) | ((us & 0x600) >> 9); // green
          *d++ = c;
          c = ((us & 0x1f) << 3) | ((us & 0x1c) >> 2); // blue
          *d++ = c;
       }
    }
}

/* Convert incoming bitmap from RGB565 to RGB888 */
void st_Convert_RGB565_to_RGB888(MFDB* src_mfdb, MFDB* dst_mfdb){

    int16_t x, y;
    u_int8_t* pTempBitmap = (u_int8_t*)dst_mfdb->fd_addr;

    int16_t iWidth =  MFDB_STRIDE(src_mfdb->fd_w);
    int16_t iHeight = src_mfdb->fd_h;
    int16_t iPitch = iWidth << 1;

    u_int8_t    *d;
    u_int16_t   *s;
    u_int16_t   us;
    u_int8_t    c;

    u_int8_t* pBitmap = (u_int8_t*)src_mfdb->fd_addr;

    for (y=0; y<iHeight; y++) {
       d = &pTempBitmap[y * iWidth * 3];
       s = (unsigned short *)&pBitmap[y * iPitch];
       for (x = 0; x < iWidth; x++) {
          us = *s++;
          c = ((us & 0xf800) >> 8) | ((us & 0xe000) >> 13); // red
          *d++ = c;
          c = ((us & 0x7e0) >> 3) | ((us & 0x600) >> 9); // green
          *d++ = c;
          c = ((us & 0x1f) << 3) | ((us & 0x1c) >> 2); // blue
          *d++ = c;
       }
    }
}

u_int16_t ARGB_to_RGB565(u_int8_t *ARGBPixel)
{
    u_int16_t b = (ARGBPixel[3] >> 3) & 0x1f;
    u_int16_t g = ((ARGBPixel[2] >> 2) & 0x3f) << 5;
    u_int16_t r = ((ARGBPixel[1] >> 3) & 0x1f) << 11;

    return (u_int16_t) (r | g | b);
}

u_int32_t ARGB_to_GRAY(u_int8_t *ARGBPixel) {
	u_int8_t grayscale_color = GRAY(ARGBPixel[1], ARGBPixel[2], ARGBPixel[3]);
	return (uint32_t)((0xFF << 24) + (grayscale_color << 16) + (grayscale_color << 8) + grayscale_color);
}

u_int8_t RGB_to_8bits_Indexed(u_int8_t *ARGBPixel){
    u_int8_t color_8bits;
    uint32_t color_32bits = ( 0xFF ) << 24 | ( ARGBPixel[0] << 16 ) | ( ARGBPixel[1] << 8 ) |  ARGBPixel[2];

    switch (color_32bits)
    {
    case 0:
        color_8bits = 0xFF;
        break;
    case 0x00FFFFFF:
        color_8bits = 0;
        break;        
    default:
        color_8bits = zview_Get_Closest_Value_sRGB(ARGBPixel);
        break;
    }

    return color_8bits;
}

u_int8_t RGB_to_8bits(u_int8_t *ARGBPixel){
    uint8_t r, g, b;
    r = div_255_fast((ARGBPixel[0] << 3) - ARGBPixel[0]);
    g = div_255_fast((ARGBPixel[1] << 3) - ARGBPixel[1]);
    b = div_255_fast((ARGBPixel[2] << 1) + ARGBPixel[2]);
    u_int8_t color_8bits = r << 5 | g << 2 | b;
    return color_8bits;
}

u_int32_t Indexed_to_ARGB(u_int8_t index){
    uint32_t color_32bits = ( 0xFF ) << 24 | ( (vdi_palette[index][0] * 255 / 1000) << 16 ) | ( (vdi_palette[index][1] * 255 / 1000) << 8 ) |  (vdi_palette[index][2] * 255 / 1000);
    return color_32bits;
}

u_int8_t GRAY_to_MONO(u_int8_t *ARGBPixel){

    u_int8_t color;
    if( (ARGBPixel[0] + ARGBPixel[1] + ARGBPixel[2]) > 0){
    // if( (ARGBPixel[1] + ARGBPixel[2] + ARGBPixel[3]) > 0){
        return 0;
    } else {
        return 1;
    } 
}

void st_Convert_ARGB_to_GRAY(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int32_t* src_ptr = (u_int32_t*)src_mfdb->fd_addr;
    u_int32_t* dst_ptr = (u_int32_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    for (unsigned long index = 0; index < totalPixels; index++)
    {
        *dst_ptr++ = ARGB_to_GRAY((uint8_t*)src_ptr++);
    }
}

void st_Convert_GRAY_to_MONO(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int8_t* dst_ptr = (u_int8_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = (MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h) * 3;
    unsigned long j, i = 0;
    for (j = 0; j < totalPixels; j += 24)
    {
        dst_ptr[i++] = ( (GRAY_to_MONO((uint8_t*)&src_ptr[j]) & 0x01) << 7) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 3]) & 0x01) << 6) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 6]) & 0x01) << 5) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 9]) & 0x01) << 4) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 12]) & 0x01) << 3) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 15]) & 0x01) << 2) |
                    (( GRAY_to_MONO((uint8_t*)&src_ptr[j + 18]) & 0x01) << 1) |
                    (GRAY_to_MONO((uint8_t*)&src_ptr[j + 21]) & 0x01);
    }
}

void st_Convert_Indexed_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int32_t* dst_ptr = (u_int32_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    for (unsigned long index = 0; index < totalPixels; index++)
    {
        *dst_ptr++ = Indexed_to_ARGB(*src_ptr++);
    }

}

void st_Convert_Mono_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int32_t* dst_ptr = (u_int32_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(dst_mfdb->fd_w) * dst_mfdb->fd_h;
    uint32_t i = 0;
    uint32_t k = 0;

    while (i < totalPixels)
    {
        for(int16_t j = 7; j >= 0; j--){
             dst_ptr[i] = ( (src_ptr[k] >> j) & 0x01 ) == 1 ? 0xFF000000 : 0xFFFFFFFF;
             i++;
        }
        k++;
        // st_Progress_Bar_Signal(global_progress_bar, mul_100_fast(i) / totalPixels, (int8_t*)"1bpp to 32bpp in progress");
    }

}

void st_Convert_RGB_to_8bits_Indexed(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int8_t* dst_ptr = (u_int8_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    u_int32_t i = 0;
    for (unsigned long index = 0; index < totalPixels; index++)
    {
        *dst_ptr++ = RGB_to_8bits_Indexed(&src_ptr[i]);
        i = i + 3;
    }
}

void st_Convert_RGB_to_8bits(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int8_t* dst_ptr = (u_int8_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    u_int32_t i = 0;
    for (unsigned long index = 0; index < totalPixels; index++)
    {
        *dst_ptr++ = RGB_to_8bits(&src_ptr[i]);
        i = i + 3;
    }
}

void st_Convert_ARGB_to_RGB565(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int32_t* src_ptr = (u_int32_t*)src_mfdb->fd_addr;
    u_int16_t* dst_ptr = (u_int16_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    for (unsigned long index = 0; index < totalPixels; index++) {
        *dst_ptr++ = ARGB_to_RGB565((u_int8_t*)src_ptr++);
    }
}

void st_Convert_RGB888_to_ARGB(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int32_t* dst_ptr = (u_int32_t*)dst_mfdb->fd_addr;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;

    u_int32_t index = 0, j = 0;
    while (index < (totalPixels)) {
        dst_ptr[index++] = 0xFF << 24 | ( src_ptr[j++] & 0xFF ) << 16| ( src_ptr[j++] & 0xFF ) << 8 | ( src_ptr[j++] & 0xFF );
    }
}

void st_Convert_ARGB_to_RGB888(MFDB* src_mfdb, MFDB* dst_mfdb){

    u_int8_t* src_ptr = (u_int8_t*)src_mfdb->fd_addr;
    u_int32_t* dst_ptr = (u_int32_t*)dst_mfdb->fd_addr;

    u_int8_t r1, g1, b1, r2, g2, b2;
    u_int8_t r3, g3, b3, r4, g4, b4;

    const size_t totalPixels = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h;
    u_int32_t index24 = 0;
    u_int32_t index = 0;
    while (index < (totalPixels << 2))
    {
        index++;
        r1 = src_ptr[index++];
        g1 = src_ptr[index++];
        b1 = src_ptr[index++];
        index++;
        r2 = src_ptr[index++];
        g2 = src_ptr[index++];
        b2 = src_ptr[index++];
        index++;
        r3 = src_ptr[index++];
        g3 = src_ptr[index++];
        b3 = src_ptr[index++];
        index++;
        r4 = src_ptr[index++];
        g4 = src_ptr[index++];
        b4 = src_ptr[index++];

        dst_ptr[index24++] = (r1 << 24) | (g1 << 16) | (b1 << 8) | r2;
        dst_ptr[index24++] = (g2 << 24) | (b2 << 16) | (r3 << 8) | g3;
        dst_ptr[index24++] = (b3 << 24) | (r4 << 16) | (g4 << 8) | b4;
    }
}

void st_Color_Transparency_ARGB(u_int32_t* dst_buffered_image, u_int32_t* color, const uint16_t width, const uint16_t height) {
    const size_t totalPixels = MFDB_STRIDE(width) * height;

	const u_int32_t colorAlpha = ALPHA(*color);
	const u_int8_t colorR = UNMULTIPLY(RED(*color), colorAlpha);
	const u_int8_t colorG = UNMULTIPLY(GREEN(*color), colorAlpha);
	const u_int8_t colorB = UNMULTIPLY(BLUE(*color), colorAlpha);

    for (unsigned long index = 0; index < totalPixels; index++) {
        const u_int8_t dst_buffered_imageR = RED(*dst_buffered_image);
        const u_int8_t dst_buffered_imageG = GREEN(*dst_buffered_image);
        const u_int8_t dst_buffered_imageB = BLUE(*dst_buffered_image);

        const u_int32_t R = BLEND(dst_buffered_imageR, colorR, colorAlpha);
        const u_int32_t G = BLEND(dst_buffered_imageG, colorG, colorAlpha);
        const u_int32_t B = BLEND(dst_buffered_imageB, colorB, colorAlpha);

        *dst_buffered_image++ = ARGB(OPAQUE, R , G, B);
    }
}

void st_Color_Transparency_RGB888(u_int8_t* dst_buffered_image, u_int32_t* color, const uint16_t width, const uint16_t height) {
    const size_t totalPixels = MFDB_STRIDE(width) * height;

    u_int32_t *dst_ptr = (u_int32_t*)dst_buffered_image;

	const u_int32_t colorAlpha = ALPHA(*color);
	const u_int8_t colorR = UNMULTIPLY(RED(*color), colorAlpha);
	const u_int8_t colorG = UNMULTIPLY(GREEN(*color), colorAlpha);
	const u_int8_t colorB = UNMULTIPLY(BLUE(*color), colorAlpha);
    
    u_int32_t index24 = 0;
    u_int32_t i = 0;
    for (unsigned long index = 0; index < totalPixels >> 2; index++) {
        const u_int32_t r1 = BLEND(dst_buffered_image[i++], colorR, colorAlpha);
        const u_int32_t g1 = BLEND(dst_buffered_image[i++], colorG, colorAlpha);
        const u_int32_t b1 = BLEND(dst_buffered_image[i++], colorB, colorAlpha);

        const u_int32_t r2 = BLEND(dst_buffered_image[i++], colorR, colorAlpha);
        const u_int32_t g2 = BLEND(dst_buffered_image[i++], colorG, colorAlpha);
        const u_int32_t b2 = BLEND(dst_buffered_image[i++], colorB, colorAlpha);

        const u_int32_t r3 = BLEND(dst_buffered_image[i++], colorR, colorAlpha);
        const u_int32_t g3 = BLEND(dst_buffered_image[i++], colorG, colorAlpha);
        const u_int32_t b3 = BLEND(dst_buffered_image[i++], colorB, colorAlpha);

        const u_int32_t r4 = BLEND(dst_buffered_image[i++], colorR, colorAlpha);
        const u_int32_t g4 = BLEND(dst_buffered_image[i++], colorG, colorAlpha);
        const u_int32_t b4 = BLEND(dst_buffered_image[i++], colorB, colorAlpha);

        dst_ptr[index24++] = ((r1 & 0xFF) << 24) | ((g1 & 0xFF) << 16) | ((b1 & 0xFF) << 8) | (r2 & 0xFF);
        dst_ptr[index24++] = ((g2 & 0xFF) << 24) | ((b2 & 0xFF) << 16) | ((r3 & 0xFF) << 8) | (g3 & 0xFF);
        dst_ptr[index24++] = ((b3 & 0xFF) << 24) | ((r4 & 0xFF) << 16) | ((g4 & 0xFF) << 8) | (b4 & 0xFF);

    }
}

void st_Color_Transparency_RGB565(u_int16_t* dst_buffered_image, u_int32_t* color, const uint16_t width, const uint16_t height) {
    const size_t totalPixels = MFDB_STRIDE(width) * height;

	const u_int32_t colorAlpha = ALPHA(*color);
	const u_int8_t colorR = UNMULTIPLY(RED(*color), colorAlpha);
	const u_int8_t colorG = UNMULTIPLY(GREEN(*color), colorAlpha);
	const u_int8_t colorB = UNMULTIPLY(BLUE(*color), colorAlpha);

    for (unsigned long index = 0; index < totalPixels; index++) {
		u_int32_t* pix32 = (u_int32_t *)mem_alloc(sizeof(u_int32_t));
		const u_int8_t R5 = (*dst_buffered_image >> 11) & 0x1F;
		const u_int8_t G6 = (*dst_buffered_image >> 5) & 0x3F;
		const u_int8_t B5 = *dst_buffered_image & 0x1F;

		*pix32 =  ((OPAQUE << 24) | ((R8(R5) & 0xFF) << 16) | ((G8(G6) & 0xFF) << 8) | (B8(B5) & 0xFF));
        const u_int8_t dst_buffered_imageR = RED(*pix32);
        const u_int8_t dst_buffered_imageG = GREEN(*pix32);
        const u_int8_t dst_buffered_imageB = BLUE(*pix32);

        const u_int32_t R = BLEND(dst_buffered_imageR, colorR, colorAlpha);
        const u_int32_t G = BLEND(dst_buffered_imageG, colorG, colorAlpha);
        const u_int32_t B = BLEND(dst_buffered_imageB, colorB, colorAlpha);

		*pix32 = ARGB(OPAQUE, R , G, B);
        *dst_buffered_image++ = ARGB_to_RGB565((u_int8_t*)pix32);
		mem_free(pix32);
    }
}

void st_Rescale_ARGB( MFDB* wi_original_mfdb, MFDB* wi_rendered_mfdb, int16_t dst_width, int16_t dst_height){

    void *old_ptr = wi_rendered_mfdb->fd_addr;

    u_int8_t *rescale_buffer = st_ScreenBuffer_Alloc_bpp(dst_width, dst_height, 32);

    mfdb_update_bpp(wi_rendered_mfdb, (int8_t*)rescale_buffer, dst_width, dst_height, 32);

    libyuv::ARGBScale((u_int8_t *)wi_original_mfdb->fd_addr, 
    MFDB_STRIDE(wi_original_mfdb->fd_w) << 2 , 
    wi_original_mfdb->fd_w, 
    wi_original_mfdb->fd_h, 
    rescale_buffer, MFDB_STRIDE(wi_rendered_mfdb->fd_w) << 2, wi_rendered_mfdb->fd_w, wi_rendered_mfdb->fd_h, (libyuv::FilterMode)3);
    if(old_ptr != NULL){
        mem_free(old_ptr);
    }
}

void st_Rotate_ARGB( MFDB* wi_original_mfdb, MFDB* wi_rendered_mfdb, int16_t degree){

    int16_t width, height;

    if(degree == 180){
        width = wi_original_mfdb->fd_w;
        height = wi_original_mfdb->fd_h;
    }else{
        width = wi_original_mfdb->fd_h;
        height = wi_original_mfdb->fd_w;
    }
    void *old_ptr = wi_rendered_mfdb->fd_addr;

    u_int8_t *rescale_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
    mfdb_update_bpp(wi_rendered_mfdb, (int8_t*)rescale_buffer, width, height, 32);
    st_MFDB_Fill(wi_rendered_mfdb, 0xFFFFFFFF);
    libyuv::ARGBRotate((u_int8_t *)wi_original_mfdb->fd_addr, MFDB_STRIDE(wi_original_mfdb->fd_w) << 2, 
                        rescale_buffer, MFDB_STRIDE(wi_rendered_mfdb->fd_w) << 2, 
                        wi_original_mfdb->fd_w, wi_original_mfdb->fd_h, (libyuv::RotationMode)degree);
    if(old_ptr != NULL){
        mem_free(old_ptr);
    }
}

MFDB* st_MFDB32_To_MFDB16(MFDB* MFDB32){
    u_int8_t* dst_buffer = st_ScreenBuffer_Alloc_bpp(MFDB32->fd_w, MFDB32->fd_h, 16);
    MFDB* MFDB16 = mfdb_alloc_bpp((int8_t*)dst_buffer, MFDB32->fd_w, MFDB32->fd_h, 16);
    // printf("===> MFDB32 W %d / MFDB16 W%d\n", MFDB32->fd_w, MFDB16->fd_w);
    st_Convert_ARGB_to_RGB565(MFDB32, MFDB16);

    return MFDB16;
}

/* 24BPP */

MFDB* st_MFDB32_To_MFDB24(MFDB* MFDB32){
    int8_t* dst_buffer = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB32->fd_w, MFDB32->fd_h, 24);
    MFDB* dst_rgb888 = mfdb_alloc_bpp(dst_buffer, MFDB32->fd_w, MFDB32->fd_h, 24);

    st_Convert_ARGB_to_RGB888(MFDB32, dst_rgb888);

    return dst_rgb888;
}

MFDB* st_MFDB24_To_MFDB32(MFDB* MFDB24){
        u_int8_t    *src_buffer = (u_int8_t *)MFDB24->fd_addr;
        int8_t*     dst_buffer = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB24->fd_w, MFDB24->fd_h, 32);
        MFDB*       dst_argb = mfdb_alloc_bpp(dst_buffer, MFDB24->fd_w, MFDB24->fd_h, 32);

        u_int32_t   dst_size = MFDB_STRIDE(MFDB24->fd_w) * MFDB24->fd_h << 2;
        u_int32_t   i = 0, j = 0;

        while(i < dst_size){
            dst_buffer[i++] = 0xFF;
            dst_buffer[i++] = src_buffer[j++];
            dst_buffer[i++] = src_buffer[j++];
            dst_buffer[i++] = src_buffer[j++];
        }

        return dst_argb;
}

/* GRAYSCALE */

MFDB* st_MFDB32_To_MFDBGRAY(MFDB* MFDB32){
    int8_t* dst_buffer = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB32->fd_w, MFDB32->fd_h, 32);
    MFDB*   dst_rgb_gray = mfdb_alloc_bpp(dst_buffer, MFDB32->fd_w, MFDB32->fd_h, 32);

    st_Convert_ARGB_to_GRAY(MFDB32, dst_rgb_gray);

    return dst_rgb_gray;
}

/* 8BPP */

MFDB* st_MFDB32_To_MFDB8bpp(MFDB* MFDB32){

    int16_t bpp = screen_workstation_bits_per_pixel;
    bool use_zview_dithering = false;
    bool force_planar_mode = false;
    bool use_rgb2lab = false;
    bool disable_classic_dithering = false;

    if(edDi_present && screen_workstation_bits_per_pixel < 16){
        use_zview_dithering = true;
    }

    if(MFDB32->fd_r2){
        bpp = MFDB32->fd_r2;
        use_rgb2lab = true;
        use_zview_dithering = false;
        force_planar_mode = true;
        MFDB32->fd_r2 = 0;
    }
    if(MFDB32->fd_r3){
        disable_classic_dithering = true;
        MFDB32->fd_r3 = 0;
    }
    int16_t max_colors = (1 << bpp);    

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"ARGB to 256 colors");
    st_Progress_Bar_Signal(global_progress_bar, 10, (int8_t*)"Init");

    if(!zview_Color_Init && use_zview_dithering){
        st_Progress_Bar_Signal(global_progress_bar, 20, (int8_t*)"Palette prefetching");
        zview_Set_Max_Color(max_colors);
        zview_VDI_SavePalette_sRGB(vdi_palette);
        zview_Build_Cube216(pix_palette);
        zview_Color_Init = true;
    }

    MFDB* MFDB24 = st_MFDB32_To_MFDB24(MFDB32);

    int8_t* dst_buffer_8bits = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB24->fd_w, MFDB24->fd_h, 8);
    MFDB* MFDB8C = mfdb_alloc_bpp(dst_buffer_8bits, MFDB24->fd_w, MFDB24->fd_h, 8);
    /* screen_workstation_format == 1 => Whole planes */
    if(screen_workstation_format > 0 && !force_planar_mode){
        st_Progress_Bar_Signal(global_progress_bar, 20, (int8_t*)"Floyd dithering");
        st_Floyd_Dithering(MFDB24, screen_workstation_bits_per_pixel);
        st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"RGB to 8bits conversion");
        st_Convert_RGB_to_8bits(MFDB24, MFDB8C);
    } else {

        if(use_zview_dithering){
            st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"edDi Dithering RGB to 8bits");
            zview_Dither_RGB_to_8bits((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
        }else{
            st_Progress_Bar_Signal(global_progress_bar, 30, (int8_t*)"Floyd dithering");
            st_Floyd_Dithering(MFDB24, bpp);
            if(use_rgb2lab) {  
                st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"rgb2lab_RGB_to_8bits_Indexed");      
                rgb2lab_RGB_to_8bits_Indexed((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h, max_colors);
            } else {
                st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"classic_RGB_to_8bits_Indexed"); 
                classic_RGB_to_8bits_Indexed((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h, max_colors);
            }            
        }
    }

    mfdb_free(MFDB24);

    if(screen_workstation_format < 2 || force_planar_mode){
        st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"8bpp Chunky to 8bpp Planar");
        MFDB* MFDB8P = st_Chunky_to_Planar_8bits(MFDB8C);
        mfdb_free(MFDB8C);
        st_Progress_Bar_Step_Done(global_progress_bar);
        st_Progress_Bar_Finish(global_progress_bar);
        return MFDB8P;
    } else {
        st_Progress_Bar_Step_Done(global_progress_bar);
        st_Progress_Bar_Finish(global_progress_bar);
        return MFDB8C;
    }
}

MFDB* st_MFDB8bpp_to_MFDB32(MFDB* MFDB8){

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"256 colors to ARGB");
    st_Progress_Bar_Signal(global_progress_bar, 10, (int8_t*)"Init");

    int8_t* dst_buffer_32bits = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB8->fd_w, MFDB8->fd_h, 32);
    MFDB* MFDB32 = mfdb_alloc_bpp(dst_buffer_32bits, MFDB8->fd_w, MFDB8->fd_h, 32);

    if(screen_workstation_format == 0){
    st_Progress_Bar_Signal(global_progress_bar, 30, (int8_t*)"8bpp planar to chunky");
    MFDB* MFDB8C = st_Planar_to_Chunky_8bits(MFDB8);
    st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"Index to ARGB");
    st_Convert_Indexed_to_ARGB(MFDB8C, MFDB32);
    mfdb_free(MFDB8C);
    } else {
        st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"Index to ARGB");
        st_Convert_Indexed_to_ARGB(MFDB8, MFDB32);
    }

    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);

    return MFDB32;
}

/* 1BPP */

MFDB* st_MFDB32_To_MFDB1bpp(MFDB* MFDB32){

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"ARGB to Mono");
    st_Progress_Bar_Signal(global_progress_bar, 30, (int8_t*)"ARGB to GRAY in progress");

    MFDB* MFDBGRAY = st_MFDB32_To_MFDBGRAY(MFDB32);
    MFDB* MFDB24 = st_MFDB32_To_MFDB24(MFDBGRAY);

    st_Floyd_Dithering(MFDB24, 1);

    st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"GRAY to Mono in progress");

    int8_t* dst_buffer_1bpp = (int8_t*)mem_alloc((MFDB_STRIDE(MFDB24->fd_w) * MFDB24->fd_h) >> 3);
    memset(dst_buffer_1bpp, 0, ((MFDB_STRIDE(MFDB24->fd_w) * MFDB24->fd_h) >> 3));
    MFDB* dst_1bpp = mfdb_alloc_bpp(dst_buffer_1bpp, MFDB24->fd_w, MFDB24->fd_h, 1);

    st_Convert_GRAY_to_MONO(MFDB24, dst_1bpp);

    mfdb_free(MFDBGRAY);
    mfdb_free(MFDB24);

    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);

    return dst_1bpp;
}

MFDB* st_MFDB1bpp_to_MFDB32(MFDB* MFDB1bpp){

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"Mono to ARGB");
    st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"1bpp to 32bpp in progress");

    int8_t* dst_buffer_32bits = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB1bpp->fd_w, MFDB1bpp->fd_h, 32);
    MFDB* MFDB32 = mfdb_alloc_bpp(dst_buffer_32bits, MFDB1bpp->fd_w, MFDB1bpp->fd_h, 32);
    st_Convert_Mono_to_ARGB(MFDB1bpp, MFDB32);

    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);

    return MFDB32;
}

/* 4BPP */

MFDB* st_MFDB32_To_MFDB4bpp(MFDB* MFDB32){

    int16_t bpp = screen_workstation_bits_per_pixel;
    bool use_zview_dithering = false;
    bool use_rgb2lab = false;
    bool disable_classic_dithering = false;

    if(edDi_present && screen_workstation_bits_per_pixel < 16){
        use_zview_dithering = true;
    }

    if(MFDB32->fd_r2){
        bpp = MFDB32->fd_r2;
        use_rgb2lab = true;
        use_zview_dithering = false;
        MFDB32->fd_r2 = 0;
    }
    if(MFDB32->fd_r3){
        disable_classic_dithering = true;
        MFDB32->fd_r3 = 0;
    }
    int16_t max_colors = (1 << bpp);

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"ARGB to 16 colors");
    st_Progress_Bar_Signal(global_progress_bar, 10, (int8_t*)"Palette fetching");

    if(!zview_Color_Init && use_zview_dithering){
        st_Progress_Bar_Signal(global_progress_bar, 20, (int8_t*)"Palette vectors building");
        zview_Set_Max_Color(max_colors);
        zview_VDI_SavePalette_sRGB(vdi_palette);
        zview_Build_Cube216(pix_palette);
        zview_Color_Init = true;
    }

    if(!rgb2lab_Color_Init && use_rgb2lab){
        st_VDI_SavePalette_LAB(max_colors);
    }

    MFDB* MFDB24 = st_MFDB32_To_MFDB24(MFDB32);

    int8_t* dst_buffer_8bpp = (int8_t*)st_ScreenBuffer_Alloc_bpp(MFDB24->fd_w, MFDB24->fd_h, 8);
    MFDB* MFDB8C = mfdb_alloc_bpp(dst_buffer_8bpp, MFDB24->fd_w, MFDB24->fd_h, 8);

    if(use_zview_dithering) {
        st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"edDi Dithering RGB to 8bits");
        zview_Dither_RGB_to_8bits((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
    } else {
        if(!disable_classic_dithering){
            st_Progress_Bar_Signal(global_progress_bar, 30, (int8_t*)"Floyd dithering");
            st_Floyd_Dithering(MFDB24, bpp);
        }
        st_Progress_Bar_Signal(global_progress_bar, 60, (int8_t*)"RGB to 8bits indexed image (may be long)");
        if(use_rgb2lab) {  
            rgb2lab_RGB_to_8bits_Indexed((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h, max_colors);
        } else {
            classic_RGB_to_8bits_Indexed((uint8_t*)MFDB24->fd_addr, (uint8_t*)MFDB8C->fd_addr, MFDB24->fd_w, MFDB24->fd_h, max_colors);
        }
    }

    mfdb_free(MFDB24);

    st_Progress_Bar_Signal(global_progress_bar, 75, (int8_t*)"8bpp indexed image to 4bpp");
    MFDB* MFDB4P = st_Chunky8bpp_to_Planar_4bpp(MFDB8C);
    mfdb_free(MFDB8C);

    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);

    return MFDB4P;
}

MFDB* st_MFDB4bpp_to_MFDB32(MFDB* MFDB4bpp, int16_t* this_palette){

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"4bpp to ARGB");
    st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"4bpp to 32bpp in progress");

    MFDB* MFDB32 = st_Planar4bpp_to_chunky32bpp(MFDB4bpp, (u_int16_t*)this_palette);

    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);

    return MFDB32;
}

void* st_Floyd_Dithering(MFDB* MFDB24, int16_t bpp){

    switch (bpp)
    {
    case 1:
        makeDitherFS((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
        break;        
    case 4:
        makeDitherFSRgb3bpp((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
        break;
    case 8:
        makeDitherFSRgb6bpp((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
        break;
    default:
        break;
    }
    return NULL;
}

void* st_Sierra_Dithering(MFDB* MFDB24, int16_t bpp){

    switch (bpp)
    {
    case 1:
        makeDitherSierra((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h);
        break;        
    case 4:
        makeDitherSierraRgbNbpp((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h, 1);
        break;
    case 8:
        makeDitherSierraRgbNbpp((u_int8_t*)MFDB24->fd_addr, MFDB24->fd_w, MFDB24->fd_h, 3);
        break;
    default:
        break;
    }
    return NULL;
}
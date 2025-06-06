#include "planar.h"

/* chunky to planar routine : */
extern "C" {
#if WITH_VASM
    void c2p1x1_8_falcon(void * planar, void * chunky, u_int32_t count);
#endif
}

#include "../utils/utils.h"
#include "../external/zview/zview_planar.h"

inline void st_C2P_8bpp_4px(u_int8_t* destination_buffer, u_int8_t* src_data, u_int16_t k);
inline void st_C2P_4bpp_8px(u_int8_t* destination_buffer, u_int8_t* src_data);

inline void convert_4bpp_chunky32bpp(u_int8_t *src_buffer, u_int32_t *temp_buffer, u_int16_t *palette, u_int16_t width);

MFDB* st_Chunky_to_Planar_8bits(MFDB* source_mfdb){

    u_int16_t dest_width = source_mfdb->fd_w;
    u_int16_t dest_height = source_mfdb->fd_h;
    u_int8_t* src_data = (u_int8_t*)source_mfdb->fd_addr;

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(dest_width, dest_height, 8);
    MFDB* destination_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, dest_width, dest_height, 8);
    memset((void*)destination_buffer, 0x00, dest_height * dest_width);
	u_int32_t totalpixels, i;
    totalpixels = MFDB_STRIDE(dest_width)* dest_height;

#if WITH_VASM
    c2p1x1_8_falcon(destination_buffer, src_data, totalpixels);
#else
    for(i = 0; i < totalpixels; i += 16){
        st_C2P_8bpp_4px(&destination_buffer[i], &src_data[i], 0);
        st_C2P_8bpp_4px(&destination_buffer[i + 4], &src_data[i], 2);
        st_C2P_8bpp_4px(&destination_buffer[i + 8], &src_data[i], 4);
        st_C2P_8bpp_4px(&destination_buffer[i + 12], &src_data[i], 6);            
    }
#endif
	return destination_mfdb;
}

inline void st_C2P_8bpp_4px(u_int8_t* destination_buffer, u_int8_t* src_data, u_int16_t k){
    u_int32_t i, m, n;

    m = 0;
    for(i = 0; i < 2; i++, n = 0, k++){
        // n = 0;
        destination_buffer[ m++ ] = 
                            (((src_data[ n++ ] >> k) & 0x01) << 7) +
                            (((src_data[ n++ ] >> k) & 0x01) << 6) +
                            (((src_data[ n++ ] >> k) & 0x01) << 5) +
                            (((src_data[ n++ ] >> k) & 0x01) << 4) +
                            (((src_data[ n++ ] >> k) & 0x01) << 3) +  
                            (((src_data[ n++ ] >> k) & 0x01) << 2) +
                            (((src_data[ n++ ] >> k) & 0x01) << 1) +
                            ((src_data[ n++ ] >> k) & 0x01);
        destination_buffer[ m++ ] = 
                            (((src_data[ n++ ] >> k) & 0x01) << 7) +
                            (((src_data[ n++ ] >> k) & 0x01) << 6) +
                            (((src_data[ n++ ] >> k) & 0x01) << 5) +
                            (((src_data[ n++ ] >> k) & 0x01) << 4) +
                            (((src_data[ n++ ] >> k) & 0x01) << 3) +  
                            (((src_data[ n++ ] >> k) & 0x01) << 2) +
                            (((src_data[ n++ ] >> k) & 0x01) << 1) +
                            ((src_data[ n++ ] >> k) & 0x01);
    }
}

MFDB* st_Planar_to_Chunky_8bits(MFDB* source_mfdb){

    u_int16_t dest_width = source_mfdb->fd_w;
    u_int16_t dest_height = source_mfdb->fd_h;
    u_int8_t* src_data = (u_int8_t*)source_mfdb->fd_addr;

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(dest_width, dest_height, 8);
    MFDB* destination_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, dest_width, dest_height, 8);
    memset((void*)destination_buffer, 0x00, dest_height * dest_width);
	u_int32_t totalpixels, i;
    totalpixels = MFDB_STRIDE(dest_width)* dest_height;

    u_int32_t x, y;
    for(y = 0; y < dest_height; y++){
        i = (y * MFDB_STRIDE(dest_width));               
        planar8_to_chunky8( (u_int8_t*)&src_data[i], (u_int8_t*)&destination_buffer[i], MFDB_STRIDE(dest_width));
    }    

	return destination_mfdb;
}

MFDB* st_Chunky8bpp_to_Planar_4bpp(MFDB* source_mfdb){

    u_int16_t dest_width = source_mfdb->fd_w;
    u_int16_t dest_height = source_mfdb->fd_h;
    u_int8_t* src_data = (u_int8_t*)source_mfdb->fd_addr;

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(dest_width, dest_height, 4);
    MFDB* destination_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, dest_width, dest_height, 4);

    u_int8_t* dest_4bpp_C = st_ScreenBuffer_Alloc_bpp(dest_width, dest_height, 4);

	u_int32_t i;
    uint32_t totalpixels = (MFDB_STRIDE(dest_width)* dest_height) >> 1;

    for(i = 0; i < totalpixels; i++){
        dest_4bpp_C[ i ] =  ( ( reverse(src_data[ i << 1 ]) & 0xF0) ) | ( ( reverse(src_data[ (i << 1) + 1]) & 0xF0) >> 4 ) ;      
    }
    for(i = 0; i < totalpixels; i += 8){
        st_C2P_4bpp_8px(&destination_buffer[i], &dest_4bpp_C[i]);
    }
    mem_free(dest_4bpp_C);

	return destination_mfdb;
}

inline void st_C2P_4bpp_8px(u_int8_t* destination_buffer, u_int8_t* src_data){
    for ( int16_t j = 7, l = 3 ; j >= 4, l >= 0; j--, l-- ){
        *destination_buffer++ = ( 
            ( (((*src_data >> j) & 0x01) << 3 ) | (((*src_data++ >> l) & 0x01) << 2) |
            (((*src_data >> j) & 0x01) << 1) | ((*src_data++ >> l) & 0x01) ) << 4 ) | 
            ( (((*src_data >> j) & 0x01) << 3 ) | (((*src_data++ >> l) & 0x01) << 2) |
            (((*src_data >> j) & 0x01) << 1) | ((*src_data++ >> l) & 0x01) );
        *destination_buffer++ = (
            ( (((*src_data >> j) & 0x01) << 3 ) | (((*src_data++ >> l) & 0x01) << 2) |
            (((*src_data >> j) & 0x01) << 1) |  ((*src_data++ >> l) & 0x01)) << 4 ) |
            ( (((*src_data >> j) & 0x01) << 3 ) | (((*src_data++ >> l) & 0x01) << 2) |
            (((*src_data >> j) & 0x01) << 1) | ((*src_data++ >> l) & 0x01) );
        src_data -= 8;
    }
}

MFDB *st_Planar4bpp_to_chunky32bpp(MFDB* MFDB4bpp, u_int16_t *palette) {
    int16_t y, i;

    u_int8_t *src_buffer = (uint8_t*)MFDB4bpp->fd_addr;
    u_int16_t width = (uint16_t)MFDB4bpp->fd_w;
    uint16_t height = (uint16_t)MFDB4bpp->fd_h;

    u_int32_t *dst_buffer;
    
    dst_buffer = (u_int32_t *)st_ScreenBuffer_Alloc_bpp(width, height, 32);

    MFDB* destination_mfdb = mfdb_alloc_bpp((int8_t*)dst_buffer, width, height, 32);

    for(y = 0; y < height; y++){
        convert_4bpp_chunky32bpp(&src_buffer[(MFDB_STRIDE(width) >> 1) * y], &dst_buffer[MFDB_STRIDE(width) * y], palette, MFDB_STRIDE(width));
    }
    return destination_mfdb;
}

inline void convert_4bpp_chunky32bpp(u_int8_t *src_buffer, u_int32_t *temp_buffer, u_int16_t *palette, u_int16_t width){
    int16_t     i, j, k;
    u_int8_t    pix4;
    u_int16_t   color;
    u_int8_t    r, g, b, a = 0xFF;

    i = 0;
    k = 0;
    while (i < width) {
        pix4 = 0;
        for(j = 7; j >= 0; j--){
            pix4 = ((src_buffer[k] >> j & 0x01) << 3)
                | ((src_buffer[k + 2] >> j & 0x01) << 2)
                | ((src_buffer[k + 4] >> j & 0x01) << 1)
                | ((src_buffer[k + 6] >> j & 0x01));
            color = palette[ reverse(pix4) >> 4 ];

            r = (((color >> 8) & 0x07) << 5) | (((color >> 8) & 0x07) << 2) | ((color >> 9) & 0x03);
            g = (((color >> 4) & 0x07) << 5) | (((color >> 4) & 0x07) << 2) | ((color >> 5) & 0x03);
            b = ((color & 0x07)  << 5) | ((color & 0x07) << 2) | (color & 0x03);

            temp_buffer[i] = a << 24 | r << 16 | g << 8 | b;
            i++;
        }
        for(j = 7; j >= 0; j--){
            pix4 = ((src_buffer[k + 1] >> j & 0x01) << 3)
                | ((src_buffer[k + 3] >> j & 0x01) << 2)
                | ((src_buffer[k + 5] >> j & 0x01) << 1)
                | ((src_buffer[k + 7] >> j & 0x01));
            color = palette[ reverse(pix4) >> 4 ];

            r = (((color >> 8) & 0x07) << 5) | (((color >> 8) & 0x07) << 2) | ((color >> 9) & 0x03);
            g = (((color >> 4) & 0x07) << 5) | (((color >> 4) & 0x07) << 2) | ((color >> 5) & 0x03);
            b = ((color & 0x07)  << 5) | ((color & 0x07) << 2) | (color & 0x03);

            temp_buffer[i] = a << 24 | r << 16 | g << 8 | b;
            i++;
        }
        k += 8;
    }
}

/*
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// Example indexed Atari palette (initialize with your specific colors)
uint32_t atari_palette[256];

// Function to calculate the distance between two colors
static inline int colorDistance(uint32_t color1, uint32_t color2) {
    // Extract RGB values from 32-bit color
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    // Calculate Euclidean distance in RGB space
    return (r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2);
}

// Function to find the nearest color index in the palette
static uint8_t findNearestColor(uint32_t pixel) {
    int minDistance = INT32_MAX;
    uint8_t closestIndex = 0;

    for (uint8_t i = 0; i < 256; ++i) {
        int distance = colorDistance(pixel & 0x00FFFFFF, atari_palette[i]);
        if (distance < minDistance) {
            minDistance = distance;
            closestIndex = i;
        }
    }

    return closestIndex;
}

// Function to convert a chunky 32bpp image to an 8bpp planar image using an indexed palette
void chunkyToPlanar_Atari(uint32_t *chunky, uint8_t *planar, int width, int height, int chunkyPitch, int planarPitch) {
    // Initialize planar output to zero
    for (int i = 0; i < planarPitch * height / 8; ++i) {
        planar[i] = 0;
    }

    // Loop through each pixel in the chunky buffer
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate the pixel index in the chunky format
            uint32_t pixel = chunky[y * chunkyPitch + x];

            // Find the nearest color index in the palette
            uint8_t index = findNearestColor(pixel);

            // Write to the planar buffer (bitwise operations for a planar format)
            int bit_position = 7 - (x % 8);
            planar[(y * planarPitch) + (x / 8)] |= (index << bit_position); // Set the appropriate bit for the index
        }
    }
}
*/

/*
#include <stdio.h>
#include <stdint.h>

extern void chunkyToPlanar_Atari(uint32_t* chunky_image, uint8_t* planar_image, int width, int height);
extern uint32_t atari_palette[256]; // Declare the Atari palette as an external array

int main() {
    // Example dimensions (can be modified based on actual image size)
    int width = 320;
    int height = 240;

    // Allocate memory for the images
    uint32_t* chunky_image = (uint32_t*)malloc(width * height * sizeof(uint32_t));
    uint8_t* planar_image = (uint8_t*)malloc((width * height) / 8);

    if (!chunky_image || !planar_image) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    // Initialize the chunky_image with sample data (example)
    // In practice, load your actual image data into chunky_image here.
    for (int i = 0; i < width * height; ++i) {
        chunky_image[i] = (255 << 24) | (rand() % 256 << 16) | (rand() % 256 << 8) | (rand() % 256); // ARGB format
    }

    // Convert chunky to planar
    chunkyToPlanar_Atari(chunky_image, planar_image, width, height);

    // Process planar_image as needed. 

    // Cleanup
    free(chunky_image);
    free(planar_image);

    return 0;
}

*/

/*

*/
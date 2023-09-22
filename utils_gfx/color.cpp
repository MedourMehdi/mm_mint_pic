#include "color.h"

#include "../external/rgb2lab/rgb2lab.h"
#include <math.h>

/* Classic color distance declaration */
inline u_int32_t distance_rgb( u_int16_t* RGB1, u_int16_t* RGB2 );
inline u_int16_t get_closest_value(u_int8_t* RGB_ptr, int16_t max_colors);

/* RGB2LAB declarations */
double main_vdi_palette_lab[256][3];
double deltaE(double* labA, double* labB);
u_int16_t get_closest_value_rgb2lab(u_int8_t* RGB_ptr, int16_t max_colors);

/* Classic color distance routines */

inline u_int32_t distance_rgb( u_int16_t* RGB1, u_int16_t* RGB2 ) {
    u_int32_t rez = 0;

    u_int32_t r = (RGB1[0] - RGB2[0]);
    u_int32_t g = (RGB1[1] - RGB2[1]);
    u_int32_t b = (RGB1[2] - RGB2[2]);

    u_int32_t drp2 = (r*r);
    u_int32_t dgp2 = (g*g);
    u_int32_t dbp2 = (b*b);    

    u_int32_t t = (RGB1[0] + RGB2[0]) >> 1;

    rez = sqrt((drp2 << 1) + (dgp2 << 2) + mul_3_fast(dbp2) + t * (drp2 - dbp2) >> 8);

    return rez;
}

inline u_int16_t get_closest_value(u_int8_t* RGB_ptr, int16_t max_colors) {
    u_int16_t i = 0;
    u_int32_t j = 0, best_idx = 0;

    u_int16_t better_distance = 0xFFFF;

    u_int16_t pal_value[3];

    u_int16_t RGB[3];

    RGB[0] = RGB_ptr[0];
    RGB[1] = RGB_ptr[1];
    RGB[2] = RGB_ptr[2];    

	while(i < max_colors ) {

        switch (computer_type)
        {
        case 0:
            pal_value[0] = ((palette_ori[i] >> 8) & 0x0F ) << 5 ;
            pal_value[1] = ((palette_ori[i] >> 4) & 0x0F ) << 5 ;
            pal_value[2] = ((palette_ori[i]) & 0x0F ) << 5 ;            
            break;
        default:
            pal_value[0] =  ( ((palette_ori[i] >> 7) & 0x0E ) | ((palette_ori[i] >> 11) & 0x01 ) ) << 4;
            pal_value[1] = ( ((palette_ori[i] >> 3) & 0x0E) | ((palette_ori[i] >> 7) & 0x01 ) ) << 4;
            pal_value[2] = ( ((palette_ori[i]) & 0x07 ) << 1 | ((palette_ori[i] >> 3) & 0x01 ) ) << 4;
            break;
        }

        if(computer_type == 1){

        } else {

        }        
        j = distance_rgb(RGB,pal_value);

        if( j < better_distance ){
            better_distance = j;
            best_idx = i;
        }
        i++;
	}

    return best_idx;
}

void classic_RGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height, int16_t max_colors){
    u_int32_t totalPixels = mul_3_fast(MFDB_STRIDE(width) * height);
    u_int32_t i = 0;
    while(i < totalPixels){
        *dst_ptr++ = (u_int8_t)get_closest_value(&src_ptr[i], max_colors);
        i = i + 3;
    }
}

/* RGB2LAB routines */

void st_VDI_SavePalette_LAB(int16_t max_colors) {
	u_int16_t i;
	DoubleTriplet this_pal, result_pal_lab;    

	for(i = 0; i < max_colors; i++) {
        // if(edDi_present && screen_workstation_bits_per_pixel < 16){
        //     this_pal.r = ((double)(_vdi_palette[i][0]) / 1000);
        //     this_pal.g = ((double)(_vdi_palette[i][1]) / 1000);
        //     this_pal.b = ((double)(_vdi_palette[i][2]) / 1000);
        // } else 
        if(computer_type > 0) {
            this_pal.r = (double)(( ( ((palette_ori[i] >> 7) & 0x0E ) | ((palette_ori[i] >> 11) & 0x01 ) ) << 4 )) / 255 ;
            this_pal.g = (double)((( ((palette_ori[i] >> 3) & 0x0E) | ((palette_ori[i] >> 7) & 0x01 ) ) << 4 )) / 255 ;
            this_pal.b = (double)((( ((palette_ori[i]) & 0x07 ) << 1 | ((palette_ori[i] >> 3) & 0x01 ) ) << 4 )) / 255 ;                    
        } else {
            this_pal.r = (double)( (((palette_ori[i] >> 8) & 0x0F ) << 5)) / 255  ;
            this_pal.g = (double)( (((palette_ori[i] >> 4) & 0x0F ) << 5)) / 255  ;
            this_pal.g = (double)( (((palette_ori[i]) & 0x0F ) << 5)) / 255  ;            
        }

		result_pal_lab = labFromRgb(this_pal);
		main_vdi_palette_lab[i][0] = result_pal_lab.L;
		main_vdi_palette_lab[i][1] = result_pal_lab.A;
		main_vdi_palette_lab[i][2] = result_pal_lab.B;
	}
}

double deltaE(double* labA, double* labB){
  double deltaL = labA[0] - labB[0];
  double deltaA = labA[1] - labB[1];
  double deltaB = labA[2] - labB[2];
  double c1 = sqrt(labA[1] * labA[1] + labA[2] * labA[2]);
  double c2 = sqrt(labB[1] * labB[1] + labB[2] * labB[2]);
  double deltaC = c1 - c2;
  double deltaH = deltaA * deltaA + deltaB * deltaB - deltaC * deltaC;
  deltaH = deltaH < 0 ? 0 : sqrt(deltaH);
  double sc = 1.0 + 0.045 * c1;
  double sh = 1.0 + 0.015 * c1;
  double deltaLKlsl = deltaL / (1.0);
  double deltaCkcsc = deltaC / (sc);
  double deltaHkhsh = deltaH / (sh);
  double i = deltaLKlsl * deltaLKlsl + deltaCkcsc * deltaCkcsc + deltaHkhsh * deltaHkhsh;
  return i < 0 ? 0 : sqrt(i);
}

u_int16_t get_closest_value_rgb2lab(u_int8_t* RGB_ptr, int16_t max_colors){

    u_int16_t i = 0;
    u_int16_t best_idx = 0;
    double j = 0;

    double better_distance = 0x1.fffffffffffffp1023;

    DoubleTriplet this_rgb, result_rgb_lab;

    double LAB1[3], LAB2[3];

    this_rgb.r = (double)RGB_ptr[0] / 255;
    this_rgb.g = (double)RGB_ptr[1] / 255;
    this_rgb.b = (double)RGB_ptr[2] / 255;

    result_rgb_lab = labFromRgb(this_rgb);

    LAB1[0] = result_rgb_lab.L;
    LAB1[1] = result_rgb_lab.A;
    LAB1[2] = result_rgb_lab.B;

	while(i < max_colors) {

        LAB2[0] = main_vdi_palette_lab[i][0];
        LAB2[1] = main_vdi_palette_lab[i][1];   
        LAB2[2] = main_vdi_palette_lab[i][2];   

        j = deltaE(&LAB2[0], &LAB1[0]);

        if( j < better_distance ){
            better_distance = j;
            best_idx = i;
        }
        i++;
	}

    return best_idx;
}

void rgb2lab_RGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height, int16_t max_colors){
    u_int32_t totalPixels = mul_3_fast(MFDB_STRIDE(width) * height);
    u_int32_t i = 0;
    while(i < totalPixels){
        *dst_ptr++ = (u_int8_t)get_closest_value_rgb2lab(&src_ptr[i], max_colors);
        i = i + 3;
    }
}

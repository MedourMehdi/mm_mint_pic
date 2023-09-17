#include "color.h"

inline u_int32_t distance_rgb( u_int16_t* RGB1, u_int16_t* RGB2 );
inline u_int16_t get_closest_value(u_int8_t* RGB_ptr);


inline float fast_sqrt(float val)  {
        union
        {
            int32_t tmp;
            float val;
        } u;
        u.val = val;
        u.tmp -= 1<<23; /* Remove last bit so 1.0 gives 1.0 */
        /* tmp is now an approximation to logbase2(val) */
        u.tmp >>= 1; /* divide by 2 */
        u.tmp += 1<<29; /* add 64 to exponent: (e+127)/2 =(e/2)+63, */
        /* that represents (e/2)-64 but we want e/2 */
        return u.val;
}

inline int32_t fast_square(int32_t n)
{
    // handle negative input
    if (n < 0)
        n = -n;
 
    // Initialize result
    int32_t res = n;
 
    // Add n to res n-1 times
    for (int32_t i = 1; i < n; i++)
        res += n;
 
    return res;
}

inline u_int32_t distance_rgb( u_int16_t* RGB1, u_int16_t* RGB2 ) {
    u_int32_t rez = 0;

    u_int32_t r = (RGB1[0] - RGB2[0]);
    u_int32_t g = (RGB1[1] - RGB2[1]);
    u_int32_t b = (RGB1[2] - RGB2[2]);

    u_int32_t drp2 = fast_square(r);
    u_int32_t dgp2 = fast_square(g);
    u_int32_t dbp2 = fast_square(b);

    u_int32_t t = (RGB1[0] + RGB2[0]) >> 1;

    rez = fast_sqrt((drp2 << 1) + (dgp2 << 2) + mul_3_fast(dbp2) + t * (drp2 - dbp2) >> 8);

    return rez;
}

inline u_int16_t get_closest_value(u_int8_t* RGB_ptr) {
    u_int16_t i = 0;
    u_int32_t j = 0, best_idx = 0;

    u_int16_t better_distance = 0xFFFF;

    u_int16_t pal_value[3];

    u_int16_t RGB[3];

    RGB[0] = RGB_ptr[0];
    RGB[1] = RGB_ptr[1];
    RGB[2] = RGB_ptr[2];    

	while(i < (1 << screen_workstation_bits_per_pixel) ) {
        // /* STF Pal 16 colors */
        // pal_value[0] = ((palette_ori[i] >> 8) & 0x07 ) << 5 ;
        // pal_value[1] = ((palette_ori[i] >> 4) & 0x07 ) << 5 ;
        // pal_value[2] = ((palette_ori[i]) & 0x07 ) << 5 ;

        if(computer_type == 1){
            pal_value[0] =  ( ((palette_ori[i] >> 7) & 0x0E ) | ((palette_ori[i] >> 11) & 0x01 ) ) << 4;
            pal_value[1] = ( ((palette_ori[i] >> 3) & 0x0E) | ((palette_ori[i] >> 7) & 0x01 ) ) << 4;
            pal_value[2] = ( ((palette_ori[i]) & 0x07 ) << 1 | ((palette_ori[i] >> 3) & 0x01 ) ) << 4;
        } else {
            pal_value[0] = ((palette_ori[i] >> 8) & 0x0F ) << 5 ;
            pal_value[1] = ((palette_ori[i] >> 4) & 0x0F ) << 5 ;
            pal_value[2] = ((palette_ori[i]) & 0x0F ) << 5 ;
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

void classic_RGB_to_8bits_Indexed(u_int8_t* src_ptr, u_int8_t* dst_ptr, int16_t width, int16_t height){
    u_int32_t totalPixels = mul_3_fast(MFDB_STRIDE(width) * height);
    u_int32_t i = 0;
    while(i < totalPixels){
        *dst_ptr++ = (u_int8_t)get_closest_value(&src_ptr[i]);
        i = i + 3;
    }
}
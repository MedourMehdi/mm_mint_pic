#include "planar.h"
#include "../utils/utils.h"

#include "../external/zview/zview_planar.h"

inline void st_C2P_8bpp_4px(u_int8_t* destination_buffer, u_int8_t* src_data, u_int16_t k);
inline void st_C2P_4bpp_8px(u_int8_t* destination_buffer, u_int8_t* src_data);

MFDB* st_Chunky_to_Planar_8bits(MFDB* source_mfdb){

    u_int16_t dest_width = source_mfdb->fd_w;
    u_int16_t dest_height = source_mfdb->fd_h;
    u_int8_t* src_data = (u_int8_t*)source_mfdb->fd_addr;

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(dest_width, dest_height, 8);
    MFDB* destination_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, dest_width, dest_height, 8);
    memset((void*)destination_buffer, 0x00, dest_height * dest_width);
	u_int32_t totalpixels, i;
    totalpixels = MFDB_STRIDE(dest_width)* dest_height;

    for(i = 0; i < totalpixels; i += 16){
        st_C2P_8bpp_4px(&destination_buffer[i], &src_data[i], 0);
        st_C2P_8bpp_4px(&destination_buffer[i + 4], &src_data[i], 2);
        st_C2P_8bpp_4px(&destination_buffer[i + 8], &src_data[i], 4);
        st_C2P_8bpp_4px(&destination_buffer[i + 12], &src_data[i], 6);            
    }
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

    // if(edDi_present){
    //     for(i = 0; i < totalpixels; i++){
    //         dest_4bpp_C[ i ] =  ( ( reverse(src_data[ i << 1 ]) & 0xF0) ) | ( ( reverse(src_data[ (i << 1) + 1]) & 0xF0) >> 4 ) ;      
    //     }        
    // } else {
    //     for(i = 0; i < totalpixels; i++){
    //         dest_4bpp_C[ i ] =   ( (src_data[ i << 1 ] & 0x0F) << 4 ) | ( src_data[ (i << 1) + 1 ] & 0x0F ) ;      
    //     }
    // }

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
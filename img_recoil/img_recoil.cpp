


#include "img_recoil.h"
#include "../img_handler.h"
#include "../utils/utils.h"
#ifdef WITH_RECOIL

#include "../external/recoil/recoil-stdio.h"

void st_Win_Print_Recoil(int16_t this_win_handle);
void _st_Read_Recoil(int16_t this_win_handle, boolean file_process);


void st_Init_Recoil(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_Recoil;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_Recoil(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_Recoil(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_Recoil(int16_t this_win_handle, boolean file_process){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){

    const char *input_file = this_win->wi_data->path;

	RECOIL *recoil = RECOILStdio_New();
	if (recoil == NULL) {
        sprintf(alert_message, "Out Of Mem Error");
        st_form_alert(FORM_EXCLAM, alert_message);
		return;
	}

	if (!RECOILStdio_Load(recoil, input_file)) {
        sprintf(alert_message, "%s\nFile decoding error", input_file);
        st_form_alert(FORM_EXCLAM, alert_message);
		return;
	}

		u_int16_t width = RECOIL_GetWidth(recoil) ;
		u_int16_t height = RECOIL_GetHeight(recoil);
        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);

        u_int32_t total_pixels = width * height;

		u_int8_t* src_RGB = (u_int8_t*)RECOIL_GetPixels(recoil);

        u_int32_t* dst_ARGB = (u_int32_t*)destination_buffer;

        for(int y = 0; y < height; y++){
            for(int x = 0; x < width; x++){
                int i = (MFDB_STRIDE(width) * y) + x;
                // int j = ((width * y) + x) * 3;
                dst_ARGB[i] = (*src_RGB++) << 24 | (*src_RGB++) << 16 | (*src_RGB++) << 8 | *src_RGB++ ;
            }                
        }

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;
        RECOIL_Delete(recoil);	
	}
}

#endif
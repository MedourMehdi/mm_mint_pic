#include "img_dummy.h"
#include "../img_handler.h"

#include "../utils/utils.h"

void st_Win_Print_Dummy(int16_t this_win_handle);
void _st_Read_Dummy(int16_t this_win_handle, boolean file_process);

void st_Init_Dummy(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_Dummy;
    if(use_cached_icons){
        this_win->render_win = NULL;
    } else {
        if(!st_Set_Renderer(this_win)){
            sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
            st_form_alert(FORM_STOP, alert_message);
            return;
        }
    }
}

void st_Win_Print_Dummy(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_Dummy(this_win_handle, this_win->prefers_file_instead_mem);

    if(use_cached_icons){
        st_Start_Window_Process(this_win);
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
        this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
        this_win->total_length_w = this_win->wi_to_display_mfdb->fd_w;
        this_win->total_length_h = this_win->wi_to_display_mfdb->fd_h;

        if(!this_win->wi_data->autoscale){st_Limit_Work_Area(this_win);}
        this_win->wi_data->remap_displayed_mfdb = FALSE;        
        st_End_Window_Process(this_win);
    } else {
        if( st_Img32b_To_Window(this_win) == false ){
            st_form_alert(FORM_STOP, alert_message);
        }    
    }
}

void _st_Read_Dummy(int16_t this_win_handle, boolean file_process){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
		u_int16_t width = 120;
		u_int16_t height = CONTROLBAR_H;
        u_int16_t bpp_wanted = 32;
        if(this_win->wi_data->control_bar_media && use_cached_icons){
            bpp_wanted = screen_workstation_bits_per_pixel;
        }

        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, bpp_wanted);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", use_cached_icons ? (width * height * (bpp_wanted >> 2)) : (width * height) / (8 / bpp_wanted ) );
            st_form_alert(FORM_EXCLAM, alert_message);
        }

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, bpp_wanted);

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;	
	}
}
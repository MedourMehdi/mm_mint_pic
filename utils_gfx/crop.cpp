#include "crop.h"
#include "pix_convert.h"

#include "../utils/utils.h"

#define MIN_CROPW 64
#define MIN_CROPH 64

void st_Crop_Refresh_Img32b(int16_t this_win_handle);

bool st_Crop_To_MFDB(struct_crop* this_crop){

    u_int8_t *ptr;
    u_int32_t totalPixels;
    u_int32_t index = 0;

	struct_window *this_win_master = detect_window(this_crop->master_win_handle);
    int16_t nb_components_32bits = 4;

    if(this_win_master = NULL){
        sprintf(alert_message, "Error win handle is null");
        st_form_alert(FORM_STOP, alert_message);
    }

	if(this_crop->wi_crop_original != NULL){
		mfdb_free(this_crop->wi_crop_original);
	}
    
    TRACE(("this_crop->rect_crop_array.g_w %d, this_crop->rect_crop_array.g_h %d\n", this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h))
	u_int8_t* crop_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, nb_components_32bits << 3);
	this_crop->wi_crop_original = mfdb_alloc_bpp((int8_t*)crop_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, nb_components_32bits << 3);
 
	u_int8_t* tmp_buffer;
    MFDB* tmp_mfdb;

    MFDB *MFDB32 = NULL;
    MFDB *MFDB8P = NULL;

	int16_t xy[8];

	xy[0] = this_crop->rect_crop_array.g_x ;
	xy[1] = this_crop->rect_crop_array.g_y ;
	xy[2] = xy[0] + this_crop->wi_crop_original->fd_w - 1 ;
	xy[3] = xy[1] + this_crop->wi_crop_original->fd_h - 1;

	xy[4] = 0; xy[5] = 0;
	xy[6] = this_crop->wi_crop_original->fd_w - 1; xy[7] = this_crop->wi_crop_original->fd_h - 1;
	// xy[2] -= 1;	xy[3] -= 1;	

    switch (screen_workstation_bits_per_pixel){
        case 1:
            tmp_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            tmp_mfdb = mfdb_alloc_bpp((int8_t*)tmp_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, tmp_mfdb);

            MFDB32 = st_MFDB1bpp_to_MFDB32(tmp_mfdb);

            mfdb_update_bpp(this_crop->wi_crop_original, (int8_t*)MFDB32->fd_addr, MFDB32->fd_w, MFDB32->fd_h, 32);

            mem_free(MFDB32); /* This free MFDB structure and keep fd_addr buffer */
            mfdb_free(tmp_mfdb);
            return true;
            
            break;
        case 4:
            tmp_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            tmp_mfdb = mfdb_alloc_bpp((int8_t*)tmp_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);

            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, tmp_mfdb);

            tmp_mfdb->fd_w = MAX(tmp_mfdb->fd_w, MIN_CROPW);
            tmp_mfdb->fd_h = MAX(tmp_mfdb->fd_h, MIN_CROPH);

            MFDB32 = st_MFDB4bpp_to_MFDB32(tmp_mfdb, palette_ori);
            mfdb_update_bpp(this_crop->wi_crop_original, (int8_t*)MFDB32->fd_addr, MFDB32->fd_w, MFDB32->fd_h, 32);
            /* Trix to signal "no dithering" to st_MFDB4bpp_to_MFDB32 function */
            this_crop->wi_crop_original->fd_r3 = 1;
            mem_free(MFDB32); /* This free MFDB structure and keep fd_addr buffer */
            mfdb_free(tmp_mfdb);
            return true;
            
            break;                 
        case 8:

            tmp_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);

            tmp_mfdb = mfdb_alloc_bpp((int8_t*)tmp_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, tmp_mfdb);

            MFDB32 = st_MFDB8bpp_to_MFDB32(tmp_mfdb);

            mfdb_update_bpp(this_crop->wi_crop_original, (int8_t*)MFDB32->fd_addr, MFDB32->fd_w, MFDB32->fd_h, 32);
            this_crop->wi_crop_original->fd_r3 = 1;
            mem_free(MFDB32); /* This free MFDB structure and keep fd_addr buffer */
            mfdb_free(tmp_mfdb);
            return true;

            break;        
        case 16:
            tmp_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            tmp_mfdb = mfdb_alloc_bpp((int8_t*)tmp_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, tmp_mfdb);
            st_Convert_RGB565_to_ARGB(tmp_mfdb, this_crop->wi_crop_original);
            mfdb_free(tmp_mfdb);
            return true;
            break;
        case 24:
            tmp_buffer = st_ScreenBuffer_Alloc_bpp(this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);
            tmp_mfdb = mfdb_alloc_bpp((int8_t*)tmp_buffer, this_crop->rect_crop_array.g_w, this_crop->rect_crop_array.g_h, screen_workstation_bits_per_pixel);                
            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, tmp_mfdb);
            st_Convert_RGB888_to_ARGB(tmp_mfdb, this_crop->wi_crop_original);
            mfdb_free(tmp_mfdb);
            return true;
            break;
        case 32:
            vro_cpyfm(st_vdi_handle, S_ONLY, xy, &screen_mfdb, this_crop->wi_crop_original);
            /* Fixing Alpha Channel */
            ptr = (u_int8_t*)this_crop->wi_crop_original->fd_addr;
            totalPixels = MFDB_STRIDE(this_crop->wi_crop_original->fd_w) * this_crop->wi_crop_original->fd_h;
            while (index < totalPixels)
            {
                ptr[index << 2] = 0xFF;
                index++;
            }
            return true;
            break;
        default:
            sprintf(alert_message, "Sorry\n%dbpp not supported\nPlz fixe me!", screen_workstation_bits_per_pixel);
            st_form_alert(FORM_STOP, alert_message);        
            return false;
            break;
    }
    
}

void st_Crop_Refresh_Img32b(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    st_Img32b_To_Window(this_win);
}

void st_Init_Crop(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
    this_win->refresh_win = st_Crop_Refresh_Img32b;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    this_win->wi_data->img.original_width = this_win->wi_original_mfdb.fd_w;
    this_win->wi_data->img.original_height = this_win->wi_original_mfdb.fd_h;
    this_win->wi_data->img.scaled_pourcentage = 0;
    this_win->wi_data->img.rotate_degree = 0;
    this_win->wi_data->wi_buffer_modified = FALSE;
    this_win->wi_data->stop_original_data_load = FALSE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
}

void st_Crop_Start(int16_t this_win_handle){

	struct_window*	this_win = detect_window(this_win_handle);
	this_win->wi_data->crop_requested = true;
	wind_update(BEG_MCTRL);
	graf_mouse(THIN_CROSS,0L);
}

bool st_Crop_Finish(int16_t this_win_handle, int16_t mouse_x, int16_t mouse_y){
	struct_window*	this_win = detect_window(this_win_handle);

    int16_t rubber_w = 0;
    int16_t rubber_h = 0;
				
    graf_rubberbox( mouse_x, mouse_y, MIN_CROPW, MIN_CROPH, &rubber_w, &rubber_h );

    wind_update(END_MCTRL);
	
    graf_mouse(ARROW,0L);

    this_win->wi_data->crop_requested = false;
    if(this_win->wi_crop != NULL){
        mem_free(this_win->wi_crop);
    }

    rubber_w = MAX(rubber_w, MIN_CROPW);
    rubber_h = MAX(rubber_h, MIN_CROPH);

    this_win->wi_crop = (struct_crop*)mem_alloc(sizeof(struct_crop));
    this_win->wi_crop->master_win_handle = this_win_handle;

    this_win->wi_crop->rect_crop_array.g_x = mouse_x;
    this_win->wi_crop->rect_crop_array.g_y = mouse_y;
    this_win->wi_crop->rect_crop_array.g_w = rubber_w;
    this_win->wi_crop->rect_crop_array.g_h = rubber_h;

    this_win->wi_crop->wi_crop_original = NULL;

    return st_Crop_To_MFDB(this_win->wi_crop);
}

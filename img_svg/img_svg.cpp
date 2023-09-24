#include "img_SVG.h"
#include "../img_handler.h"

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include "../external/nanosvg/nanosvg.h"
#include "../external/nanosvg/nanosvgrast.h"

#include "../utils/utils.h"

void st_Win_Print_SVG(int16_t this_win_handle);
void _st_Read_SVG(int16_t this_win_handle, boolean file_process);


void st_Init_SVG(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
	this_win->refresh_win = st_Win_Print_SVG;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;

    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_SVG(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->needs_refresh == TRUE){
        this_win->wi_data->wi_original_modified = FALSE;
        this_win->wi_data->needs_refresh = FALSE;
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    } 

    _st_Read_SVG(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_SVG(int16_t this_win_handle, boolean file_process){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->wi_original_modified == FALSE){

        NSVGimage *image = NULL;
        NSVGrasterizer *rast = NULL;

	    u_int16_t width, height;
        u_int8_t alpha_channel, red, green, blue;
        u_int8_t* destination_buffer;
        u_int32_t *dst_ptr;

        printf("parsing %s\n", this_win->wi_data->path );
        image = nsvgParseFromFile(this_win->wi_data->path, "px", 96.0f);
        if (image == NULL) {
            printf("Could not open SVG image.\n");
            goto error;
        }

        // width = (u_int16_t)image->width > 0 ? (u_int16_t)image->width : 320;
        // height = (u_int16_t)image->height > 0 ? (u_int16_t)image->height : 200;

        width = (u_int16_t)image->width;
        height = (u_int16_t)image->height;

        if(!width || !height){
            sprintf(alert_message, "Sorry:\nCan't rasterize this image");
            st_form_alert(FORM_EXCLAM, alert_message);
            goto error;            
        }

        destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
            st_form_alert(FORM_EXCLAM, alert_message);
            goto error;
        }

        rast = nsvgCreateRasterizer();

        if (rast == NULL) {
            printf("Could not init rasterizer.\n");
            goto error;
        }

        printf("rasterizing image %d x %d\n", width, height);
        
        nsvgRasterize(rast, image, 0, 0, 1, destination_buffer, width, height, MFDB_STRIDE(width) << 2);

        dst_ptr = (u_int32_t*)destination_buffer;
        for(int16_t y = 0; y < height; y++ ){
            for(int16_t x = 0; x < width; x++){
                dst_ptr[(y * MFDB_STRIDE(width)) + x] = ((dst_ptr[(y * MFDB_STRIDE(width)) + x] & 0x000000FF) << 24 ) | ((dst_ptr[(y * MFDB_STRIDE(width)) + x] & 0xFFFFFF00) >> 8);
            }
        }

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;
        this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
        this_win->total_length_h = this_win->wi_original_mfdb.fd_h;     
        this_win->wi_data->wi_original_modified = TRUE;
        this_win->wi_data->wi_buffer_modified = FALSE;	

        error:
            nsvgDeleteRasterizer(rast);
            nsvgDelete(image);        		
	}

}
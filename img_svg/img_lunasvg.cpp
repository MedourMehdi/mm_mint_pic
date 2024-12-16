#include "img_svg.h"
#ifdef WITH_LUNASVG
#include "../img_handler.h"

#include <lunasvg/lunasvg.h>

using namespace lunasvg;

#include "../utils/utils.h"
#include "../utils_gfx/pix_convert.h"

#include "../rsc_processing/progress_bar.h"

void st_Win_Print_SVG(int16_t this_win_handle);
void _st_Read_SVG(int16_t this_win_handle, boolean file_process);

void st_Init_SVG(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_SVG;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_SVG(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
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

    if(this_win->wi_data->stop_original_data_load == FALSE){
        std::unique_ptr<Document> image;
        Bitmap rast;


	    int16_t width = -1, height = -1;
        u_int8_t alpha_channel, red, green, blue;
        u_int8_t* destination_buffer;
        u_int32_t *src_ptr, *dst_ptr;
        uint32_t pix_pos, pix_pos_stride;

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "SVG READING", 15,  "SVG: processing...");
        image = Document::loadFromFile(this_win->wi_data->path);
        if(image == nullptr){
            sprintf(alert_message, "Could not open SVG image:\n%s", this_win->wi_data->path);
            st_form_alert(FORM_EXCLAM, alert_message);            
            goto error;
        }
        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 55, "SVG -> RGBA");
        rast = image->renderToBitmap(-1,-1,0xFFFFFFFF);
        if(rast.isNull()){
            sprintf(alert_message, "Could not init rasterizer.\n");
            st_form_alert(FORM_EXCLAM, alert_message);            
            goto error;
        }

        width = (u_int16_t)rast.width();
        height = (u_int16_t)rast.height();
        
        printf("width = %d heigh = %d\n", width, height);

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

        // st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 75, "RGBA -> ARGB");

        // destination_buffer = rast.data();
        src_ptr = (u_int32_t*)rast.data();
        dst_ptr = (u_int32_t*)destination_buffer;
        for(int16_t y = 0; y < height; y++ ){
            pix_pos = y * width;
            pix_pos_stride = y * MFDB_STRIDE(width);
            for(int16_t x = 0; x < width; x++){
                // u_int32_t fg_col = ((dst_ptr[(y * MFDB_STRIDE(width)) + x] & 0x000000FF) << 24 ) | ((dst_ptr[(y * MFDB_STRIDE(width)) + x] & 0xFFFFFF00) >> 8);
                dst_ptr[pix_pos_stride + x] = src_ptr[pix_pos + x];
            }
        }

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
        TRACE(("mfdb_update_bpp()\n"))
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

    error:
        // rast.release();
        // image.release();

        // plutovg_surface_destroy(rast);
        // plutosvg_document_destroy(image);

        st_Win_Progress_Bar_Finish(this_win->wi_handle);

	}

}
#endif
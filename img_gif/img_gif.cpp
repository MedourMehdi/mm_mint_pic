#include "img_gif.h"

#include "../utils/utils.h"
#include "../img_handler.h"

#include "../utils_gfx/pix_convert.h"
#include "../utils_gfx/ttf.h"

#include "../utils_rsc/progress.h"
#include "../thumbs/thumbs.h"

#ifndef TTF_DEFAULT_PATH
#define TTF_DEFAULT_PATH "./fonts/arial.ttf"
#endif
#ifndef PRIMARY_IMAGE_ID
#define PRIMARY_IMAGE_ID    -1
#endif

void _st_Read_GIF(int16_t this_win_handle, boolean file_processing, int32_t img_id);
void _st_Handle_Thumbs_GIF(int16_t this_win_handle, boolean file_process);
void _st_Handle_Thumbs_GIF_Generic(int16_t this_win_handle, boolean file_process);

void st_Win_Print_GIF(int16_t);


void st_Init_GIF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_GIF;
    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }    
    /* thumbnails stuff */
    if(this_win->wi_thumb == NULL){
        if(cpu_type < 40){
            _st_Handle_Thumbs_GIF_Generic(this_win->wi_handle, this_win->prefers_file_instead_mem);
        }else{
            _st_Handle_Thumbs_GIF(this_win->wi_handle, this_win->prefers_file_instead_mem);
        }
        
    }
}

void st_Win_Print_GIF(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_GIF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);
    
    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_GIF(int16_t this_win_handle, boolean file_processing, int32_t img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    if(this_win->wi_data->stop_original_data_load == FALSE){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"GIF READING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");

        const char *file_name = this_win->wi_data->path;
        int32_t this_img = 0;
        int error;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 35, (int8_t*)"Opening file");
        GifFileType* gifFile = DGifOpenFileName(file_name, &error);
        if (!gifFile) {
            sprintf(alert_message, "DGifOpenFileName() failed - %d", error);
            st_form_alert(FORM_STOP, alert_message);        

        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);        
            DGifCloseFile(gifFile, &error);

        }
        
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 55, (int8_t*)"Reading headers");
        ColorMapObject* commonMap = gifFile->SColorMap;

        if(img_id > PRIMARY_IMAGE_ID){
            this_img = img_id;
        }
        u_int16_t width = gifFile->SWidth;
        u_int16_t height = gifFile->SHeight;

        GraphicsControlBlock image_gcb;
        DGifSavedExtensionToGCB(gifFile, this_img, &image_gcb);
		int32_t trans = image_gcb.TransparentColor;
        int32_t disposal = image_gcb.DisposalMode;
		int32_t image_delay = image_gcb.DelayTime;

        u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)temp_buffer, width, height, 32);
        
        st_MFDB_Fill(&this_win->wi_original_mfdb, 0XFFFFFFFF);
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 85, (int8_t*)"Building ARGB image");
        int32_t ii, jj, x, y;
        u_int32_t* ptr_argb = (u_int32_t*)temp_buffer;        
        int16_t img_current = 0;
        while( img_current <= this_img ){
            const SavedImage& saved = gifFile->SavedImages[img_current];
            const GifImageDesc& desc = saved.ImageDesc;
            const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;

            u_int8_t alpha ;
            int colorCount = colorMap->ColorCount;

            for(int z = 0; z < saved.ExtensionBlockCount; z++)
            {
                ExtensionBlock * block = &saved.ExtensionBlocks[z];
                if(block->Function == 249)
                {
                    if(block->Bytes[0])
                        alpha = (u_int8_t)block->Bytes[3];
                }
            }
                
            for(y = 0; y < desc.Height; y++){
                for(x = 0; x < desc.Width; x++){
                    ii = (x + desc.Left) + ((y + desc.Top) * MFDB_STRIDE(width));
                    int c = saved.RasterBits[ (y * desc.Width) + x];
                    if(c == alpha || c >= colorCount || c == gifFile->SBackGroundColor){
                        if(img_current == 0 || (c == gifFile->SBackGroundColor && c < colorCount && c >= 0 && trans == -1 && disposal != DISPOSE_BACKGROUND) ){                         
                            ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                        }else{
                            continue;
                        }
                    } else if (colorMap) {
                        ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                    }
                }
            }
            img_current++;
        }

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
        this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
        this_win->wi_data->stop_original_data_load = TRUE;
        this_win->wi_data->wi_buffer_modified = FALSE;

        DGifCloseFile(gifFile, &error);
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        
    }
}


void _st_Handle_Thumbs_GIF(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

	const char *file_name;
    GifFileType* gifFile;
    int error;
    u_int16_t idx = 0;

    if( file_process == TRUE ){
        file_name = this_win->wi_data->path;
        gifFile = DGifOpenFileName(file_name, &error);
        
        if (!gifFile) {
            sprintf(alert_message, "DGifOpenFileName() failed - %d", error);
            st_form_alert(FORM_STOP, alert_message);        
            // return false;
        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);        
            DGifCloseFile(gifFile, &error);
            // return false;
        }
    }

    this_win->wi_data->img.img_total = gifFile->ImageCount;
    this_win->wi_data->img.img_id = idx;
    this_win->wi_data->img.img_index = idx + 1;
    if(this_win->wi_data->img.img_total > 1){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"Thumbs processing");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 1, (int8_t*)"Init");

        ColorMapObject* commonMap = gifFile->SColorMap;

        u_int16_t final_width = gifFile->SWidth;
        u_int16_t final_height = gifFile->SHeight;

        u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(final_width, final_height, 32);
        MFDB* temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, final_width, final_height, 32);

        u_int16_t wanted_width = 100;
        u_int16_t wanted_height = 140;
        if(final_height < final_width){
            wanted_width = 140;
            wanted_height = 100;            
        }
        u_int16_t wanted_padx = 8;
        u_int16_t wanted_pady = 8;

        this_win->wi_data->thumbnail_slave = true;
        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, wanted_padx, wanted_pady, wanted_width, wanted_height);

        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;
        this_win->wi_thumb->thumbs_nb = this_win->wi_data->img.img_total;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
            this_win->wi_thumb->thumbs_list_array[i].thumb_id = i;
            this_win->wi_thumb->thumbs_list_array[i].thumb_index = i + 1;

            char progess_bar_indication[96];
            int16_t bar_pos = mul_100_fast(i) / this_win->wi_thumb->thumbs_nb;
            sprintf(progess_bar_indication, "Thumbnail id.%d/%d - Image id.%d", i, this_win->wi_thumb->thumbs_nb, this_win->wi_thumb->thumbs_list_array[i].thumb_id);
            st_Progress_Bar_Signal(this_win->wi_progress_bar, bar_pos, (int8_t*)progess_bar_indication);

            const SavedImage& saved = gifFile->SavedImages[i];
            const GifImageDesc& desc = saved.ImageDesc;
            const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;  

            GraphicsControlBlock image_gcb;
            DGifSavedExtensionToGCB(gifFile, i, &image_gcb);
            int trans = image_gcb.TransparentColor;
            int disposal = image_gcb.DisposalMode;            
            // printf("id %d trans %d\n", i, trans);
            u_int8_t alpha = 0xFF;
            int colorCount = colorMap->ColorCount;

            for(int z = 0; z < saved.ExtensionBlockCount; z++)
            {
                ExtensionBlock * block = &saved.ExtensionBlocks[z];
                if(block->Function == 249)
                {
                    if(block->Bytes[0])
                        alpha = (u_int8_t)block->Bytes[3];
                }
            }

            int32_t ii, jj, x, y;
            u_int32_t* ptr_argb = (u_int32_t*)temp_buffer; 

            for(y = 0; y < desc.Height; y++){
                for(x = 0; x < desc.Width; x++){
                    ii = (x + desc.Left) + ((y + desc.Top) * MFDB_STRIDE(final_width));
                    int c = saved.RasterBits[ (y * desc.Width) + x];
                    if(c == alpha || c >= colorCount || c == gifFile->SBackGroundColor){
                        if(i == 0 || (c == gifFile->SBackGroundColor && c < colorCount && c >= 0 && trans == -1 && disposal != DISPOSE_BACKGROUND) ){                         
                            ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                        }else{
                            continue;
                        }
                    } else if (colorMap) {
                        ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                    }
                }
            }

            u_int16_t new_width = wanted_width;
            u_int16_t new_height = wanted_height;
            if (temp_mfdb->fd_w  > this_win->wi_thumb->thumb_w_size || temp_mfdb->fd_h > this_win->wi_thumb->thumb_h_size){
                float factor_h, factor_v;
                factor_h = temp_mfdb->fd_w  / (float)this_win->wi_thumb->thumb_w_size;
                factor_v = temp_mfdb->fd_h / (float)this_win->wi_thumb->thumb_h_size;

                if (factor_v > factor_h) {
                    new_height = this_win->wi_thumb->thumb_h_size;
                    new_width  = temp_mfdb->fd_w / factor_v;
                } else {
                    new_height = temp_mfdb->fd_h / factor_h;
                    new_width  = this_win->wi_thumb->thumb_w_size;
                }
            }

            u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(new_width, new_height, 32);
            MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, new_width, new_height, 32);

            st_Rescale_ARGB(temp_mfdb, thumb_original_mfdb, new_width, new_height);

            char thumb_txt[10] = {'\0'};
            sprintf(thumb_txt,"%d", this_win->wi_thumb->thumbs_list_array[i].thumb_index );
            print_ft_simple((thumb_original_mfdb->fd_w >> 1) - 4, thumb_original_mfdb->fd_h - 4, thumb_original_mfdb, (char*)TTF_DEFAULT_PATH, 14, thumb_txt);


            if(screen_workstation_bits_per_pixel != 32){
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = thumb_original_mfdb;
            }
            
            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            this_win->wi_thumb->thumbs_list_array[i].thumb_selected = FALSE;
        }
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        mfdb_free(temp_mfdb);
    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
    }

    if(file_process){DGifCloseFile(gifFile, &error);} 
}

void _st_Handle_Thumbs_GIF_Generic(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

	const char *file_name;
    GifFileType* gifFile;
    int error;
    u_int16_t idx = 0;

    if( file_process == TRUE ){
        file_name = this_win->wi_data->path;
        gifFile = DGifOpenFileName(file_name, &error);
        
        if (!gifFile) {
            sprintf(alert_message, "DGifOpenFileName() failed - %d", error);
            st_form_alert(FORM_STOP, alert_message);        
            // return false;
        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);        
            DGifCloseFile(gifFile, &error);
            // return false;
        }
    }

    this_win->wi_data->img.img_total = gifFile->ImageCount;
    this_win->wi_data->img.img_id = idx;
    this_win->wi_data->img.img_index = idx + 1;

    st_Thumb_List_Generic(this_win, "GIF Building images index", "GIF", 80, 20, 4, 4, TRUE);

    if(file_process){DGifCloseFile(gifFile, &error);} 
}
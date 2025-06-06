#include "img_gif.h"

#include "../utils/utils.h"
#include "../img_handler.h"

#include "../utils_gfx/pix_convert.h"
#include "../utils_gfx/ttf.h"

#include "../thumbs/thumbs.h"

#include "../rsc_processing/progress_bar.h"

#ifndef PRIMARY_IMAGE_ID
#define PRIMARY_IMAGE_ID    -1
#endif

void _st_Read_GIF(int16_t this_win_handle, boolean file_processing, long img_id);
void _st_Handle_Thumbs_GIF(int16_t this_win_handle, boolean file_process);
void _st_Handle_Thumbs_GIF_Generic(int16_t this_win_handle, boolean file_process);

void st_Win_Print_GIF(int16_t);


void st_Init_GIF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
    if(this_win->wi_data->video_media){
        this_win->wi_data->img.img_id = 0;
        this_win->wi_data->img.img_index = 1; 
    }
    this_win->refresh_win = st_Win_Print_GIF;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }    

    if(!this_win->wi_data->video_media){
        /* thumbnails stuff */
        if(this_win->wi_thumb == NULL){
            if(cpu_type < 40){
                _st_Handle_Thumbs_GIF_Generic(this_win->wi_handle, this_win->prefers_file_instead_mem);
            }else{
                _st_Handle_Thumbs_GIF(this_win->wi_handle, this_win->prefers_file_instead_mem);
            }
        }
    }
}

void st_Win_Print_GIF(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->video_media){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }
    if(!this_win->wi_data->video_media){
        // printf("Read Gif \n");
        _st_Read_GIF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);
    }
    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_GIF(int16_t this_win_handle, boolean file_processing, long img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    if(this_win->wi_data->stop_original_data_load == FALSE){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "GIF READING", 15,  "Starting...");

        const char *file_name = this_win->wi_data->path;
        long this_img = 0;
        int error;

        GifFileType* gifFile = DGifOpenFileName(file_name, &error);
        if (!gifFile) {
            sprintf(alert_message, "DGifOpenFileName() failed - %d", error);
            st_form_alert(FORM_STOP, alert_message);
            return;
        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);        
            DGifCloseFile(gifFile, &error);
            return;
        }
        if(img_id > (gifFile->ImageCount - 1)){
            printf("img_id out of range\n");
            return;
        }        
        if(img_id > PRIMARY_IMAGE_ID){
            this_img = img_id;
        }

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 30, "GIF: Parsing header");

        ColorMapObject* commonMap = gifFile->SColorMap;

        u_int16_t width = gifFile->SWidth;
        u_int16_t height = gifFile->SHeight;

        u_int8_t* temp_buffer = NULL;
        temp_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(temp_buffer == NULL){
            return;
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)temp_buffer, width, height, 32);
        st_MFDB_Fill(&this_win->wi_original_mfdb, 0XFFFFFFFF);

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 79, "GIF: Building ARGB pixels");

        long ii, jj, x, y;
        u_int32_t* ptr_argb = (u_int32_t*)temp_buffer;        
        int16_t img_current = 0;
        while( img_current <= this_img ){

            GraphicsControlBlock image_gcb;
            DGifSavedExtensionToGCB(gifFile, img_current, &image_gcb);
       
            const SavedImage& saved = gifFile->SavedImages[img_current];
            const GifImageDesc& desc = saved.ImageDesc;
            const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;

            int trans = image_gcb.TransparentColor ;
            int alpha; 
            u_int16_t delay = image_gcb.DelayTime;
            int disposal = image_gcb.DisposalMode;
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
                        if(this_win->wi_data->img.img_id == 0 || (c == gifFile->SBackGroundColor && c < colorCount && c >= 0 && trans == -1 && disposal != DISPOSE_BACKGROUND) ){                         
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

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

        DGifCloseFile(gifFile, &error);
        st_Win_Progress_Bar_Finish(this_win->wi_handle);
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
            return;
        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);        
            DGifCloseFile(gifFile, &error);
            return;
        }
    }

    this_win->wi_data->img.img_total = gifFile->ImageCount;
    this_win->wi_data->img.img_id = idx;
    this_win->wi_data->img.img_index = idx + 1;

    if(this_win->wi_data->img.img_total > 1){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "GIF Thumbs processing", 1,  "Starting...");

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

        this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));

        struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
        struct_st_thumbs_list* prev_thumb_ptr = NULL;

        this_win->wi_thumb->thumbs_open_new_win = FALSE;
        #ifndef WITH_FREETYPE
        this_win->wi_thumb->thumbs_use_gem_text = TRUE;
        #endif
        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;
        this_win->wi_thumb->thumbs_nb = this_win->wi_data->img.img_total;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
            if(thumb_ptr == NULL){
                thumb_ptr = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
            }
            thumb_ptr->thumb_id = i;
            thumb_ptr->thumb_index = i + 1;
            thumb_ptr->thumb_selectable = TRUE;
            thumb_ptr->thumb_visible = TRUE;
            thumb_ptr->next = NULL;
            thumb_ptr->prev = prev_thumb_ptr;
            if(thumb_ptr->prev != NULL){
                thumb_ptr->prev->next = thumb_ptr;
            }

            char progess_bar_indication[96];
            sprintf(progess_bar_indication, "GIF Thumbs processing: %d/%d", i, this_win->wi_thumb->thumbs_nb);

            st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), progess_bar_indication);

            const SavedImage& saved = gifFile->SavedImages[i];
            const GifImageDesc& desc = saved.ImageDesc;
            const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;  

            GraphicsControlBlock image_gcb;
            DGifSavedExtensionToGCB(gifFile, i, &image_gcb);
            int trans = image_gcb.TransparentColor;
            int disposal = image_gcb.DisposalMode;
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

            long ii, jj, x, y;
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
            char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
            strcpy(font_path, current_path);
            strcat(font_path, TTF_DEFAULT_PATH);
            sprintf(thumb_txt,"%d", thumb_ptr->thumb_index );

            if(this_win->wi_thumb->thumbs_use_gem_text){
                thumb_ptr->thumb_text = (char*)mem_alloc(sizeof(thumb_txt) + 1);
                sprintf(thumb_ptr->thumb_text,"%s", thumb_txt );
            }
            #ifdef WITH_FREETYPE
            else{
                print_TTF((thumb_original_mfdb->fd_w >> 1) - 4, thumb_original_mfdb->fd_h - 4, thumb_original_mfdb, font_path, 14, thumb_txt);
            }
            #endif
            
            if(screen_workstation_bits_per_pixel != 32){
                thumb_ptr->thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                thumb_ptr->thumb_mfdb = thumb_original_mfdb;
            }

            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            thumb_ptr->thumb_selected = FALSE;

            prev_thumb_ptr = thumb_ptr;
            thumb_ptr = NULL;
        }
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        st_Win_Progress_Bar_Finish(this_win->wi_handle);
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

    st_Thumb_List_Generic(this_win, "GIF Building images index", "GIF", 80, 20, 4, 4, FALSE);

    if(file_process){DGifCloseFile(gifFile, &error);} 
}

void *st_Win_Play_GIF_Video(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        sprintf(alert_message, "Error this_win is NULL");
        st_form_alert(FORM_STOP, alert_message);        
    }
        u_int32_t time_start, time_end, duration;

        const char *file_name = this_win->wi_data->path;
        int error;

        GifFileType* gifFile = DGifOpenFileName(file_name, &error);
        if (!gifFile) {
            sprintf(alert_message, "DGifOpenFileName() failed - %d", error);
            // printf("Error opening %s - Handle %d\n", file_name, this_win->wi_handle);
            st_form_alert(FORM_STOP, alert_message);
            return NULL;
        }
        if (DGifSlurp(gifFile) == GIF_ERROR) {
            sprintf(alert_message, "DGifSlurp() failed - %d", gifFile->Error);
            st_form_alert(FORM_STOP, alert_message);
            DGifCloseFile(gifFile, &error);
            return NULL;
        }

        this_win->wi_data->img.img_total = gifFile->ImageCount;

        ColorMapObject* commonMap = gifFile->SColorMap;

        u_int16_t width = gifFile->SWidth;
        u_int16_t height = gifFile->SHeight;

        u_int8_t* temp_buffer = NULL;
        temp_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(temp_buffer == NULL){
            return NULL;
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)temp_buffer, width, height, 32);
        st_MFDB_Fill(&this_win->wi_original_mfdb, 0XFFFFFFFF);

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

        this_win->refresh_win(this_win->wi_handle);

        long ii, jj, x, y;
        u_int32_t* ptr_argb = (u_int32_t*)temp_buffer;        

        while( (this_win->wi_data->img.img_id < this_win->wi_data->img.img_total) && this_win->wi_data->wi_pth != NULL){
            if(this_win->wi_data->play_on || this_win->wi_data->img.img_id == 0){
                time_start = clock();
                GraphicsControlBlock image_gcb;
                DGifSavedExtensionToGCB(gifFile, this_win->wi_data->img.img_id, &image_gcb);
        
                const SavedImage& saved = gifFile->SavedImages[this_win->wi_data->img.img_id];
                const GifImageDesc& desc = saved.ImageDesc;
                const ColorMapObject* colorMap = desc.ColorMap ? desc.ColorMap : commonMap;

                int trans = image_gcb.TransparentColor ;
                int alpha; 
                u_int16_t delay = image_gcb.DelayTime;
                int disposal = image_gcb.DisposalMode;
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
                            if(this_win->wi_data->img.img_id == 0 || (c == gifFile->SBackGroundColor && c < colorCount && c >= 0 && trans == -1 && disposal != DISPOSE_BACKGROUND) ){                         
                                ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                            }else{
                                continue;
                            }
                        } else if (colorMap) {
                            ptr_argb[ii] = alpha << 24 | colorMap->Colors[c].Red << 16 | colorMap->Colors[c].Green << 8 | colorMap->Colors[c].Blue;
                        }
                    }
                }

                time_end = clock();
                duration = 5 * (time_end - time_start);

                while( duration <  mul_10_fast(delay)){
                    duration = 5 * (clock() - time_start);
                }
                if(screen_workstation_bits_per_pixel != 32){
                    this_win->wi_data->wi_buffer_modified = FALSE;
                    this_win->wi_data->remap_displayed_mfdb = TRUE;
                    this_win->refresh_win(this_win->wi_handle);
                }
                st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);          
                send_message(this_win_handle, WM_REDRAW);
                this_win->wi_data->img.img_id++;
                this_win->wi_data->img.img_index = this_win->wi_data->img.img_id + 1;
                pthread_yield_np();
            }
            if(this_win->wi_data->img.img_id == (this_win->wi_data->img.img_total - 1)){
                this_win->wi_data->img.img_id = 0;
                this_win->wi_data->img.img_index = this_win->wi_data->img.img_id + 1;
            }
            pthread_yield_np();
        }

        DGifCloseFile(gifFile, &error);
        send_message(this_win_handle, WM_CLOSED);
        return NULL;
}
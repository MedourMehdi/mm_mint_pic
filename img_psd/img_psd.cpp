#include "img_psd.h"
#include "../img_handler.h"
#include "../utils_gfx/pix_convert.h"
#include "../thumbs/thumbs.h"
#include "../utils_gfx/ttf.h"
#include "../utils/utils.h"
#include <libpsd/libpsd.h>

#ifndef TTF_DEFAULT_PATH
#define TTF_DEFAULT_PATH "./fonts/arial.ttf"
#endif
#ifndef PRIMARY_IMAGE_ID
#define PRIMARY_IMAGE_ID    -1
#endif

void st_Win_Print_PSD(int16_t this_win_handle);
void _st_Read_PSD(int16_t this_win_handle, boolean file_process, long img_id);
void _st_Handle_Thumbs_PSD(int16_t this_win_handle, boolean file_process);

void st_Init_PSD(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_PSD;
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
    /* thumbnails stuff */
    if(this_win->wi_thumb == NULL){
        _st_Handle_Thumbs_PSD(this_win->wi_handle, this_win->prefers_file_instead_mem);
    }    
}

void st_Win_Print_PSD(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_PSD(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_PSD(int16_t this_win_handle, boolean file_process, long img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){

        psd_context *context = NULL;
        psd_status status;

		u_int16_t width = 320;
		u_int16_t height = 200;
        u_int8_t* destination_buffer;
        u_int32_t max_layer;

        status = psd_image_load(&context, (psd_char*)this_win->wi_data->path);
        if(status){
            sprintf(alert_message, "Can't load this PSD file");
            st_form_alert(FORM_STOP, alert_message);
        }

/*
	psd_uint					version;
	psd_bool					has_real_merged_data;
	psd_int						writer_name_length;
	psd_ushort *				writer_name;
	psd_int						reader_name_length;
	psd_ushort *				reader_name;
	psd_uint					file_version;
*/
printf("###\tVersion\t: %d\n", context->version_info.version);
printf("###\thas_real_merged_data\t: %d\n", context->version_info.has_real_merged_data);
printf("###\tfile_version\t: %d\n", context->version_info.file_version);

        if(img_id == PRIMARY_IMAGE_ID){
            img_id = 0;
            max_layer = context->layer_count;
            width = context->width;
            height = context->height;
            destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
            if(destination_buffer == NULL){
                sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
                st_form_alert(FORM_EXCLAM, alert_message);
            }
            if(this_win->wi_original_mfdb.fd_addr != NULL){
                mem_free(this_win->wi_original_mfdb.fd_addr);
            }
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
            if(status){
                goto end;
            }
            for (int i = img_id; i < max_layer; ++i) // iterate over all layers (if desired)
            {
                bool layer_visible = context->layer_records[i].visible;

                u_int32_t* imgdata = context->layer_records[i].image_data;
                u_int8_t* src_data = (u_int8_t*)imgdata;
                int lwidth = context->layer_records[i].width;
                int lheight = context->layer_records[i].height;
                int offset_x = context->layer_records[i].left;
                int offset_y = context->layer_records[i].top;
                long ii, jj, x, y;
                u_int32_t* ptr_argb = (u_int32_t*)destination_buffer;  
                for(y = 0; y < lheight; y++){
                    for(x = 0; x < lwidth; x++){
                        ii = (x + offset_x) + ((y + offset_y) * MFDB_STRIDE(width));
                        jj = (y * lwidth) + x;
                        ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
                    }
                }
            }
        }else{
            img_id = this_win->wi_data->img.img_id;
            width = context->layer_records[img_id].width;
            height = context->layer_records[img_id].height;
            destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
            if(destination_buffer == NULL){
                sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
                st_form_alert(FORM_EXCLAM, alert_message);
            }
            if(this_win->wi_original_mfdb.fd_addr != NULL){
                mem_free(this_win->wi_original_mfdb.fd_addr);
            }
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
            if(status){
                goto end;
            }

            u_int32_t* imgdata = context->layer_records[img_id].image_data;
            u_int8_t* src_data = (u_int8_t*)imgdata;

            long ii, jj, x, y;
            u_int32_t* ptr_argb = (u_int32_t*)destination_buffer;  
            for(y = 0; y < height; y++){
                for(x = 0; x < width; x++){
                    ii = x + (y * MFDB_STRIDE(width));
                    jj = (y * width) + x;
                    ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
                }
            }         
        }

        // printf("img_id %ld - max_layer %ld - this_win->wi_data->img.img_index %d, this_win->wi_data->img.img_id %d\n", img_id, max_layer, this_win->wi_data->img.img_index, this_win->wi_data->img.img_id);


        psd_image_free(context);
end:        
        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;
        this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
        this_win->total_length_h = this_win->wi_original_mfdb.fd_h;     
        this_win->wi_data->stop_original_data_load = TRUE;
        this_win->wi_data->wi_buffer_modified = FALSE;			
	}
}


void _st_Handle_Thumbs_PSD(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

    u_int16_t idx = 0;

    psd_context * context = NULL;
    psd_status status;

    status = psd_image_load(&context, (psd_char*)this_win->wi_data->path);
    if(status){
        sprintf(alert_message, "Can't load this PSD file");
        st_form_alert(FORM_STOP, alert_message);
        return;
    }

    this_win->wi_data->img.img_total = context->layer_count;
    this_win->wi_data->img.img_id = -1;
    // this_win->wi_data->img.img_index = idx + 1;
    if(this_win->wi_data->img.img_total > 1){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"Thumbs processing");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 1, (int8_t*)"Init");

        u_int16_t final_width = context->width;
        u_int16_t final_height = context->height;

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

        this_win->wi_thumb->thumbs_open_new_win = true;

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

            u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(final_width, final_height, 32);
            MFDB* temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, final_width, final_height, 32);

            u_int32_t* ptr_argb = (u_int32_t*)temp_buffer; 

            u_int32_t* imgdata = context->layer_records[i].image_data;
            int lwidth = context->layer_records[i].width;
            int lheight = context->layer_records[i].height;
            int offset_x = context->layer_records[i].left;
            int offset_y = context->layer_records[i].top;
            long ii, jj, x, y; 
            for(y = 0; y < lheight; y++){
                for(x = 0; x < lwidth; x++){
                    ii = (x + offset_x) + ((y + offset_y) * MFDB_STRIDE(final_width));
                    jj = (y * lwidth) + x;
                    ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
                }
            }

            // u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(lwidth, lheight, 32);
            // MFDB* temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, lwidth, lheight, 32);

            // u_int32_t* ptr_argb = (u_int32_t*)temp_buffer; 

            // long ii, jj, x, y; 
            // for(y = 0; y < lheight; y++){
            //     for(x = 0; x < lwidth; x++){
            //         ii = x + (y  * MFDB_STRIDE(lwidth));
            //         jj = (y * lwidth) + x;
            //         ptr_argb[ii] = st_Blend_Pix(ptr_argb[ii], imgdata[jj]);
            //     }
            // }

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

            char thumb_txt[128] = {'\0'};
            sprintf(thumb_txt,"%s", context->layer_records[i].layer_name );
            print_ft_simple(2, thumb_original_mfdb->fd_h - 2, thumb_original_mfdb, (char*)TTF_DEFAULT_PATH, 12, thumb_txt);


            if(screen_workstation_bits_per_pixel != 32){
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = thumb_original_mfdb;
            }
            
            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            this_win->wi_thumb->thumbs_list_array[i].thumb_selected = FALSE;
            mfdb_free(temp_mfdb);
        }
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        
    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
    }

    psd_image_free(context);
}

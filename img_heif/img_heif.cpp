#include "img_heif.h"

#include "../utils/utils.h"
// #include "../utils_gfx/pix_convert.h"
#include "../img_handler.h"

#include "../thumbs/thumbs.h"

#include "../rsc_processing/progress_bar.h"

#define PRIMARY_IMAGE_ID    -1

/* Local definitions */

void _st_Read_HEIF(int16_t, boolean file_processing, long img_id);
void _st_Handle_Thumbs_Heif(int16_t this_win_handle, boolean file_process);
void _st_Heif_RGBA_To_ARGB(uint8_t* source_buffer, uint8_t* destination_buffer, int16_t width, int16_t height);

void st_Win_Print_HEIF(int16_t);

void st_Init_HEIF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_HEIF;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }    
    /* thumbnails stuff */
    if(this_win->wi_thumb == NULL){
        _st_Handle_Thumbs_Heif(this_win->wi_handle, this_win->prefers_file_instead_mem);
    }
}

void st_Win_Print_HEIF(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_HEIF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);
    
    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }

}

void _st_Read_HEIF(int16_t this_win_handle, boolean file_process, long img_id) {

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

    int8_t *data_original = (int8_t *)this_win->wi_data->original_buffer;
    u_int32_t data_size = this_win->wi_data->STAT_FILE.st_size;

	int8_t *file_name;

    if(this_win->wi_data->stop_original_data_load != TRUE){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "HEIF READING", 10,  "HEIF: Starting...");

        #if LIBHEIF_HAVE_VERSION(1,13,0)
        heif_init(NULL);
        #endif

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 30, "HEIF: Getting context");        

        struct heif_context *ctx = heif_context_alloc();
        struct heif_image_handle *handle;

        if( file_process == TRUE ){
            file_name = (int8_t *)this_win->wi_data->path;
            heif_context_read_from_file(ctx, (const char*)file_name, NULL);
        } else {
            heif_context_read_from_memory_without_copy(ctx, data_original, data_size, NULL);
        }

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 30, "HEIF: Getting image handle");

        if(img_id == PRIMARY_IMAGE_ID){
            // Get a handle to the primary image
            heif_context_get_primary_image_handle(ctx, &handle);
        } else {
            heif_context_get_image_handle (ctx, img_id, &handle);
        } 

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 40, "HEIF: Getting originals w & h");

        int16_t width = heif_image_handle_get_width(handle);
        int16_t height = heif_image_handle_get_height(handle);
        int16_t nb_components_32bits = 4;

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 50, "HEIF: Decode Image");

        // Decode the image and convert colorspace to RGB, saved as 32bit interleaved
        struct heif_image* img;

        heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, NULL);

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 60, "HEIF: Getting bits per pixel");

        this_win->wi_data->img.bit_per_pixel = heif_image_get_bits_per_pixel( img, heif_channel_interleaved);

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 70, "HEIF: Computing raw data");

        int stride;
        const u_int8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 80, "HEIF: Computing RGBA to ARGB");

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }

        uint8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", (width * height) << 2);
            st_form_alert(FORM_EXCLAM, alert_message);
        }else{
            _st_Heif_RGBA_To_ARGB((u_int8_t *)data, destination_buffer, width, height);
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, 32);
        }
        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

        st_Win_Progress_Bar_Finish(this_win->wi_handle);

        heif_image_release(img);
        heif_context_free(ctx);
        heif_image_handle_release(handle);

        #if LIBHEIF_HAVE_VERSION(1,13,0)
        heif_deinit();
        #endif
        
        mem_free(img);
        mem_free((void*)data);
    }
}

void _st_Handle_Thumbs_Heif(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

    int8_t *data_original = (int8_t *)this_win->wi_data->original_buffer;
    u_int32_t data_size = this_win->wi_data->STAT_FILE.st_size;

	int8_t *file_name;
#if LIBHEIF_HAVE_VERSION(1,13,0)
    heif_init(NULL);
#endif
    struct heif_context *ctx = heif_context_alloc();
    struct heif_image_handle *handle, *thumbnail_handle;
    struct heif_image* img;

    if( file_process == TRUE ){
        file_name = (int8_t *)this_win->wi_data->path;
        heif_context_read_from_file(ctx, (const char*)file_name, NULL);
    } else {
        heif_context_read_from_memory_without_copy(ctx, data_original, data_size, NULL);
    }

    this_win->wi_data->img.img_total = heif_context_get_number_of_top_level_images(ctx);

    if(this_win->wi_data->img.img_total > 1){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "HEIF THUMBS", 1,  "Starting...");

        int16_t nb_components_32bits = 4;

        heif_item_id id_array[this_win->wi_data->img.img_total];
        heif_context_get_list_of_top_level_image_IDs(ctx, &id_array[0], this_win->wi_data->img.img_total);

        uint32_t itemId = id_array[0];
        heif_context_get_image_handle(ctx, itemId, &handle);
        this_win->wi_data->img.img_id = id_array[0];
        this_win->wi_data->img.img_index = 1;
        this_win->wi_data->thumbnail_slave = true;

        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, 4, 8, 110, 80);

        this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
        struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
        struct_st_thumbs_list* prev_thumb_ptr = NULL;

        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
            heif_item_id thumb_ids[1];
            int16_t n_thumbnails = heif_image_handle_get_number_of_thumbnails(handle );
            int16_t n_items = heif_image_handle_get_list_of_thumbnail_IDs(handle, thumb_ids, 1 );

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

            if (n_thumbnails > 1){
                heif_image_handle_get_thumbnail(handle, thumb_ids[0], &thumbnail_handle);
                thumb_ptr->thumb_id = thumb_ids[0];
            } else {
                heif_context_get_image_handle(ctx, id_array[i], &thumbnail_handle);
                thumb_ptr->thumb_id = id_array[i];
            }
            thumb_ptr->thumb_index = i + 1;

            char progess_bar_indication[96];
            sprintf(progess_bar_indication, "Thumbnail #%d/%d - Image id.%d", i+1, this_win->wi_thumb->thumbs_nb, thumb_ptr->thumb_id);

            st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), progess_bar_indication);

            heif_decode_image(thumbnail_handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, NULL);

            int16_t old_width  = heif_image_handle_get_width(thumbnail_handle);
            int16_t old_height = heif_image_handle_get_height(thumbnail_handle);
            int16_t new_width = old_width;
            int16_t new_height = old_height;

            if (old_width  > this_win->wi_thumb->thumb_w_size || old_height > this_win->wi_thumb->thumb_h_size){
                float factor_h, factor_v;
                factor_h = old_width  / (float)this_win->wi_thumb->thumb_w_size;
                factor_v = old_height / (float)this_win->wi_thumb->thumb_h_size;

                if (factor_v > factor_h) {
                    new_height = this_win->wi_thumb->thumb_h_size;
                    new_width  = old_width / factor_v;
                } else {
                    new_height = old_height / factor_h;
                    new_width  = this_win->wi_thumb->thumb_w_size;
                }
            }
            heif_image *scaled_img = NULL;
            heif_image_scale_image(img, &scaled_img, new_width, new_height, NULL);

            uint8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(new_width, new_height, nb_components_32bits << 3);

            int stride;
            const u_int8_t* data = heif_image_get_plane_readonly(scaled_img, heif_channel_interleaved, &stride);

            _st_Heif_RGBA_To_ARGB((u_int8_t *)data, destination_buffer, new_width, new_height);

            MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, new_width, new_height, nb_components_32bits << 3);

            if(screen_workstation_bits_per_pixel != 32){
                thumb_ptr->thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                thumb_ptr->thumb_mfdb = thumb_original_mfdb;
            }

            thumb_ptr->thumb_visible = true;
            thumb_ptr->thumb_selectable = true;
            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            thumb_ptr->thumb_selected = FALSE;

            prev_thumb_ptr = thumb_ptr;
            thumb_ptr = NULL;
            heif_image_release(img);
            heif_image_handle_release(thumbnail_handle);
        }

        st_Win_Progress_Bar_Finish(this_win->wi_handle);

        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        heif_image_handle_release(handle);
    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
        this_win->wi_data->img.img_index = 1;
    }

    heif_context_free(ctx);
#if LIBHEIF_HAVE_VERSION(1,13,0)    
    heif_deinit();
#endif    
}

void _st_Heif_RGBA_To_ARGB(uint8_t* source_buffer, uint8_t* destination_buffer, int16_t width, int16_t height){
    u_int32_t x, y, i;
    u_int32_t *dst_ptr, *src_ptr;
    src_ptr = (u_int32_t*)source_buffer;
    dst_ptr = (u_int32_t*)destination_buffer;
    for(int16_t y = 0; y < height; y++ ){
        for(int16_t x = 0; x < width; x++){
            i = (y * MFDB_STRIDE(width)) + x;
            dst_ptr[i] = ((src_ptr[i] & 0x000000FF) << 24 ) | ((src_ptr[i] & 0xFFFFFF00) >> 8);
        }
    }
}

void st_Write_HEIF(uint8_t* src_buffer, int width, int height, const char* filename) {

#if LIBHEIF_HAVE_VERSION(1,13,0)
    heif_init(NULL);
#endif
    int16_t thumbnail_bbox_size = 0;

    heif_context* ctx = heif_context_alloc();
    heif_image_handle *handle = NULL;
    heif_encoder* encoder = NULL;
    heif_error err;

    uint8_t* data;
    int stride;

    struct_win_progress_bar* this_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(NIL, "HEIF WRITING", 10,  "HEIF image encoding");

    // heif_encoder_set_lossy_quality(encoder, 50);

    heif_image* image;
    err = heif_image_create(width, height,
                            heif_colorspace_RGB,
                            heif_chroma_interleaved_RGB,
                            &image);
    if(err.code != 0){
        sprintf(alert_message, "heif_image_create\n%d - %s", err.code, err.message);
        goto clean;
    }
    st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 30, "HEIF: Adding image plane");    
    err = heif_image_add_plane(image, heif_channel_interleaved, width, height, 8);
    if(err.code != 0){
        sprintf(alert_message, "heif_image_add_plane\n%d - %s", err.code, err.message);
        goto clean;
    }
    st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 50, "HEIF: Filling image plane");
    data = heif_image_get_plane(image, heif_channel_interleaved, &stride);
    for ( int y = 0; y < height; y++ ){
        int k = y * stride;
        for ( int x = 0; x < width; x++ ){
            // int ind1 = (x * 3) + k;
            int ind1 = ((x << 1) + x) + k;
            int ind2 = (((y * MFDB_STRIDE(width)) + x) << 2);
            *(data + ind1    ) = *(src_buffer + ind2 + 1);    // R
            *(data + ind1 + 1) = *(src_buffer + ind2 + 2);    // G
            *(data + ind1 + 2) = *(src_buffer + ind2 + 3);    // B
            // *(data + ind1 + 3) = *(src_buffer + ind2    );    // A
        }
    }
    
    err = heif_context_get_encoder_for_format(ctx, heif_compression_HEVC, &encoder);
    if(err.code != 0){
        sprintf(alert_message, "heif_context_get_encoder_for_format\n%d - %s", err.code, err.message);
        goto clean;
    }
    st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 70, "HEIF: Image encoding");
    err = heif_context_encode_image(ctx, image, encoder, NULL, &handle);
    if(err.code != 0){
        sprintf(alert_message, "heif_context_encode_image\n%d - %s", err.code, err.message);
        goto clean;
    }

    thumbnail_bbox_size = 320;

    if (thumbnail_bbox_size > 0 && thumbnail_bbox_size < width && thumbnail_bbox_size < height) {
        struct heif_image_handle* thumbnail_handle;
        st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 80, "HEIF: Thumbnail encoding");
        err = heif_context_encode_thumbnail(ctx, image, handle, encoder, NULL, thumbnail_bbox_size, &thumbnail_handle);
        if(err.code != 0){
            sprintf(alert_message, "heif_context_encode_thumbnail\n%d - %s", err.code, err.message);
            goto clean;
        }
        if (thumbnail_handle) {
            heif_image_handle_release(thumbnail_handle);
        }
    }

clean:
    heif_image_handle_release (handle);
    heif_encoder_release(encoder);
    heif_context_write_to_file(ctx, filename);
    heif_context_free(ctx);
#if LIBHEIF_HAVE_VERSION(1,13,0)    
    heif_deinit();
#endif
    st_Win_Progress_Bar_Finish(this_progress_bar->win_form_handle);
    if(err.code != 0){
        st_form_alert(FORM_STOP, alert_message);
    }

}
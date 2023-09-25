#include "img_heif.h"

#include "../utils/utils.h"
// #include "../utils_gfx/pix_convert.h"
#include "../img_handler.h"

#include "../utils_rsc/progress.h"

#include "../thumbs/thumbs.h"

#define PRIMARY_IMAGE_ID    -1

/* Local definitions */

void _st_Read_HEIF(int16_t, boolean file_processing, u_int32_t img_id);
void _st_Handle_Thumbs_Heif(int16_t this_win_handle, boolean file_process);
void _st_Heif_RGBA_To_ARGB(uint8_t* source_buffer, uint8_t* destination_buffer, int16_t width, int16_t height, int16_t stride, u_int32_t background_color);

void st_Win_Print_HEIF(int16_t);

void st_Init_HEIF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
	this_win->refresh_win = st_Win_Print_HEIF;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    /* Progress Bar Stuff */
this_win->wi_progress_bar = global_progress_bar;
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

    if(this_win->wi_data->needs_refresh == TRUE){
        this_win->wi_data->wi_original_modified = FALSE;
        this_win->wi_data->needs_refresh = FALSE;
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    } 

    _st_Read_HEIF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);
    
    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }

}

void _st_Read_HEIF(int16_t this_win_handle, boolean file_process, u_int32_t img_id) {

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

    int8_t *data_original = (int8_t *)this_win->wi_data->original_buffer;
    u_int32_t data_size = this_win->wi_data->STAT_FILE.st_size;

	int8_t *file_name;

    if(this_win->wi_data->wi_original_modified != TRUE){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"HEIF READING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");
#if LIBHEIF_HAVE_VERSION(1,13,0)
        heif_init(NULL);
#endif
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 25, (int8_t*)"Context read");

        struct heif_context *ctx = heif_context_alloc();
        struct heif_image_handle *handle;

        if( file_process == TRUE ){
            file_name = (int8_t *)this_win->wi_data->path;
            heif_context_read_from_file(ctx, (const char*)file_name, NULL);
        } else {
            heif_context_read_from_memory_without_copy(ctx, data_original, data_size, NULL);
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 35, (int8_t*)"Getting image handle");

        if(img_id == PRIMARY_IMAGE_ID){
            // Get a handle to the primary image
            heif_context_get_primary_image_handle(ctx, &handle);
        } else {
            heif_context_get_image_handle (ctx, img_id, &handle);
        } 

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 45, (int8_t*)"Getting originals width & height");

        int16_t width = heif_image_handle_get_width(handle);
        int16_t height = heif_image_handle_get_height(handle);
        int16_t nb_components_32bits = 4;

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 55, (int8_t*)"HEIF Decode Image");

        // Decode the image and convert colorspace to RGB, saved as 32bit interleaved
        struct heif_image* img;

        heif_decode_image(handle, &img, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, NULL);

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 65, (int8_t*)"Getting bits per pixel");

        this_win->wi_data->img.bit_per_pixel = heif_image_get_bits_per_pixel( img, heif_channel_interleaved);

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 75, (int8_t*)"Accessing raw data");

        int stride;
        const u_int8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 85, (int8_t*)"Computing RGBA to ARGB");

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }

        uint8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_EXCLAM, alert_message);
        } else {
            _st_Heif_RGBA_To_ARGB((u_int8_t *)data, destination_buffer, width, height, stride, 0xFFFFFFFF);
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, nb_components_32bits << 3);
            this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
            this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
            this_win->wi_data->wi_original_modified = TRUE;
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);

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

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"THUMBS PROCESSING");

        int16_t nb_components_32bits = 4;

        heif_item_id id_array[this_win->wi_data->img.img_total];
        heif_context_get_list_of_top_level_image_IDs(ctx, &id_array[0], this_win->wi_data->img.img_total);

        uint32_t itemId = id_array[0];
        heif_context_get_image_handle(ctx, itemId, &handle);
        this_win->wi_data->img.img_id = id_array[0];
        this_win->wi_data->img.img_index = 1;
        this_win->wi_data->thumbnail_slave = true;

        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, 4, 8, 110, 80);

        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
            heif_item_id thumb_ids[1];
            int16_t n_thumbnails = heif_image_handle_get_number_of_thumbnails(handle );
            int16_t n_items = heif_image_handle_get_list_of_thumbnail_IDs(handle, thumb_ids, 1 );

            if (n_thumbnails > 1){
                heif_image_handle_get_thumbnail(handle, thumb_ids[0], &thumbnail_handle);
                this_win->wi_thumb->thumbs_list_array[i].thumb_id = thumb_ids[0];
            } else {
                heif_context_get_image_handle(ctx, id_array[i], &thumbnail_handle);
                this_win->wi_thumb->thumbs_list_array[i].thumb_id = id_array[i];
            }
            this_win->wi_thumb->thumbs_list_array[i].thumb_index = i + 1;

            char progess_bar_indication[96];
            sprintf(progess_bar_indication, "Thumbnail #%d/%d - Image id.%d", i+1, this_win->wi_thumb->thumbs_nb, this_win->wi_thumb->thumbs_list_array[i].thumb_id);
            st_Progress_Bar_Signal(this_win->wi_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), (int8_t*)progess_bar_indication);

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

            _st_Heif_RGBA_To_ARGB((u_int8_t *)data, destination_buffer, new_width, new_height, stride, this_win->wi_thumb->thumb_background_color);

            MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, new_width, new_height, nb_components_32bits << 3);

            if(screen_workstation_bits_per_pixel != 32){
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb = thumb_original_mfdb;
            }

            this_win->wi_thumb->thumbs_list_array[i].thumb_mfdb_stride = MFDB_STRIDE(new_width) - new_width;  

            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            this_win->wi_thumb->thumbs_list_array[i].thumb_selected = FALSE;

            heif_image_release(img);
            heif_image_handle_release(thumbnail_handle);
        }
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
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

void _st_Heif_RGBA_To_ARGB(uint8_t* source_buffer, uint8_t* destination_buffer, int16_t width, int16_t height, int16_t stride, u_int32_t background_color){
    u_int32_t x, y, i, j, k, l, m, n, o;
    int16_t width_stride = MFDB_STRIDE(width) - width;
    int16_t nb_components_32bits = 4;

    uint8_t a, r, g, b, *ptr_color;
    ptr_color = (uint8_t*)&background_color;
    a = ptr_color[0]; r = ptr_color[1]; g = ptr_color[2]; b = ptr_color[3];

        for (y = 0; y < height; y++) {
            m = (width * y);
            l = (MFDB_STRIDE(width) * y);
            o = (y * stride);
            for (x = 0; x < width; x++) {
                n = (l + x) << 2;
                j = o + (x << 2);
                destination_buffer[n] = source_buffer[j + 3];
                destination_buffer[n + 1] = source_buffer[j];
                destination_buffer[n + 2] = source_buffer[j + 1];
                destination_buffer[n + 3] = source_buffer[j + 2];
                i = ((l + x) + 1) << 2;
            }
            for(k = width_stride; k > 0; k--){
                destination_buffer[i++] = a;
                destination_buffer[i++] = r;
                destination_buffer[i++] = g;
                destination_buffer[i++] = b;
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
    /* Progress Bar Stuff */
    struct_progress_bar* wi_progress_bar = (struct_progress_bar*)mem_alloc(sizeof(struct_progress_bar));
    wi_progress_bar->progress_bar_enabled = TRUE;
    wi_progress_bar->progress_bar_in_use = FALSE;
    wi_progress_bar->progress_bar_locked = FALSE;
    st_Progress_Bar_Add_Step(wi_progress_bar);
    st_Progress_Bar_Init(wi_progress_bar, (int8_t*)"HEIF WRITING");
    st_Progress_Bar_Signal(wi_progress_bar, 15, (int8_t*)"Heif image create");

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
    st_Progress_Bar_Signal(wi_progress_bar, 35, (int8_t*)"Image: adding plane");
    err = heif_image_add_plane(image, heif_channel_interleaved, width, height, 8);
    if(err.code != 0){
        sprintf(alert_message, "heif_image_add_plane\n%d - %s", err.code, err.message);
        goto clean;
    }
    st_Progress_Bar_Signal(wi_progress_bar, 55, (int8_t*)"Image: filling plane");
    
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
    st_Progress_Bar_Signal(wi_progress_bar, 75, (int8_t*)"Image encoding");    
    err = heif_context_encode_image(ctx, image, encoder, NULL, &handle);
    if(err.code != 0){
        sprintf(alert_message, "heif_context_encode_image\n%d - %s", err.code, err.message);
        goto clean;
    }

    thumbnail_bbox_size = 320;

    if (thumbnail_bbox_size > 0 && thumbnail_bbox_size < width && thumbnail_bbox_size < height) {
        struct heif_image_handle* thumbnail_handle;
        st_Progress_Bar_Signal(wi_progress_bar, 75, (int8_t*)"Thumbnail encoding");   
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
    st_Progress_Bar_Signal(wi_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(wi_progress_bar);
    st_Progress_Bar_Finish(wi_progress_bar);
    mem_free(wi_progress_bar);
    if(err.code != 0){
        st_form_alert(FORM_STOP, alert_message);
    }

}
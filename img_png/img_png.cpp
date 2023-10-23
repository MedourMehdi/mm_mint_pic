#include "img_png.h"
#include "../img_handler.h"

#include "../utils/utils.h"

#define PNG_BACKGROUND_COLOR	0xFFFFFF
#ifndef ALPHA_COMPOSITE
#define ALPHA_COMPOSITE( composite, fg, alpha, bg) {									\
    u_int16_t temp = (( u_int16_t)( fg) * ( u_int16_t)( alpha) +						\
                   ( u_int16_t)( bg) * ( u_int16_t)(255 - ( u_int16_t)( alpha)) + ( u_int16_t)128);	\
    ( composite) = ( u_int8_t)(( temp + ( temp >> 8)) >> 8);								\
}
#endif

void st_Win_Print_PNG(int16_t this_win_handle);
void _st_Read_PNG(int16_t this_win_handle, boolean file_process);
void _st_Read_PNG_Callback(png_structp _pngptr, png_bytep _data, png_size_t _len);

void st_Init_PNG(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_PNG;

    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_PNG(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_PNG(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_PNG(int16_t my_win_handle, boolean file_process) {
	struct_window *this_win;
	this_win = detect_window(my_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"PNG READING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");

        int x, y;

        int width, height, channels;
        png_byte color_type;
        png_byte bit_depth;

        png_structp png_ptr;
        png_infop info_ptr;
        png_bytep * row_pointers;

        u_int8_t header[BYTES_TO_CHECK];
        const char *file_name;
        FILE *fp;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 25, (int8_t*)"Init PNG parameters");
        if( file_process == TRUE ){
            file_name = this_win->wi_data->path;
            fp = fopen(file_name, "rb");
            if (!fp){ 
                form_alert(1, "[1][File could not be opened for reading][Okay]"); 
                st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
                st_Progress_Bar_Finish(this_win->wi_progress_bar);
                return;
            }
            fread(header, 1, 8, fp);
        } else {
            memcpy(header, this_win->wi_data->original_buffer, BYTES_TO_CHECK);
        }

        if (png_sig_cmp((png_const_bytep)header, 0, BYTES_TO_CHECK)){
        // if (png_sig_cmp((png_const_charp)header, 0, BYTES_TO_CHECK)){            
            form_alert(1, "[1][This file is not recognized as a PNG file][Okay]");
            st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
            st_Progress_Bar_Finish(this_win->wi_progress_bar);
            return;
        }

        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png_ptr){
            form_alert(1, "[1][png_create_read_struct failed][Okay]");
            st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
            st_Progress_Bar_Finish(this_win->wi_progress_bar);
            return;
        }
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr){
            form_alert(1, "[1][png_create_info_struct failed][Okay]");
            st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
            st_Progress_Bar_Finish(this_win->wi_progress_bar);
            return;
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 35, (int8_t*)"Getting data");
        if( file_process == TRUE ){
            png_init_io(png_ptr, fp);
        } else {
            png_set_read_fn(png_ptr, &this_win->wi_data->original_buffer[BYTES_TO_CHECK], _st_Read_PNG_Callback);
        }
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 45, (int8_t*)"Parsing data parameters");
        png_set_sig_bytes(png_ptr, BYTES_TO_CHECK);
        png_read_info(png_ptr, info_ptr);

        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);

        /*
        ///  brief describes which color/alpha channels are present.
        ///  PNG_COLOR_TYPE_GRAY        (bit depths 1, 2, 4, 8, 16)
        ///  PNG_COLOR_TYPE_GRAY_ALPHA  (bit depths 8, 16)
        ///  PNG_COLOR_TYPE_PALETTE     (bit depths 1, 2, 4, 8)
        ///  PNG_COLOR_TYPE_RGB         (bit_depths 8, 16)
        ///  PNG_COLOR_TYPE_RGB_ALPHA   (bit_depths 8, 16)
        ///  PNG_COLOR_MASK_PALETTE
        ///  PNG_COLOR_MASK_COLOR
        ///  PNG_COLOR_MASK_ALPHA
        */

        if (bit_depth == 16) {
            png_set_strip_16(png_ptr);
        }
        if (color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_palette_to_rgb(png_ptr);
        }
        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
            png_set_expand_gray_1_2_4_to_8(png_ptr);
        }
        if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
                png_set_tRNS_to_alpha(png_ptr);
        // These color_type don't have an alpha channel then fill it with 0xff.
        if(color_type == PNG_COLOR_TYPE_RGB ||
        color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_PALETTE) {
            png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
        }
        if(color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
            png_set_gray_to_rgb(png_ptr);
        }

        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0){
            png_set_tRNS_to_alpha(png_ptr);
        }

        png_read_update_info(png_ptr, info_ptr);

        width = png_get_image_width(png_ptr, info_ptr);
        height = png_get_image_height(png_ptr, info_ptr);
        color_type = png_get_color_type(png_ptr, info_ptr);
        bit_depth = png_get_bit_depth(png_ptr, info_ptr);
        channels = png_get_channels( png_ptr, info_ptr);

        row_pointers = (png_bytep *)mem_alloc(sizeof(png_bytep) * height);
        for (y=0; y<height; y++)
                    row_pointers[y] = (png_byte *)mem_alloc(png_get_rowbytes(png_ptr,info_ptr));

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 65, (int8_t*)"Unpack PNG data");
        png_read_image(png_ptr, row_pointers);

        u_int32_t i, j, k;
        u_int8_t a, r, g, b, red, green, blue;
        int16_t nb_components_original = channels;
        int16_t nb_components_32bits = 4;

        u_int8_t *destination_buffer = st_ScreenBuffer_Alloc_bpp(width,height, nb_components_32bits << 3);

        this_win->wi_data->img.bit_per_pixel = channels << 3;

        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }

        int16_t width_stride = mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, nb_components_32bits << 3);
        st_MFDB_Fill_bpp(&this_win->wi_original_mfdb, 0x00FFFFFF, 32);
        this_win->total_length_w = width;
        this_win->total_length_h = height;
        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;        
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 75, (int8_t*)"Building ARGB bitmap");
        for (y = (height - 1); y != -1; y--) {
            for (x = 0; x < width; x++) {
                i = (x + y * MFDB_STRIDE(width)) * nb_components_32bits;
                r = row_pointers[y][x * nb_components_original];
                g = row_pointers[y][(x * nb_components_original) + 1];
                b = row_pointers[y][(x * nb_components_original) + 2];
                a = row_pointers[y][(x * nb_components_original) + 3];
                if( a == 255) {
                    red   = r;
                    green = g;
                    blue  = b;
                }
                else if ( a == 0) {
                    red   = 0xFF;
                    green = 0xFF;
                    blue  = 0xFF;
                }
                else {
                    ALPHA_COMPOSITE( red,   r, a, ( PNG_BACKGROUND_COLOR >> 16) & 0xFF);
                    ALPHA_COMPOSITE( green, g, a, ( PNG_BACKGROUND_COLOR >> 8) & 0xFF);
                    ALPHA_COMPOSITE( blue,  b, a, ( PNG_BACKGROUND_COLOR) & 0xFF);
                }
                destination_buffer[i++] = a;
                destination_buffer[i++] = red;
                destination_buffer[i++] = green;
                destination_buffer[i++] = blue;
            }
            for(k = width_stride; k > 0; k--){
                destination_buffer[i++] = 0xFF;
                destination_buffer[i++] = 0xFF;
                destination_buffer[i++] = 0xFF;
                destination_buffer[i++] = 0xFF;
            }
        }
        
        png_read_end(png_ptr, info_ptr);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

        if( file_process == TRUE ){
            fclose(fp);
        }

        for (y=0; y<height; y++){
            mem_free(row_pointers[y]);
        }
        mem_free(row_pointers);
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        this_win->wi_data->stop_original_data_load = TRUE;
        this_win->wi_data->wi_buffer_modified = FALSE;
    }
}

int16_t st_Save_PNG(const char* filename, int width, int height, int bitdepth, int colortype, unsigned char* data, int pitch, int transform) {

    int16_t i = 0;
    int16_t r = 0;
    FILE* fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep* row_pointers = NULL;
    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"PNG WRITING");
    st_Progress_Bar_Signal(global_progress_bar, 10, (int8_t*)"PNG image encoding");
    if (NULL == data) {
        sprintf(alert_message, "Error: failed to save the png because the given data is NULL.");
        r = -1;
        goto error;
    }
    if (0 == strlen(filename)) {
        sprintf(alert_message, "Error: failed to save the png because the given filename length is 0.");
        r = -2;
        goto error;
    }

    if (0 == pitch) {
        sprintf(alert_message, "Error: failed to save the png because the given pitch is 0.");
        r = -3;
        goto error;
    }

    fp = fopen(filename, "wb");
    if (NULL == fp) {
        sprintf(alert_message, "Error: failed to open the png file: %s", filename);
        r = -4;
        goto error;
    }
    st_Progress_Bar_Signal(global_progress_bar, 30, (int8_t*)"PNG: Create Write Structure");
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (NULL == png_ptr) {
        sprintf(alert_message, "Error: failed to create the png write struct.");
        r = -5;
        goto error;
    }
    st_Progress_Bar_Signal(global_progress_bar, 40, (int8_t*)"PNG: Create Info Structure");
    info_ptr = png_create_info_struct(png_ptr);
    if (NULL == info_ptr) {
        sprintf(alert_message, "Error: failed to create the png info struct.");
        r = -6;
        goto error;
    }

  /* Set the image information here.  Width and height are up to 2^31,
  * bit_depth is one of 1, 2, 4, 8, or 16, but valid values also depend on
  * the color_type selected. color_type is one of PNG_COLOR_TYPE_GRAY,
  * PNG_COLOR_TYPE_GRAY_ALPHA, PNG_COLOR_TYPE_PALETTE, PNG_COLOR_TYPE_RGB,
  * or PNG_COLOR_TYPE_RGB_ALPHA.  interlace is either PNG_INTERLACE_NONE or
  * PNG_INTERLACE_ADAM7, and the compression_type and filter_type MUST
  * currently be PNG_COMPRESSION_TYPE_BASE and PNG_FILTER_TYPE_BASE. REQUIRED
  */
    st_Progress_Bar_Signal(global_progress_bar, 50, (int8_t*)"PNG: Set IHDR");
    png_set_IHDR(png_ptr,
                info_ptr,
                width,
                height,
                bitdepth,                 /* e.g. 8 */
                colortype,                /* PNG_COLOR_TYPE_{GRAY, PALETTE, RGB, RGB_ALPHA, GRAY_ALPHA, RGBA, GA} */
                PNG_INTERLACE_NONE,       /* PNG_INTERLACE_{NONE, ADAM7 } */
                PNG_COMPRESSION_TYPE_BASE,
                PNG_FILTER_TYPE_BASE);
    row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for (i = 0; i < height; ++i) {
        row_pointers[i] = data + i * pitch;
    }
    st_Progress_Bar_Signal(global_progress_bar, 60, (int8_t*)"PNG: Init I/O");
    png_init_io(png_ptr, fp);
    st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"PNG: Set Rows");
    png_set_rows(png_ptr, info_ptr, row_pointers);
    st_Progress_Bar_Signal(global_progress_bar, 90, (int8_t*)"PNG: Write Data");
    png_write_png(png_ptr, info_ptr, transform, NULL);


error:

    if (NULL != fp) {
        fclose(fp);
        fp = NULL;
    }

    if (NULL != png_ptr) {

    if (NULL == info_ptr) {
        sprintf(alert_message, "Error: info ptr is null. not supposed to happen here.\n");
    }

    png_destroy_write_struct(&png_ptr, &info_ptr);
    png_ptr = NULL;
    info_ptr = NULL;
    }

    if (NULL != row_pointers) {
        free(row_pointers);
        row_pointers = NULL;
    }

    if(r > 0){
        st_form_alert(FORM_STOP, alert_message);
    }

    st_Progress_Bar_Signal(global_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar); 

    return r;
 }

void _st_Read_PNG_Callback(png_structp _pngptr, png_bytep _data, png_size_t _len){
    static int offset = 0;
    u_int8_t *input;

    /* Get input */
    input = (u_int8_t *)png_get_io_ptr(_pngptr);

    /* Copy data from input */
    memcpy(_data, input + offset, _len);
    offset += _len;
}
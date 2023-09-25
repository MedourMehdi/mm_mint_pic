#include "img_jpeg.h"

#include <jpeglib.h>

#include "../utils/utils.h"
#include "../img_handler.h"

void st_Win_Print_JPEG(int16_t this_win_handle);
void _st_Read_JPEG(int16_t this_win_handle, boolean file_process);

void st_Init_JPEG(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_JPEG;
    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }    
}

void st_Win_Print_JPEG(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_JPEG(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void st_Write_JPEG(u_int8_t* src_buffer, int width, int height, const char* filename)
{

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"JPEG WRITING");
    st_Progress_Bar_Signal(global_progress_bar, 10, (int8_t*)"JPEG image encoding");

    J_COLOR_SPACE color_space = JCS_RGB;
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;

    /* this is a pointer to one row of image data */
    JSAMPROW row_pointer[1];
    FILE *outfile = fopen( filename, "wb" );

    if ( !outfile )
    {
        sprintf(alert_message, "Error opening output jpeg file %s\n!", filename );
        st_form_alert(FORM_STOP, alert_message);
        return;
    }

    st_Progress_Bar_Signal(global_progress_bar, 40, (int8_t*)"JPEG compression");
    cinfo.err = jpeg_std_error( &jerr );
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    /* Setting the parameters of the output file here */
    cinfo.image_width = width;  
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = color_space;
    /* default compression parameters, we shouldn't be worried about these */
    jpeg_set_defaults( &cinfo );
    /* Now do the compression .. */
    jpeg_start_compress( &cinfo, TRUE );
    /* like reading a file, this time write one row at a time */
    st_Progress_Bar_Signal(global_progress_bar, 70, (int8_t*)"Writing scanlines");
    while( cinfo.next_scanline < cinfo.image_height )
    {
        row_pointer[0] = &src_buffer[ cinfo.next_scanline * MFDB_STRIDE(cinfo.image_width) *  cinfo.input_components];
        jpeg_write_scanlines( &cinfo, row_pointer, 1 );
    }

    /* similar to read file, clean up after we're done compressing */
    jpeg_finish_compress( &cinfo );
    jpeg_destroy_compress( &cinfo );
    fclose( outfile );
    
    st_Progress_Bar_Signal(global_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar); 

    /* success code is 1! */
    return;
}

void _st_Read_JPEG (int16_t this_win_handle,  boolean file_process){

    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    if(this_win->wi_data->stop_original_data_load == FALSE){
        int16_t nb_components_24bits = 3, nb_components_32bits = 4, nb_components_original;

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"JPEG READING");

        // Variables for the source jpg
        u_int32_t jpg_size;
        u_int8_t *jpg_buffer;

        const char *file_name;
        FILE *fp;

        // Variables for the decompressor itself
        struct jpeg_decompress_struct cinfo;
        struct jpeg_error_mgr jerr;

        // Variables for the output buffer, and how long each row is
        int16_t row_stride, width, height;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Opening file");

        cinfo.err = jpeg_std_error(&jerr);

        jpeg_create_decompress(&cinfo);

        if( file_process == TRUE ){
            file_name = this_win->wi_data->path;
            fp = fopen(file_name, "rb");
            if (!fp){ 
                form_alert(1, "[1][File could not be opened for reading][Okay]"); 
                return;
            }
            jpeg_stdio_src(&cinfo, fp);
        } else {
            jpg_size = this_win->wi_data->STAT_FILE.st_size;
            jpg_buffer = (u_int8_t*) this_win->wi_data->original_buffer;
            jpeg_mem_src(&cinfo, jpg_buffer, jpg_size);
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 30, (int8_t*)"Reading header");
        if (jpeg_read_header(&cinfo, TRUE) != 1) {
            form_alert(1, "[1][Data does not seem to be a normal JPEG][Okay]");
            exit(EXIT_FAILURE);
        }

        cinfo.dct_method = JDCT_IFAST;
        cinfo.do_fancy_upsampling = FALSE;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 45, (int8_t*)"Decompress");
        jpeg_start_decompress(&cinfo);
        
        width = cinfo.output_width;
        height = cinfo.output_height;
        
        nb_components_original = cinfo.output_components;

        if(width <= 0 || height <= 0 || nb_components_original <= 0){
            form_alert(1, "[1][Header Values Error][Okay]");
            exit(EXIT_FAILURE);
        }

        u_int8_t* RGB_Buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_original << 3);
        if(RGB_Buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_original);
            st_form_alert(FORM_EXCLAM, alert_message);
            return;
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 60, (int8_t*)"Processing scanlines");
        row_stride = width * nb_components_original;

        while (cinfo.output_scanline < cinfo.output_height) {
            u_int8_t *buffer_array[1];
            buffer_array[0] = RGB_Buffer + \
                            (cinfo.output_scanline) * row_stride;

            jpeg_read_scanlines(&cinfo, buffer_array, 1);
        }

        jpeg_finish_decompress(&cinfo);
        jpeg_destroy_decompress(&cinfo);

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 90, (int8_t*)"Building ARGB buffer");
        u_int8_t* ARGB_Buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(ARGB_Buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_EXCLAM, alert_message);
            return;
        }
        

        const size_t totalPixels = width * height;
        u_int32_t i = 0;
        u_int8_t* src_ptr = (u_int8_t*)RGB_Buffer;
        u_int32_t* dst_ptr = (u_int32_t*)ARGB_Buffer;
        
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }        
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)ARGB_Buffer, width, height, nb_components_32bits << 3);

        u_int32_t index = 0, j = 0, x, y;

        for(y = 0; y < height; y++){
            for(x = 0; x < width; x++){
                i = (x + y * MFDB_STRIDE(width));               
                dst_ptr[i++] = 0XFF << 24 | ( src_ptr[j++] & 0xFF ) << 16| ( src_ptr[j++] & 0xFF ) << 8 | ( src_ptr[j++] & 0xFF );
            }
        }

        mem_free(RGB_Buffer);
        
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);

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
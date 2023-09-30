#include "img_tga.h"
#include "../external/tgafunc/tgafunc.h"
#include "../utils/utils.h"
#include "../img_handler.h"

void st_Win_Print_TGA(int16_t this_win_handle);
void _st_Read_TGA(int16_t this_win_handle, boolean file_process);

void st_Init_TGA(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_TGA;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    this_win->prefers_file_instead_mem = TRUE; /* If FALSE the original file will be copied to memory and available in this_win->wi_data->original_buffer */
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_TGA(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_TGA(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}


void _st_Read_TGA(int16_t this_win_handle, boolean file_process)
{
	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }
    if(this_win->wi_data->stop_original_data_load == FALSE){
        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"TGA READING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");

        const char *image_name = this_win->wi_data->path;
        uint8_t *data;
        tga_info *info;
        enum tga_error error_code;
        if(file_process == TRUE){
            error_code = tga_load(&data, &info, image_name);
            if(error_code != TGA_NO_ERROR){
            sprintf(alert_message, "%s\nTGA_NO_ERROR %d\n", image_name, TGA_NO_ERROR);
            st_form_alert(FORM_EXCLAM, alert_message); 
            }
        }

        int16_t nb_components_32bits = 4;
        int16_t width = tga_get_image_width(info);
        int16_t height = tga_get_image_height(info);
        int16_t depth = tga_get_bytes_per_pixel(info) << 3;
        int16_t pix_format = tga_get_pixel_format(info);

        u_int8_t   a, r, g, b;
        u_int32_t  i, j;
        int16_t x, y;

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        uint8_t* pixel;
        uint16_t pix16;

        for (y = 0; y < height; y++)
        {
            for (x = 0; x < width; x++)
            {
                pixel = tga_get_pixel(data, info, x, y);
                i = (x + (y * (MFDB_STRIDE(width)))) * nb_components_32bits;
                switch (depth)
                {
                case 32:
                    destination_buffer[i++] = pixel[3];
                    destination_buffer[i++] = pixel[2];
                    destination_buffer[i++] = pixel[1];
                    destination_buffer[i++] = pixel[0];
                    break;
                case 24:
                    destination_buffer[i++] = 0xFF;
                    destination_buffer[i++] = pixel[2];
                    destination_buffer[i++] = pixel[1];
                    destination_buffer[i++] = pixel[0];
                    break;
                case 16:
                    pix16 = ((pixel[1] << 8) | pixel[0]);
                    if(pix_format == 0){
                        /*to do verify : need targa 16bit gray*/
                        destination_buffer[i++] = 0xFF;
                        destination_buffer[i++] = pixel[1];
                        destination_buffer[i++] = pixel[1];
                        destination_buffer[i++] = pixel[1];                        
                    }
                    else{
                        destination_buffer[i++] = 0xFF;
                        destination_buffer[i++] = (u_int8_t)((pix16 & 0x7C00) >> 10) << 3;
                        destination_buffer[i++] = (u_int8_t)((pix16 & 0x03E0) >>  5) << 3;
                        destination_buffer[i++] = (u_int8_t)(pix16 & 0x001F) << 3;
                    }
                    break;
                case 8:
                    if(pix_format == 0){
                        destination_buffer[i++] = 0xFF;
                        destination_buffer[i++] = pixel[0];
                        destination_buffer[i++] = pixel[0];
                        destination_buffer[i++] = pixel[0];  
                    }else{
                        destination_buffer[i++] = 0xFF;
                        destination_buffer[i++] =  (pixel[0] >> 5) * 255 / 7;
                        destination_buffer[i++] = ((pixel[0] >> 2) & 0x07) * 255 / 7;
                        destination_buffer[i++] = (pixel[0] & 0x03) * 255 / 3    ;  
                    }
                    break;
                default:
                    break;
                }
            }
        }     
        
        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, nb_components_32bits << 3);
        this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
        this_win->total_length_h = this_win->wi_original_mfdb.fd_h;


        if(file_process == FALSE){
            mem_free(this_win->wi_data->original_buffer);
        }
        tga_free_data(data);
        tga_free_info(info);
        this_win->wi_data->stop_original_data_load = TRUE;

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
    }
}

void st_Write_TGA(u_int8_t* src_buffer, int width, int height, const char* filename){

    int16_t nb_components24b = 3;
    u_int8_t *destination_buffer = NULL;

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"TGA EXPORT START");
    st_Progress_Bar_Signal(global_progress_bar, 35, (int8_t*)"TGA image encoding");

    uint8_t *data;
    tga_info *info;
    enum tga_error error_code;

    error_code = tga_create(&data, &info, width, height, TGA_PIXEL_RGB24);

    if (error_code == TGA_NO_ERROR) {

        uint8_t *pixel;
        u_int32_t  i;
        int16_t x, y;
        for ( x = 0; x < width; ++x)
        {
            for ( y = 0; y < height; ++y)
            {
                i = ((x + (y * (MFDB_STRIDE(width)))) * 3) ; /* If src_buffer is ARGB */
                pixel = tga_get_pixel(data, info, x, y);
                pixel[2] = src_buffer[i++]; // Blue channel.
                pixel[1] = src_buffer[i++]; // Green channel.
                pixel[0] = src_buffer[i++]; // Red channel.
            }
        }        
        error_code = tga_save_from_info(data, info, filename);
        if (error_code != TGA_NO_ERROR) {
            form_alert(1, "[1][Image save failed][Okay]"); 
        }

        tga_free_data(data);
        tga_free_info(info);
    }

    st_Progress_Bar_Signal(global_progress_bar, 75, (int8_t*)"Saving image file");
  
    st_Progress_Bar_Signal(global_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);        

}

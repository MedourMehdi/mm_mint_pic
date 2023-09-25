#include "img_bmp.h"
#include "../external/qdbmp/qdbmp.h"
#include "../utils/utils.h"
#include "../img_handler.h"

void st_Win_Print_BMP(int16_t this_win_handle);
void _st_Read_BMP(int16_t this_win_handle, boolean file_process);

void st_Init_BMP(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
	this_win->refresh_win = st_Win_Print_BMP;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    this_win->prefers_file_instead_mem = FALSE; /* If FALSE the original file will be copied to memory and available in this_win->wi_data->original_buffer */
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_BMP(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->needs_refresh == TRUE){
        this_win->wi_data->wi_original_modified = FALSE;
        this_win->wi_data->needs_refresh = FALSE;
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    } 

    _st_Read_BMP(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_BMP(int16_t this_win_handle, boolean file_process)
{
	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }
    if(this_win->wi_data->wi_original_modified == FALSE){
        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"BMP READING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");

        BMP *img;
        if(file_process == TRUE){
            /* QDBMP seems broken for reading file */
            img = BMP_ReadFile(this_win->wi_data->path);
            if(img == NULL){
                sprintf(alert_message, "Reading file error\n%s", this_win->wi_data->path);
                st_form_alert(FORM_EXCLAM, alert_message);
            }
        } else {
            img = BMP_ReadBuffer((char *)this_win->wi_data->original_buffer);
        }

        int16_t nb_components_32bits = 4;
        int16_t width = BMP_GetWidth( img );
        int16_t height = BMP_GetHeight( img );
        int16_t depth = BMP_GetDepth( img );

        u_int8_t   r, g, b, index_color;
        u_int32_t  i;
        int16_t x, y;

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_STOP, alert_message);
            return;
        } else {
            for (y = 0; y < height; y++)
            {
                for (x = 0; x < width; x++)
                {
                    
                    if(depth == 8 || depth == 4){
                        BMP_GetPixelIndex( img, x, y, &index_color );
                        BMP_GetPaletteColor( img, index_color, &r, &g, &b);
                    }
                    else{
                        BMP_GetPixelRGB( img, (u_int32_t)x, (u_int32_t)y, &r, &g, &b );
                    }

                    i = (x + (y * (MFDB_STRIDE(width)))) * nb_components_32bits;
                    destination_buffer[i++] = 0xFF;
                    destination_buffer[i++] = r;
                    destination_buffer[i++] = g;
                    destination_buffer[i++] = b;

                }
            }     

            this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
            this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, nb_components_32bits << 3);
 
            if(file_process == FALSE){
                mem_free(this_win->wi_data->original_buffer);
            }
            BMP_Free( img );
            this_win->wi_data->wi_original_modified = TRUE;
        }

        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
    }
}

void st_Write_BMP(u_int8_t* src_buffer, int width, int height, const char* filename){

    int16_t nb_components24b = 3;
    u_int8_t *destination_buffer = NULL;

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"BMP EXPORT START");
    st_Progress_Bar_Signal(global_progress_bar, 35, (int8_t*)"BMP image encoding");
    
    BMP* img = BMP_Create(width, height, nb_components24b << 3);
    u_int32_t  i;
    int16_t x, y;
    for ( x = 0; x < width; ++x)
    {
        for ( y = 0; y < height; ++y)
        {
            i = ((x + (y * (MFDB_STRIDE(width)))) << 2) ; /* If src_buffer is ARGB */
            BMP_SetPixelRGB(img, x, y, src_buffer[i + 1], src_buffer[i + 2], src_buffer[i + 3]);
        }
    }

    st_Progress_Bar_Signal(global_progress_bar, 75, (int8_t*)"Saving image file");

    BMP_WriteFile(img, filename);
    BMP_Free( img );
      
    st_Progress_Bar_Signal(global_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);        

}
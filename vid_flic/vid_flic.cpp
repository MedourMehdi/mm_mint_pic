#include "vid_flic.h"
#include "../img_handler.h"

#include "../utils/utils.h"

#include "../external/flic/flic.h"

void st_Win_Video_Flic(int16_t this_win_handle);
void _st_Read_Flic(int16_t this_win_handle, boolean file_process);

void st_Init_Flic(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Video_Flic;
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Video_Flic(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->video_media){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void *st_Win_Play_Flic_Video(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    u_int32_t time_start, time_end, duration, delay;

    /* You should first get width and height in order to build the destination buffer */
restart:

    FILE* f = std::fopen(this_win->wi_data->path, "rb");
    flic::StdioFileInterface file(f);
    flic::Decoder decoder(&file);
    flic::Header header;
    if (!decoder.readHeader(header)){
        sprintf(alert_message, "Header file error\n");
        st_form_alert(FORM_EXCLAM, alert_message);
    }

    u_int16_t width = header.width;
    u_int16_t height = header.height;
    this_win->wi_data->img.img_total = header.frames;
    delay = header.speed;

    u_int8_t* destination_buffer_8bpp = st_ScreenBuffer_Alloc_bpp(width, height, 8);
    if(destination_buffer_8bpp == NULL){
        sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height);
        st_form_alert(FORM_EXCLAM, alert_message);
    }

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
    if(destination_buffer == NULL){
        sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
        st_form_alert(FORM_EXCLAM, alert_message);
    }
    if(this_win->wi_original_mfdb.fd_addr != NULL){
        mem_free(this_win->wi_original_mfdb.fd_addr);
    }
    mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
    st_MFDB_Fill(&this_win->wi_original_mfdb, 0XFFFFFFFF);

    uint32_t *pixel = (uint32_t*)destination_buffer;
    uint32_t total_pixels = width * height;

    this_win->wi_data->img.scaled_pourcentage = 0;
    this_win->wi_data->img.rotate_degree = 0;
    this_win->wi_data->resized = FALSE;
    this_win->wi_data->img.original_width = width;
    this_win->wi_data->img.original_height = height;

    this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
    this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
    this_win->wi_data->stop_original_data_load = TRUE;
    this_win->wi_data->wi_buffer_modified = FALSE;

    this_win->refresh_win(this_win->wi_handle);

    /* Then we fill the buffer with our frame and refresh until stop action is called */   
    flic::Frame frame;
    frame.pixels = &destination_buffer_8bpp[0];
    frame.rowstride = MFDB_STRIDE(header.width);

    while( (this_win->wi_data->img.img_id < this_win->wi_data->img.img_total) && this_win->wi_data->wi_pth != NULL ){
        if(this_win->wi_data->play_on || this_win->wi_data->img.img_id == 0){
            time_start = clock();        

            decoder.readFrame(frame);

            for(int32_t index = 0; index < total_pixels; index++){
                pixel[index] = ( 0xFF ) << 24 | 
                ( (frame.colormap[ destination_buffer_8bpp[index] ].r ) << 16 ) | 
                ( (frame.colormap[ destination_buffer_8bpp[index] ].g )  << 8 ) |
                (frame.colormap[ destination_buffer_8bpp[index] ].b );
            }
            // /* This is for non indexed colors */
            // for(int32_t index = 0; index < total_pixels; index++){
            //     pixel[index] = ( 0xFF ) << 24 | 
            //     ( ((  destination_buffer_8bpp[index]  >> 5) * 255 / 7) << 16 ) | 
            //     ( (((  destination_buffer_8bpp[index] >> 2) & 0x07) * 255 / 7) << 8 ) |
            //     (  destination_buffer_8bpp[index] & 0x03) * 255 / 3;
            // }            
       
            time_end = clock();
            duration = 5 * (time_end - time_start);

            while( duration <  delay){
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
            goto restart;
        }
        pthread_yield_np();
    }
    mem_free(destination_buffer_8bpp);
    send_message(this_win_handle, WM_CLOSED);
    return NULL;     

}
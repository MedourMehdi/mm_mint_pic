#include "snd_wav.h"
#include "../img_handler.h"

#include "../utils/utils.h"

#include "../external/wav_lib/wavreader.h"

#include "../utils_snd/utils_snd.h"

void st_Win_Video_WAV(int16_t this_win_handle);
void _st_Read_WAV(int16_t this_win_handle, boolean file_process);
void _st_Load_WAV(int16_t this_win_handle);

void st_Init_WAV(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Video_WAV;
    this_win->wi_snd = st_Init_Sound_Struct();
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
}

void st_Win_Video_WAV(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->video_media){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;     
    }

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}


void* st_Callback_Audio_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);     
    while(this_win->wi_data->wi_pth != NULL){

        if( this_win->wi_data->play_on || this_win->wi_snd->flip_play_action ){

            st_Sound_Feed((void*)this_win->wi_snd);

        } 
        pthread_yield_np();
    }
    // printf("this_win->wi_data->wi_pth == NULL\n");
    return NULL;
}

void* st_Close_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
    wav_read_close(this_win->wi_snd->user_data);
    st_Sound_Close((void*)this_win->wi_snd);
    // printf("st_Close_WAV\n");
    return NULL;
}

void *st_Win_Play_WAV(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    _st_Load_WAV( this_win_handle);
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    pthread_t thread_audio;

    st_Preset_Snd((void*)this_win->wi_snd);
    st_Sound_Buffer_Alloc((void*)this_win->wi_snd);
    st_Init_Sound((void*)this_win->wi_snd);

    pthread_create( &thread_audio, NULL, st_Callback_Audio_WAV, _this_win_handle);

    pthread_join( thread_audio, NULL);

    // printf("st_Close_WAV(_this_win_handle);\n");
    st_Close_WAV(_this_win_handle);
    // printf("send_message(this_win_handle, WM_CLOSED);\n");
    send_message(this_win_handle, WM_CLOSED);
    // printf("Done\n");
    /* Play & Pth & Pause */

    return NULL;
}

void _st_Load_WAV(int16_t this_win_handle){   
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
		u_int16_t width = 120;
		u_int16_t height = CONTROLBAR_H;
        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
        st_MFDB_Fill(&this_win->wi_original_mfdb,GREY_COLOR);
        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;  
        this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;   
        if(this_win->wi_to_display_mfdb != NULL){
            if(this_win->wi_to_display_mfdb->fd_addr != NULL){
                st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);
            }
        } 
        update_struct_window(this_win); 
        send_message(this_win_handle, WM_SIZED);
        _st_Read_WAV(this_win_handle, this_win->prefers_file_instead_mem);
        this_win->wi_data->stop_original_data_load = TRUE;	
	}
}

void* st_Process_Audio_WAV(void* _this_win_handle){
    // printf("Start st_Process_Audio_WAV\n");
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
	u_int16_t pcm_tmp;
	u_int32_t i;
    u_int32_t data_read = 0;

	data_read = wav_read_data(this_win->wi_snd->user_data, (unsigned char *)&this_win->wi_snd->pLogical[0], this_win->wi_snd->bufferSize);
    if(data_read){
        for(i = 0; i <  data_read; i += 2){
            pcm_tmp = (this_win->wi_snd->pLogical[i + 1]) << 8 | ((u_int16_t)this_win->wi_snd->pLogical[i] >> 8);
            memcpy(&this_win->wi_snd->pLogical[i], &pcm_tmp, sizeof(u_int16_t));
        }

        this_win->wi_snd->processedSize +=  data_read;
        if(data_read < this_win->wi_snd->bufferSize){
            memset((unsigned char *)&this_win->wi_snd->pLogical[data_read], 0x00, this_win->wi_snd->bufferSize - data_read);
        }
    }
    else{
        wav_read_close(this_win->wi_snd->user_data);
        this_win->wi_snd->user_data = (void*)wav_read_open(this_win->wi_data->path);
        this_win->wi_snd->processedSize = this_win->wi_snd->data_played = 0;
        this_win->wi_snd->time_start = this_win->wi_snd->time_end;
    }
    // printf("End st_Process_Audio_WAV\n");
    return NULL;
}

void _st_Read_WAV(int16_t this_win_handle, boolean file_process){

    struct_window *this_win = detect_window(this_win_handle);

    void* pWav;
    int bits_per_sample, format, sample_rate, channels;
    u_int32_t data_length;

    st_Path_Parser(this_win->wi_data->path, path_to_lnx );
    this_win->wi_snd->user_data = (void*)wav_read_open(this_win->wi_data->path);
    pWav = this_win->wi_snd->user_data;

    wav_get_header(pWav, 
                &format, 
                &channels,
                &sample_rate,
                &bits_per_sample, &data_length);
    // printf("--> bits_per_sample %d\n", bits_per_sample);

    this_win->wi_snd->effective_bytes_per_samples = bits_per_sample >> 3;
    this_win->wi_snd->original_channels = channels;
    this_win->wi_snd->original_samplerate = sample_rate;

    // printf("this_win->wi_snd->effective_bytes_per_samples %d, this_win->wi_snd->original_channels %d, this_win->wi_snd->original_samplerate %d\n", this_win->wi_snd->effective_bytes_per_samples, this_win->wi_snd->original_channels, this_win->wi_snd->original_samplerate);

    this_win->wi_snd->wanted_samplerate = this_win->wi_snd->original_samplerate;
    this_win->wi_snd->effective_channels = 2;

    this_win->wi_snd->sound_feed = st_Process_Audio_WAV;
    this_win->wi_snd->win_handle = this_win->wi_handle;
}


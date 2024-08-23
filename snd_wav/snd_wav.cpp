#ifdef WITH_WAVLIB
#include "../img_handler.h"

#include "snd_wav.hpp"
#include "../utils/utils.h"
#include "../utils_snd/utils_snd.h"

#include "../external/wav_lib/wavreader.h"

void st_Win_refresh_WAV(int16_t this_win_handle);

void* st_Init_Audio_WAV(void* _this_win_handle);
void* st_Callback_Audio_WAV(void* _this_win_handle);
void* st_Process_Audio_WAV(void* _this_win_handle);
void* st_Close_WAV(void* _this_win_handle);

void* st_Win_Snd_Default(void* _this_win_handle);

void st_Init_WAV(struct_window *this_win ){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_refresh_WAV;
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    this_win->render_win = NULL;

    this_win->wi_snd = (struct_snd *)mem_alloc(sizeof(struct_snd));
    this_win->wi_snd->bufferSize = 0;
    this_win->wi_snd->duration_s = 0;
    this_win->wi_snd->effective_channels = 0;
    this_win->wi_snd->effective_samplerate = 0;
    this_win->wi_snd->wanted_samplerate = 0;
    this_win->wi_snd->original_channels = 0;
    this_win->wi_snd->original_samplerate = 0;
    this_win->wi_snd->pBuffer = NULL;
    this_win->wi_snd->pLogical = NULL;
    this_win->wi_snd->pPhysical = NULL;
    this_win->wi_snd->prescale = 0;
    this_win->wi_snd->original_sampleformat = 0;
    this_win->wi_snd->sound_feed = NULL;
    this_win->wi_snd->win_handle = 0;
}

void st_Win_refresh_WAV(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    st_Start_Window_Process(this_win);
    this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
    st_Limit_Work_Area(this_win);
    st_End_Window_Process(this_win);
    printf("st_Win_refresh_WAV\n");
}

void *st_Win_Play_WAV(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);

    pthread_t thread_audio;

    st_Init_Audio_WAV(_this_win_handle);
    st_Sound_Buffer_Alloc((void*)this_win->wi_snd);
    st_Init_Sound((void*)this_win->wi_snd);
    pthread_create( &thread_audio, NULL, st_Callback_Audio_WAV, _this_win_handle);

    pthread_join( thread_audio, NULL);
    st_Close_WAV(_this_win_handle);

    send_message(this_win_handle, WM_CLOSED);
    printf("st_Win_Play_WAV\n");
    return NULL; 
}

void* st_Init_Audio_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
    void* pWav;
    int bits_per_sample, format, sample_rate, channels;
    u_int32_t data_length;
    // st_Path_to_Linux(this_win->wi_data->path);
    st_Path_Parser(this_win->wi_data->path, path_to_lnx );
    this_win->wi_snd->user_data = (void*)wav_read_open(this_win->wi_data->path);
    pWav = this_win->wi_snd->user_data;
    wav_get_header(pWav, 
                &format, 
                &channels,
                &sample_rate,
                &bits_per_sample, &data_length);
                printf("--> bits_per_sample %d\n", bits_per_sample);
    this_win->wi_snd->original_sampleformat = bits_per_sample >> 3;
    if(bits_per_sample >= 16){
        this_win->wi_snd->effective_sampleformat = 2;
        this_win->wi_snd->effective_bytes_per_samples = 2;
    } else {
        this_win->wi_snd->original_sampleformat = 1;
        this_win->wi_snd->effective_sampleformat = 2;
        this_win->wi_snd->effective_bytes_per_samples = 1;
    }
    this_win->wi_snd->original_channels = channels;
    this_win->wi_snd->original_samplerate = sample_rate;
    printf("this_win->wi_snd->original_channels %d, this_win->wi_snd->original_samplerate %d\n", this_win->wi_snd->original_channels, this_win->wi_snd->original_samplerate);
    this_win->wi_snd->wanted_samplerate = this_win->wi_snd->original_samplerate;
    this_win->wi_snd->effective_channels = 2;
    this_win->wi_snd->sound_feed = st_Process_Audio_WAV;
    this_win->wi_snd->win_handle = this_win->wi_handle;
    st_Preset_Snd((void*)this_win->wi_snd);
    st_Win_Snd_Default(_this_win_handle);
    printf("st_Init_Audio_WAV\n");
    return NULL;
}

void* st_Win_Snd_Default(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);    
    int16_t width = 320;
    u_int8_t* buffer = st_ScreenBuffer_Alloc_bpp(width, CONTROLBAR_H, screen_workstation_bits_per_pixel);
    mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)buffer, width, CONTROLBAR_H, screen_workstation_bits_per_pixel);
    st_Win_Set_Ready(this_win, width, CONTROLBAR_H);
    st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);
    send_message(this_win_handle, WM_REDRAW);
    printf("st_Win_Snd_Default\n");
    return NULL;
}

void* st_Callback_Audio_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);     
    while(this_win->wi_data->wi_pth != NULL){
        // if( this_win->wi_data->play_on){
            st_Sound_Feed((void*)this_win->wi_snd);
        // }
        st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);          
        send_message(this_win_handle, WM_REDRAW); 
        // printf("st_Callback_Audio_WAV\n");
        pthread_yield_np();
    }
    return NULL;
}
void* st_Process_Audio_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
	u_int16_t pcm_tmp;
	u_int32_t i;
	wav_read_data(this_win->wi_snd->user_data, (unsigned char *)&this_win->wi_snd->pLogical[0], this_win->wi_snd->bufferSize);

	for(i = 0; i <  this_win->wi_snd->bufferSize; i += 2){
		pcm_tmp = (this_win->wi_snd->pLogical[i + 1]) << 8 | ((u_int16_t)this_win->wi_snd->pLogical[i] >> 8);
		memcpy(&this_win->wi_snd->pLogical[i], &pcm_tmp, sizeof(u_int16_t));
	}
	this_win->wi_snd->processedSize +=  this_win->wi_snd->bufferSize;
    printf("st_Process_Audio_WAV\n");
    return NULL;
}

void* st_Close_WAV(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
    wav_read_close(this_win->wi_snd->user_data);
    Mfree( this_win->wi_snd->pBuffer );
	this_win->wi_snd->pBuffer = NULL;
    st_Sound_Close(NULL);
    printf("st_Close_WAV\n");
    return NULL;
}
#endif
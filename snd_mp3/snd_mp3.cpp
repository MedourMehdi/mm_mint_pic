#include "./snd_mp3.h"
#ifdef WITH_MP3LIB
#include "../img_handler.h"

#include "../utils/utils.h"

#define MINIMP3_IMPLEMENTATION
#include "../external/minimp3/minimp3.h"
#include "../external/minimp3/minimp3_ex.h"

typedef signed short SHORT;
typedef SHORT INT_PCM;

#include "../utils_snd/utils_snd.h"

void st_Win_Video_MP3(int16_t this_win_handle);
void _st_Read_MP3(int16_t this_win_handle, boolean file_process);
void _st_Load_MP3(int16_t this_win_handle);

void st_Init_MP3(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Video_MP3;
    this_win->wi_snd = st_Init_Sound_Struct();
    this_win->wi_data->img.img_id = 0;
    this_win->wi_data->img.img_index = 1;
    if(use_cached_icons){
        this_win->render_win = NULL;
    } else {
        if(!st_Set_Renderer(this_win)){
            sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
            st_form_alert(FORM_STOP, alert_message);
            return;
        }
    }
}

void st_Win_Video_MP3(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->video_media){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;     
    }

    if(use_cached_icons){
        st_Start_Window_Process(this_win);
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
        this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
        this_win->total_length_w = this_win->wi_to_display_mfdb->fd_w;
        this_win->total_length_h = this_win->wi_to_display_mfdb->fd_h;

        if(!this_win->wi_data->autoscale){st_Limit_Work_Area(this_win);}
        this_win->wi_data->remap_displayed_mfdb = FALSE;        
        st_End_Window_Process(this_win);
    } else {
        if( st_Img32b_To_Window(this_win) == false ){
            st_form_alert(FORM_STOP, alert_message);
        }    
    }
}

void* st_Callback_Audio_MP3(void* _this_win_handle){
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

void* st_Close_MP3(void* _this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
    mp3dec_ex_close((mp3dec_ex_t *)this_win->wi_snd->user_data);
    mem_free(this_win->wi_snd->user_data);

    st_Sound_Close((void*)this_win->wi_snd);
    // printf("st_Close_MP3\n");
    return NULL;
}

void *st_Win_Play_MP3(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    _st_Load_MP3( this_win_handle);
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    pthread_t thread_audio;
#ifdef USE_CIRCULAR_BUFFER
    pthread_t thread_unpack_data;
#endif
    st_Preset_Snd((void*)this_win->wi_snd);
    st_Sound_Buffer_Alloc((void*)this_win->wi_snd);
    st_Init_Sound((void*)this_win->wi_snd);

#ifdef USE_CIRCULAR_BUFFER
    this_win->wi_snd->bufferSize = this_win->wi_snd->effective_samplerate * this_win->wi_snd->effective_channels * this_win->wi_snd->effective_bytes_per_samples;
    this_win->wi_snd->global_circular_buffer = st_Sound_Build_Circular_Buffer(8, this_win->wi_snd->bufferSize);
    pthread_create( &thread_unpack_data, NULL, this_win->wi_snd->sound_feed, _this_win_handle);      
#endif

    pthread_create( &thread_audio, NULL, st_Callback_Audio_MP3, _this_win_handle);
    pthread_join( thread_audio, NULL);

#ifdef USE_CIRCULAR_BUFFER
    pthread_join( thread_unpack_data, NULL);
#endif

    st_Close_MP3(_this_win_handle);
    send_message(this_win_handle, WM_CLOSED);

    return NULL;
}

void _st_Load_MP3(int16_t this_win_handle){
    // printf("_st_Load_MP3\n");
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
		u_int16_t width = 120;
		u_int16_t height = CONTROLBAR_H;
        u_int16_t bpp_wanted = 32;

        if(this_win->wi_data->control_bar_media && use_cached_icons){
            bpp_wanted = screen_workstation_bits_per_pixel;
        }

        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, bpp_wanted);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", use_cached_icons ? (width * height * (bpp_wanted >> 2)) : (width * height) / (8 / bpp_wanted ) );
            st_form_alert(FORM_EXCLAM, alert_message);
        }
        if(this_win->wi_original_mfdb.fd_addr != NULL){
            mem_free(this_win->wi_original_mfdb.fd_addr);
        }
		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, bpp_wanted);
        st_MFDB_Fill(&this_win->wi_original_mfdb,GREY_COLOR);
        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;  
        this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
        update_struct_window(this_win);
        send_message(this_win_handle, WM_SIZED);
        _st_Read_MP3(this_win_handle, this_win->prefers_file_instead_mem);
        this_win->wi_data->stop_original_data_load = TRUE;	
	}
}

void* st_Process_Audio_MP3(void* _this_win_handle){
    // printf("Start st_Process_Audio_MP3\n");
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);

    u_int32_t data_read = 0;

    u_int32_t done = 0;
    u_int16_t* this_ptr = NULL;
    mp3dec_ex_t *mp3d = (mp3dec_ex_t *)this_win->wi_snd->user_data;
    
    unsigned char* pData[MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t)] = {0};
    
    data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;

    while( (done < this_win->wi_snd->bufferSize) && data_read) {

            this_ptr = (u_int16_t*)&this_win->wi_snd->pLogical[done];
            for (int16_t i = 0; i < data_read >> 1; i++) {
                *this_ptr++ = (( ((int16_t*) pData)[i] ) &0xFF00) | ((int16_t*) pData)[i] & 0x00FF;

            done += data_read;
            data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;
        }
    }

    if(done){
        this_win->wi_snd->processedSize +=  done;
        if(done < this_win->wi_snd->bufferSize){
            memset((unsigned char *)&this_win->wi_snd->pLogical[done], 0x00, this_win->wi_snd->bufferSize - done);
        }
    }
    else{
        mp3dec_ex_seek(mp3d, 0);
        #ifdef PRINT_REAL_HZ
        this_win->wi_snd->processedSize = this_win->wi_snd->data_played = 0;
        this_win->wi_snd->time_start = this_win->wi_snd->time_end;
        #endif
    }
    // printf("End st_Process_Audio_MP3\n");
    return NULL;
}

#ifdef USE_CIRCULAR_BUFFER
/* Circular Buffer */
void* st_Process_Audio_MP3_Circular_Buffer(void* _this_win_handle){

    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
	u_int16_t pcm_tmp;
	u_int32_t i;
    u_int32_t data_read = 0;
    circular_buffer *this_circular_buffer = this_win->wi_snd->global_circular_buffer;

    while(this_win->wi_data->wi_pth != NULL){
        if(this_circular_buffer->buffer_available && this_circular_buffer->next_buffer->buffer_available ){

            u_int32_t done = 0;
            u_int16_t* this_ptr = NULL;
            mp3dec_ex_t *mp3d = (mp3dec_ex_t *)this_win->wi_snd->user_data;
            
            unsigned char* pData[MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t)] = {0};
            
            data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;

            while( (done < this_win->wi_snd->bufferSize) && data_read) {
                this_ptr = (u_int16_t*)&this_circular_buffer->buffer[done];
                for (int16_t i = 0; i < data_read >> 1; i++) {
                    *this_ptr++ = (( ((int16_t*) pData)[i] ) &0xFF00) | ((int16_t*) pData)[i] & 0x00FF;
                }

                done += data_read;
                data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;
            }

            if(done){
                this_win->wi_snd->processedSize +=  done;
                this_circular_buffer->bytes_to_consume_size = done;
                if(done < this_win->wi_snd->bufferSize){
                    memset((unsigned char *)&this_circular_buffer->buffer[done], 0x00, this_win->wi_snd->bufferSize - done);
                }
                this_circular_buffer->buffer_available = FALSE;
            }
            else{
                mp3dec_ex_seek(mp3d, 0);
            }
        }
        this_circular_buffer = this_circular_buffer->next_buffer;
        pthread_yield_np();
    }

    return NULL;
}
/* End Circular Buffer */

void* st_Process_Audio_MP3_Circular_Buffer_8bit(void* _this_win_handle){

    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win = detect_window(this_win_handle);
	u_int16_t pcm_tmp;
	u_int32_t i;
    u_int32_t data_read = 0;
    circular_buffer *this_circular_buffer = this_win->wi_snd->global_circular_buffer;

    while(this_win->wi_data->wi_pth != NULL){
        if(this_circular_buffer->buffer_available && this_circular_buffer->next_buffer->buffer_available ){

            u_int32_t done = 0;
            u_int8_t* this_ptr = NULL;
            mp3dec_ex_t *mp3d = (mp3dec_ex_t *)this_win->wi_snd->user_data;
            
            unsigned char* pData[MINIMP3_MAX_SAMPLES_PER_FRAME * sizeof(mp3d_sample_t)] = {0};
            
            data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;

            while( (done < this_win->wi_snd->bufferSize) && data_read) {
                this_ptr = (u_int8_t*)&this_circular_buffer->buffer[done];
                for (int16_t i = 0; i < data_read ; i++) {
                    *this_ptr++ = ( ((u_int8_t*) pData)[i] );
                    i++;
                }

                done += data_read >> 1;
                data_read = (mp3dec_ex_read(mp3d, (mp3d_sample_t*)pData, MINIMP3_MAX_SAMPLES_PER_FRAME)) << 1;
            }

            if(done){
                this_win->wi_snd->processedSize +=  done;
                this_circular_buffer->bytes_to_consume_size = done;
                if(done < this_win->wi_snd->bufferSize){
                    memset((unsigned char *)&this_circular_buffer->buffer[done], 0x00, this_win->wi_snd->bufferSize - done);
                }
                this_circular_buffer->buffer_available = FALSE;
            }
            else{
                mp3dec_ex_seek(mp3d, 0);
            }
        }
        this_circular_buffer = this_circular_buffer->next_buffer;
        pthread_yield_np();
    }

    return NULL;
}


#endif

void _st_Read_MP3(int16_t this_win_handle, boolean file_process){

    struct_window *this_win = detect_window(this_win_handle);

    mp3dec_ex_t *mp3d = NULL;

    int format, sample_rate, channels;
    u_int32_t data_length;

    st_Path_Parser(this_win->wi_data->path, path_to_lnx );
    if(this_win->wi_snd->user_data == NULL){
        this_win->wi_snd->user_data = (void*)mem_calloc(1, sizeof(mp3dec_ex_t));
    }

    mp3dec_ex_open((mp3dec_ex_t *)this_win->wi_snd->user_data, this_win->wi_data->path, MP3D_SEEK_TO_SAMPLE);
    mp3d = (mp3dec_ex_t *)this_win->wi_snd->user_data;
    mp3dec_ex_seek(mp3d, 0);

    sample_rate = mp3d->info.hz;
	channels = mp3d->info.channels;

	data_length = mp3d->detected_samples >> 1;
    // printf("--> data_length %d\n", data_length);
if(computer_type < 3){
    this_win->wi_snd->effective_bytes_per_samples = 1;
} else {
    this_win->wi_snd->effective_bytes_per_samples = 2;
}

    this_win->wi_snd->original_channels = channels;
    this_win->wi_snd->original_samplerate = sample_rate;

    // printf("this_win->wi_snd->effective_bytes_per_samples %d\nthis_win->wi_snd->original_channels %d\nthis_win->wi_snd->original_samplerate %d\n", this_win->wi_snd->effective_bytes_per_samples, this_win->wi_snd->original_channels, this_win->wi_snd->original_samplerate);

    this_win->wi_snd->wanted_samplerate = this_win->wi_snd->original_samplerate;
    this_win->wi_snd->effective_channels = 2;
#ifdef USE_CIRCULAR_BUFFER
if(computer_type < 3){
    this_win->wi_snd->sound_feed = st_Process_Audio_MP3_Circular_Buffer_8bit;
}else{
    this_win->wi_snd->sound_feed = st_Process_Audio_MP3_Circular_Buffer;
}

#else
    this_win->wi_snd->sound_feed = st_Process_Audio_MP3;
#endif
    this_win->wi_snd->win_handle = this_win->wi_handle;
}
#endif
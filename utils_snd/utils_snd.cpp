#include "utils_snd.h"
#include "../utils/utils.h"

#include <mint/falcon.h>

int loadNewSample = 0;

void __attribute__((interrupt)) timerA( void )
{
	loadNewSample = 1;
	*( (volatile unsigned char*)0xFFFFFA0FL ) &= ~( 1<<5 );	//	clear in service bit
}

void enableTimerASei( void )
{
	*( (volatile unsigned char*)0xFFFFFA17L ) |= ( 1<<3 );	//	software end-of-interrupt mode
}

void *st_Preset_Snd(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    bool manage_samplerate = false;
    if(sound_struct->wanted_samplerate == 0){
        manage_samplerate = true;
        sound_struct->wanted_samplerate = sound_struct->original_samplerate;
    }
    /*	Atari platforms frequencies
    *	12584,25169,50352,0	; TT freq + delay = 0
    *	12929,24585,49170,0	; Falcon freq + delay = 0
    *	12300,24594,49165,0	; Aranym freq + delay = 0
    *	12273,25335,50667,0	; Vampire SAGA
    */

    /*
        For MilanBlaster the external clock is
        24.576 MHz when gpio(1,0) & 0x1l == 1l
        and 22.5792 MHz when gpio(1,0) &
        0x1l == 0l
    */
   
    Locksnd();
    sound_struct->left_lvl = (short)Soundcmd( LTATTEN, SND_INQUIRE );
    sound_struct->right_lvl = (short)Soundcmd( RTATTEN, SND_INQUIRE );   
    Sndstatus( SND_RESET );

    int clock_value = 25175000;
    sound_struct->use_clk_ext = 0;
    if(computer_type == 0x04 ){
        if((Gpio(1,0) & 0x1L) == 1L && milanblaster_present){
            /* 48khz */
            clock_value = 24576000;
            sound_struct->use_clk_ext = 2;
        }
        if((Gpio(1,0) & 0x1L) == 0L && milanblaster_present){
            /* 44.1khz */
            clock_value = 22579200;
            sound_struct->use_clk_ext = 1;
        }
    }

    if (computer_type == 0x06){
        u_int32_t gpio_data = Gpio(1,0);
        if(sound_struct->wanted_samplerate % 11025 == 0){
            gpio_data = (Gpio(1,0) & ~7);
            if(Gpio(2, gpio_data) < 0){
                printf("Gpio error\n");
            }
            /* 44.1khz */
            clock_value = 22579200;
            sound_struct->use_clk_ext = 1;            
        } else if (sound_struct->wanted_samplerate % 12000 == 0){
            gpio_data = (Gpio(1,0) & ~7) + 1;
            if(Gpio(2, gpio_data) < 0){
                printf("Gpio error\n");
            }
            gpio_data = Gpio(1,0);
            if( (gpio_data & 1L) ){
                /* 48khz */
                clock_value = 24576000;
                sound_struct->use_clk_ext = 2;
            }            
        } else {
            sound_struct->use_clk_ext = 0;
        }
    }

    sound_struct->prescale = ( ((clock_value >> 8 ) / sound_struct->wanted_samplerate - 1) ) ;

    switch (sound_struct->prescale)
    {
    case 1:
        switch (sound_struct->use_clk_ext)
        {
            case 1:
                sound_struct->effective_samplerate = 44100;
                break;
            case 2:
                sound_struct->effective_samplerate = 48000;
                break;
            default:
                sound_struct->effective_samplerate = 49165;
                break;
        }
        break;
    case 2:
        sound_struct->effective_samplerate = 32779;
        sound_struct->use_clk_ext = 0;
        break;
    case 3:
        switch (sound_struct->use_clk_ext)
        {
            case 1:
                sound_struct->effective_samplerate = 22050;
                break;
            case 2:
                sound_struct->effective_samplerate = 24000;
                break;
            default:
                sound_struct->effective_samplerate = 24594;
                break;
        }
        break;
    case 4:
        sound_struct->effective_samplerate = 19667;
        sound_struct->use_clk_ext = 0;
        break;
    case 5:
        sound_struct->effective_samplerate = 16389;
        sound_struct->use_clk_ext = 0;
        break;       
    case 6:
        switch (sound_struct->use_clk_ext)
        {
            case 1:
                sound_struct->effective_samplerate = 11025;
                break;
            case 2:
                sound_struct->effective_samplerate = 12000;
                break;
            default:
                sound_struct->effective_samplerate = 12273;
                break;
        }
        break;
    case 7:
        sound_struct->effective_samplerate = 9833;
        sound_struct->use_clk_ext = 0;
        break;
    case 8:
        sound_struct->effective_samplerate = 8194;
        sound_struct->use_clk_ext = 0;
        break;                                     
    default:
        printf("ERROR: Can not determine samplerate prescale for frequency %luHz\n", sound_struct->wanted_samplerate);
        break;
    }
    // printf("\n###\tst_Preset_Snd(%d) -> Wanted %luHz -> Playing at %luHz\n", sound_struct->prescale, sound_struct->wanted_samplerate, sound_struct->effective_samplerate);
    if(manage_samplerate){
        sound_struct->wanted_samplerate = sound_struct->effective_samplerate;
    }

    return NULL;
}

void *st_Sound_Buffer_Alloc(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    sound_struct->bufferSize = sound_struct->effective_samplerate * sound_struct->effective_channels * sound_struct->effective_bytes_per_samples;	// nb channels * 16 bit * FREQ Hz * 1 second
    sound_struct->pBuffer = (int8_t*)Mxalloc(sound_struct->bufferSize << 1, 0); /* Fois 2 parceque double buffer de son */
    memset(sound_struct->pBuffer,0x00, sound_struct->bufferSize << 1);
    sound_struct->pPhysical = sound_struct->pBuffer;
    sound_struct->pLogical = sound_struct->pBuffer + sound_struct->bufferSize;
    /* SURPLUS PKT */
    sound_struct->surplus_buffer = (u_int8_t*)mem_alloc(sound_struct->bufferSize);
    sound_struct->surplus_buffer_size = 0;
    // printf("-->DEBUG: Buffer size %d * %d * %d\n", sound_struct->effective_samplerate, sound_struct->effective_channels, sound_struct->effective_bytes_per_samples);
    // printf("-->DEBUG: sound_struct->bufferSize x 2 = %ld\n",(sound_struct->bufferSize << 1));
    return NULL;
}

void *st_Init_Sound(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;

#ifndef USE_CIRCULAR_BUFFER
        st_Sound_Load_And_Swap_Buffer(_sound_struct);
        st_Sound_Load_And_Swap_Buffer(_sound_struct);
#endif

    int32_t curadder = Soundcmd(ADDERIN, SND_INQUIRE); /* on recupère la source hardware actuelle */
    int32_t curadc = Soundcmd(ADCINPUT, SND_INQUIRE);    
    Soundcmd(ADCINPUT, 0);
    /*
    MODE_STEREO8 0 8-bit Stereo Mode
    MODE_STEREO16 1 16-bit Stereo Mode
    MODE_MONO 2 8-bit Mono Mode
    */ 
    if(sound_struct->effective_bytes_per_samples < 2){
        if(sound_struct->effective_channels < 2){
            Setmode( MODE_MONO );
        } else {
            Setmode( MODE_STEREO8 );
        }
    } else {
        Setmode( MODE_STEREO16 );
    }
    
    if(sound_struct->use_clk_ext){
        // printf("DEBUG: Using CLKEXT -> Devconnect( DMAPLAY, DAC, CLKEXT, sound_struct->prescale = %d, NO_SHAKE );\n", sound_struct->prescale);
        Devconnect( DMAPLAY, DAC, CLKEXT, sound_struct->prescale, NO_SHAKE );
    } else {
        // printf("DEBUG: Using CLK25M -> Devconnect( DMAPLAY, DAC, CLK25M, sound_struct->prescale = %d, NO_SHAKE );\n", sound_struct->prescale);        
        Devconnect( DMAPLAY, DAC, CLK25M, sound_struct->prescale, NO_SHAKE );
    }
   
    // Devconnect( DMAPLAY, DAC, CLK25M, sound_struct->prescale, NO_SHAKE );
	Setinterrupt( SI_TIMERA, SI_PLAY );
	Xbtimer( XB_TIMERA, 1<<3, 1, timerA );
	Supexec(enableTimerASei);    
    Buffoper( 0x00 );	// disable playback
    Jdisint( MFP_TIMERA );
    return NULL;
}

void *st_Sound_Feed(void *_sound_struct)
{
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    int16_t this_win_handle = sound_struct->win_handle;
    struct_window *this_win = detect_window(this_win_handle);
    // if(sound_struct->flip_play_action || loadNewSample){
    //     printf("flip_play_action = %d - loadNewSample %d\n", sound_struct->flip_play_action, loadNewSample);
    // }
    if(sound_struct->flip_play_action){
        // printf("sound_struct->flip_play_action %d - sound_struct->play %d \n", sound_struct->flip_play_action, sound_struct->play);
        if(sound_struct->play){
#ifdef USE_CIRCULAR_BUFFER
                if(!sound_struct->effective_circular_buffer){
                    sound_struct->effective_circular_buffer = sound_struct->global_circular_buffer;
                }
                Setbuffer( SR_PLAY, sound_struct->effective_circular_buffer->buffer , sound_struct->effective_circular_buffer->buffer + sound_struct->effective_circular_buffer->bytes_to_consume_size );
#else
                Setbuffer( SR_PLAY, sound_struct->pPhysical, sound_struct->pPhysical + sound_struct->bufferSize );
#endif
            Jenabint( MFP_TIMERA );
            Buffoper( SB_PLA_ENA | SB_PLA_RPT );
            sound_struct->flip_play_action = false;
            loadNewSample = true;
            #ifdef PRINT_REAL_HZ
            sound_struct->time_start = clock();
            // sound_struct->time_start = st_Supexec(get200hz);
            sound_struct->data_played = 0;
            #endif
        } else {
            Buffoper( 0x00 );	// disable playback
            Jdisint( MFP_TIMERA );
            sound_struct->flip_play_action = false;
            return NULL;
        }
    }
	if( loadNewSample )
	{
#ifdef USE_CIRCULAR_BUFFER
            circular_buffer * old_cb = sound_struct->effective_circular_buffer;
            circular_buffer * new_cb = sound_struct->effective_circular_buffer->next_buffer;
            
            /* set physical buffer for the next frame */
            Setbuffer( SR_PLAY, new_cb->buffer , 
                        new_cb->buffer + new_cb->bytes_to_consume_size );
            // printf("Playing Index %d\n", new_cb->buffer_index);
            sound_struct->effective_circular_buffer = new_cb;
            
            loadNewSample = 0;
            // memset(old_cb->buffer, 0x00, old_cb->bytes_to_consume_size);
            // old_cb->bytes_to_consume_size = 0;
            old_cb->buffer_available = TRUE;
#else
            st_Sound_Load_And_Swap_Buffer(_sound_struct);
            /* set physical buffer for the next frame */
            Setbuffer( SR_PLAY, sound_struct->pPhysical, sound_struct->pPhysical + sound_struct->bufferSize );
            loadNewSample = 0;
            #ifdef PRINT_REAL_HZ
            sound_struct->time_end = clock();
            // sound_struct->time_end = st_Supexec(get200hz);
            

            if(sound_struct->play){
                
                sound_struct->time_total = 5 * (sound_struct->time_end - sound_struct->time_start);
                sound_struct->data_played += sound_struct->time_total > 0 ? sound_struct->bufferSize : 0;
                
                if(sound_struct->data_played && sound_struct->time_total){
                    printf("Computed frequency %fhz\n", 
                    (float)(sound_struct->data_played * 1000) / (this_win->wi_snd->effective_channels * this_win->wi_snd->effective_bytes_per_samples * this_win->wi_snd->time_total));
                }

            }
            #endif
#endif
	} 
    return NULL;
}

void *st_Sound_Load_And_Swap_Buffer(void *_sound_struct)
{
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    /* fill in logical buffer */
    sound_struct->sound_feed( (void*)&sound_struct->win_handle );
    /* swap buffers (makes logical buffer physical) */
    int8_t* pTmp = sound_struct->pPhysical;
    sound_struct->pPhysical = sound_struct->pLogical;
    sound_struct->pLogical = pTmp;
    return NULL;
}

void *st_Sound_Close(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    Buffoper( 0x00 );	// disable playback
    Jdisint( MFP_TIMERA );
    Unlocksnd();
#ifdef USE_CIRCULAR_BUFFER
        st_Free_Circular_Buffer(sound_struct->global_circular_buffer);
#else
        mem_free(sound_struct->pBuffer);
        mem_free(sound_struct->surplus_buffer);
#endif
    return NULL;
}

void st_Sound_Float_to_PCM16(short *outbuf, float *inbuf, int length)
{
    int   count;

    const float mul = (32768.0f);
    for (count = 0; count <= length; count++) {
		int32_t tmp = (int32_t)(mul * inbuf[count]);
		tmp = MAX( tmp, -32768 ); // CLIP < 32768
		tmp = MIN( tmp, 32767 );  // CLIP > 32767
		outbuf[count] = tmp;
    }
}

void st_Sound_PCM16_to_Float(float *outbuf, short *inbuf, int length)
{
    int   count;

    const float div = (1.0f/32768.0f);
    for (count = 0; count <= length; count++) {
	outbuf[count] = div * (float) inbuf[count];
    }
}

int32_t st_Sound_Get_Playback_Position(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
	SndBufPtr local_ptr;
    int32_t current_position;
	Buffptr( (int32_t*)&local_ptr );
    if(local_ptr.play > (char *)sound_struct->pLogical){
        current_position = local_ptr.play - (char *)sound_struct->pLogical;
    }else{
        current_position = local_ptr.play - (char *)sound_struct->pPhysical;
    }
	return current_position;
}

struct_snd *st_Init_Sound_Struct(){
    struct_snd * wi_snd = (struct_snd *)mem_alloc(sizeof(struct_snd));
    wi_snd->bufferSize = 0;
    wi_snd->processedSize = 0;
    wi_snd->duration_s = 0;
    wi_snd->time_start = 0;
    wi_snd->time_end = 0;
    wi_snd->time_total = 0;
    wi_snd->data_played = 0;
    wi_snd->effective_channels = 0;
    wi_snd->effective_samplerate = 0;
    wi_snd->wanted_samplerate = 0;
    wi_snd->original_channels = 0;
    wi_snd->original_samplerate = 0;
    wi_snd->pBuffer = NULL;
    wi_snd->pLogical = NULL;
    wi_snd->pPhysical = NULL;
    wi_snd->global_circular_buffer = NULL;
    wi_snd->effective_circular_buffer = NULL;
    wi_snd->user_data = NULL;  
    wi_snd->prescale = 0;
    wi_snd->original_sampleformat = 0;
    wi_snd->effective_sampleformat = 0;
    wi_snd->sound_feed = NULL;
    wi_snd->win_handle = 0;
    wi_snd->use_clk_ext = 0;
    wi_snd->play = false;
    wi_snd->flip_play_action = false;
    return wi_snd;
}
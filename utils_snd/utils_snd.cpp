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
    int clock_value = 25175000;

    if(computer_type == 0x04 && gsxb_present){
        if(Gpio(1,0) & 0x1l == 1l){
            printf("DEBUG: GSXB Cookie present - clock_value = 24576000 ");
            // clock_value = 24576000;
        }
        if(Gpio(1,0) & 0x1l == 1l){
            printf("DEBUG: GSXB Cookie present - clock_value = 22579200 ");
            clock_value = 22579200;
            sound_struct->use_clk_ext = true;
        }
    }
    sound_struct->prescale = (((clock_value >> 8 ) / sound_struct->wanted_samplerate - 1) ) ;

    // printf("\n###\tst_Preset_Snd %d Wanted sample rate %luHz\n", sound_struct->prescale, sound_struct->wanted_samplerate);

    switch (sound_struct->prescale)
    {
    case 1:
        sound_struct->effective_samplerate = sound_struct->use_clk_ext ? 44100 : 50667;

        break;
    case 2:
        sound_struct->effective_samplerate = 32779;
        break;
    case 3:
        sound_struct->effective_samplerate = sound_struct->use_clk_ext ? 22050 : 25335;
        break;
    case 4:
        sound_struct->effective_samplerate = 19667;
        break;
    case 5:
        sound_struct->effective_samplerate = 16389;
        break;       
    case 6:
        sound_struct->effective_samplerate = sound_struct->use_clk_ext ? 11025 : 12273;
        break;
    case 7:
        sound_struct->effective_samplerate = 9833;
        break;
    case 8:
        sound_struct->effective_samplerate = 8194;
        break;                                     
    default:
        printf("ERROR: Can not determine samplerate prescale %lu\n", sound_struct->wanted_samplerate);
        break;
    }
    if(manage_samplerate){
        sound_struct->wanted_samplerate = sound_struct->effective_samplerate;
    }
	
    // printf("\n###\tst_Preset_Snd -> Playing at %luHz\n", sound_struct->effective_samplerate);

    return NULL;
}

void *st_Sound_Buffer_Alloc(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;
    sound_struct->bufferSize = sound_struct->effective_samplerate * sound_struct->effective_channels * sound_struct->effective_bytes_per_samples;	// nb channels * 16 bit * FREQ Hz * 1 second
    sound_struct->pBuffer = (int8_t*)Mxalloc(sound_struct->bufferSize << 2, 0 ); /* Fois 2 parceque double buffer de son */
    memset(sound_struct->pBuffer,0x00, sound_struct->bufferSize << 1);
    sound_struct->pPhysical = sound_struct->pBuffer;
    sound_struct->pLogical = sound_struct->pBuffer + sound_struct->bufferSize;
    /* SURPLUS PKT */
    sound_struct->surplus_buffer = (u_int8_t*)mem_alloc(sound_struct->bufferSize << 1);
    sound_struct->surplus_buffer_size = 0;
    return NULL;
}

void *st_Init_Sound(void *_sound_struct){
    struct_snd *sound_struct = (struct_snd*)_sound_struct;

    st_Sound_Load_And_Swap_Buffer(_sound_struct);
    st_Sound_Load_And_Swap_Buffer(_sound_struct);

    Locksnd();
    sound_struct->left_lvl = (short)Soundcmd( LTATTEN, SND_INQUIRE );
    sound_struct->right_lvl = (short)Soundcmd( RTATTEN, SND_INQUIRE );   
    Sndstatus( SND_RESET );
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
        Devconnect( DMAPLAY, DAC, CLKEXT, sound_struct->prescale, NO_SHAKE );
    } else {
        Devconnect( DMAPLAY, DAC, CLK25M, sound_struct->prescale, NO_SHAKE );
    }
   
    Devconnect( DMAPLAY, DAC, CLK25M, sound_struct->prescale, NO_SHAKE );
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
    if(sound_struct->flip_play_action){
        // printf("sound_struct->flip_play_action %d - sound_struct->play %d \n", sound_struct->flip_play_action, sound_struct->play);
        if(sound_struct->play){
            Setbuffer( SR_PLAY, sound_struct->pPhysical, sound_struct->pPhysical + sound_struct->bufferSize );
            Jenabint( MFP_TIMERA );
            Buffoper( SB_PLA_ENA | SB_PLA_RPT );
            sound_struct->flip_play_action = false;
            loadNewSample = true;
        } else {
            Buffoper( 0x00 );	// disable playback
            Jdisint( MFP_TIMERA );
            sound_struct->flip_play_action = false;
            return NULL;
        }
    }
	if( loadNewSample )
	{ 
        st_Sound_Load_And_Swap_Buffer(_sound_struct);
        /* set physical buffer for the next frame */
        Setbuffer( SR_PLAY, sound_struct->pPhysical, sound_struct->pPhysical + sound_struct->bufferSize );
        loadNewSample = 0;		
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
    mem_free(sound_struct->pBuffer);
    mem_free(sound_struct->surplus_buffer);
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
    wi_snd->duration_s = 0;
    wi_snd->effective_channels = 0;
    wi_snd->effective_samplerate = 0;
    wi_snd->wanted_samplerate = 0;
    wi_snd->original_channels = 0;
    wi_snd->original_samplerate = 0;
    wi_snd->pBuffer = NULL;
    wi_snd->pLogical = NULL;
    wi_snd->pPhysical = NULL;
    wi_snd->prescale = 0;
    wi_snd->original_sampleformat = 0;
    wi_snd->effective_sampleformat = 0;
    wi_snd->sound_feed = NULL;
    wi_snd->win_handle = 0;
    wi_snd->use_clk_ext = false;
    wi_snd->play = false;
    wi_snd->flip_play_action = false;
    return wi_snd;
}
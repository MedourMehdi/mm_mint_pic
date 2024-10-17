#include "../headers.h"
#include "../windows.h"

#ifndef UTILS_SND
#define UTILS_SND

extern int loadNewSample;

void *st_Preset_Snd(void *_sound_struct);
void *st_Sound_Buffer_Alloc(void *_sound_struct);

void *st_Init_Sound(void *_sound_struct);
void *st_Sound_Feed(void *_sound_struct);
void *st_Sound_Load_And_Swap_Buffer(void *_sound_struct);

void *st_Sound_Close(void *_sound_struct);

void st_Sound_Float_to_PCM16(short *outbuf, float *inbuf, int length);
void st_Sound_PCM16_to_Float(float *outbuf, short *inbuf, int length);

struct_snd *st_Init_Sound_Struct();
int32_t st_Sound_Get_Playback_Position(void *_sound_struct);

circular_buffer *st_Alloc_Circular_Buffer(u_int32_t max_buffer_size);
circular_buffer *st_Sound_Build_Circular_Buffer(u_int16_t max_buffer_index, u_int32_t max_buffer_size );
void st_Free_Circular_Buffer(circular_buffer *this_circular_buffer);

#endif
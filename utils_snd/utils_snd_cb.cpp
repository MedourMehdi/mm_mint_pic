#include "utils_snd.h"
#include "../utils/utils.h"

#include <mint/falcon.h>

/* Circular Buffer Stuff */
circular_buffer *st_Alloc_Circular_Buffer(u_int32_t max_buffer_size){
    circular_buffer *this_circular_buffer = (circular_buffer *)mem_alloc(sizeof(circular_buffer));
    this_circular_buffer->buffer = (u_int8_t*)Mxalloc(max_buffer_size, 0);
    memset(this_circular_buffer->buffer, 0x00, max_buffer_size);
    this_circular_buffer->buffer_available = TRUE;
    this_circular_buffer->buffer_index = 0;
    this_circular_buffer->max_buffer_index = 0;
    this_circular_buffer->max_buffer_size = max_buffer_size;
    this_circular_buffer->bytes_to_consume_size = 0;
    this_circular_buffer->next_buffer = NULL;
    return this_circular_buffer;
    /* sound_struct->bufferSize = sound_struct->effective_samplerate * sound_struct->effective_channels * sound_struct->effective_bytes_per_samples;	// nb channels * 16 bit * FREQ Hz * 1 second */
}

void st_Free_Circular_Buffer(circular_buffer *this_circular_buffer){
    u_int16_t i = 0;
    u_int16_t j = this_circular_buffer->max_buffer_index;
    circular_buffer *ptr_array[this_circular_buffer->max_buffer_index] = {NULL};

    for(i = 0; i < j; i++){
        ptr_array[i] = this_circular_buffer;
        this_circular_buffer = this_circular_buffer->next_buffer;        
    }

    for(i = 0; i < j; i++){
        Mfree(ptr_array[i]->buffer);
        mem_free(ptr_array[i]);
    }
}

circular_buffer *st_Sound_Build_Circular_Buffer(u_int16_t max_buffer_index, u_int32_t max_buffer_size ){
    circular_buffer *first_circular_buffer = NULL;
    circular_buffer *this_circular_buffer = NULL;
    circular_buffer *new_circular_buffer = NULL;
    for(int i = 0; i < max_buffer_index ;i++){
        new_circular_buffer = st_Alloc_Circular_Buffer(max_buffer_size);
        new_circular_buffer->buffer_index = i;
        new_circular_buffer->max_buffer_index = max_buffer_index;
        if(i == 0){
            first_circular_buffer = new_circular_buffer;
        }
        if( i == (max_buffer_index - 1)){
            new_circular_buffer->next_buffer = first_circular_buffer;
            first_circular_buffer->last_buffer = new_circular_buffer;
        }
        if(this_circular_buffer){
            new_circular_buffer->last_buffer = this_circular_buffer;
            this_circular_buffer->next_buffer = new_circular_buffer;
        }
        this_circular_buffer = new_circular_buffer;
    }
    return first_circular_buffer;
}

/* End Circular Buffer Stuff */
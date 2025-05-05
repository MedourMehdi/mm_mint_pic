#include "text.h"

void print_GEM(int16_t x, int16_t y, char* this_string){

    // GRECT rect;
    // rect.g_x = pxy[0];
    // rect.g_y = pxy[1];
    // rect.g_w = pxy[2] - pxy[0];
    // rect.g_y = pxy[3] - pxy[1];

    // u_int16_t current_pos_x = 0;
    // u_int16_t current_pos_y = hbox;
    // u_int16_t char_size = MIN(sizeof(this_string), rect.g_w/wbox);
    // char tmp_text[char_size + 1] = {'\0'};
    // strncpy(tmp_text, this_string, char_size);
    // vswr_mode( st_vdi_handle, MD_TRANS);
    // // printf("v_gtext(st_vdi_handle, %d , %d, %s)\n", rect.g_x , rect.g_y + hbox, tmp_text);
    // v_gtext(st_vdi_handle, rect.g_x , rect.g_y, tmp_text);
    // vswr_mode( st_vdi_handle, MD_REPLACE);

    vswr_mode( st_vdi_handle, MD_TRANS);
    v_gtext(st_vdi_handle, x , y , this_string);
    vswr_mode( st_vdi_handle, MD_REPLACE);    
}

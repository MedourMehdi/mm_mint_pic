#include "text.h"

void print_GEM(GRECT* rect, char* this_string){
    u_int16_t current_pos_x = 0;
    u_int16_t current_pos_y = hbox;
    u_int16_t char_size = MIN(sizeof(this_string), rect->g_w/wbox);
    char tmp_text[char_size + 1] = {'\0'};
    strncpy(tmp_text, this_string, char_size);
    vswr_mode( st_vdi_handle, MD_TRANS);			
    v_gtext(st_vdi_handle, rect->g_x , rect->g_y + hbox, tmp_text);
    vswr_mode( st_vdi_handle, MD_REPLACE);
}

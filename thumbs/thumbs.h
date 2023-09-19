#include "../headers.h"
#include "../windows.h"

#ifndef THUMBS_HEADERS
#define THUMBS_HEADERS

#define THUMB_MULTI_WINDOWS true

void st_Thumb_Desk_PXY_Update(struct_thumbs *this_win_thumb, int16_t* ext_pxy);

void* st_Thumb_MFDB_Update(void *p_param);

struct_thumbs* st_Thumb_Alloc(int16_t thumbs_nb, int16_t slave_win_handle, int16_t padx, int16_t pady, int16_t thumbnail_w_size, int16_t thumbnail_h_size);
void* st_Thumb_Free(void *this_win_thumb);

void st_Thumb_Refresh(int16_t win_thumb_handle);

void st_Handle_Click_Thumbnail(struct_window *this_win, int16_t mouse_x, int16_t mouse_y, int16_t mouse_button);
#endif
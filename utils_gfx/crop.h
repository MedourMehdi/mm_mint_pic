#include "../headers.h"
#include "../windows.h"
#include "../img_handler.h"
#ifndef CROP_HEADERS
#define CROP_HEADERS
bool st_Crop_To_MFDB(struct_crop* this_crop);
void st_Init_Crop(struct_window *this_win);
bool st_Crop_Finish(int16_t this_win_handle, int16_t mouse_x, int16_t mouse_y);
#endif
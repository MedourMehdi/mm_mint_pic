#include "headers.h"
#include "windows.h"

#ifndef CUSTOM_NEW_WINDOW_HEADERS
#define CUSTOM_NEW_WINDOW_HEADERS
int16_t new_win_thumbnails(const char* win_title, int16_t slave_win_handle);
int16_t new_win_crop(struct_crop* this_crop, const char* win_title);
int16_t new_win_form_rsc(const char *new_file, const char* win_title, int16_t object_index, int16_t *win_form_pos);
bool new_win_img(const char *new_file);
bool new_win_start();
void* new_win_img_threaded(void* _this_file);
void* new_win_start_threaded(void* );
#endif
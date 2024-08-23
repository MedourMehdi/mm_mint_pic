#ifndef RSC_COMMON_HEADER
#define RSC_COMMON_HEADER

#include "../headers.h"
#include "../windows.h"

void st_Disable_Editable_Object(OBJECT* this_object, struct_window* this_win_form);
void st_Enable_Editable_Object(OBJECT* this_object, struct_window* this_win_form);
void st_Refresh_Object(OBJECT* this_object, struct_window* this_win_form);

void st_Win_Refresh_Object(OBJECT* this_tree, int16_t this_object);

void reset_text(OBJECT *obj, char *str);

#endif
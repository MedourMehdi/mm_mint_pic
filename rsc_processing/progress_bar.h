#ifndef PROGRESS_BAR_HEADER
#define PROGRESS_BAR_HEADER

#include "../headers.h"
#include "../windows.h"

extern struct_win_progress_bar *win_global_progress_bar;
extern u_int16_t win_progress_bar_counter;

struct_win_progress_bar* st_Win_Progress_Bar_Alloc(void);
void st_Win_Progress_Bar_Destroy(struct_win_progress_bar* progress_bar);

void* new_progress_window(void* p_param);

void process_progress_bar(int16_t this_win_handle);

/* st_Win_Progress_Init return pointer to struct_win_progress_bar */
void* st_Win_Progress_Init(int16_t this_win_master_handle, const char *title, int16_t current_value, const char *progress_txt );
void st_Win_Progress_Bar_Finish(int16_t this_win_handle);

void st_Win_Progress_Bar_Update_Title(struct_win_progress_bar* progress_bar, const char *title);
void st_Win_Progress_Bar_Update_Info_Line(struct_win_progress_bar* progress_bar, int16_t current_value, const char *progress_txt);

#endif
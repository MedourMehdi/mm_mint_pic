#include "../headers.h"

void st_Form_Init_Change_Resolution(int16_t this_win_handle);
void st_Form_Events_Change_Resolution(int16_t);

void process_diag_export(int16_t this_win_handle);

/*

    OBJECT obj_gui_ftext_info = tree[DiagExport_HiddenInfo1];
    int16_t obj_pxy_ftext_info[4];
    obj_pxy_ftext_info[0] = obj_gui_ftext_info.ob_x + this_win_form->work_area.g_x;
    obj_pxy_ftext_info[1] = obj_gui_ftext_info.ob_y + this_win_form->work_area.g_y;
    obj_pxy_ftext_info[2] = obj_gui_ftext_info.ob_width + 3;
    obj_pxy_ftext_info[3] = obj_gui_ftext_info.ob_height + 3;
*/
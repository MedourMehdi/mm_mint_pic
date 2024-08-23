#include "change_rez.h"
#include "rsc_def.h"
#include "rsc_common.h"

#include "../utils/utils.h"

void st_Form_Update_Change_Resolution(u_int16_t new_rez_value, OBJECT* this_object, struct_window* this_win_form){
    struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);
    OBJECT* tree = this_win_form->wi_data->rsc.tree;
    int16_t focus_object = this_win_form->wi_data->rsc.current_object;

    char* obj_gui_ftext_new_ = this_object->ob_spec.tedinfo->te_ptext;
        sprintf(obj_gui_ftext_new_, "_____");
        sprintf(&obj_gui_ftext_new_[ strlen(obj_gui_ftext_new_) - ((new_rez_value) < 10000 ? ((new_rez_value) < 1000 ? 3 : 4) : 5) ],"%d", new_rez_value);
        st_Disable_Editable_Object(this_object, this_win_form);
}

void st_Form_Init_Change_Resolution(int16_t this_win_form_handle){

    struct_window* this_win_form = detect_window(this_win_form_handle);
    struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);
    OBJECT* tree = this_win_form->wi_data->rsc.tree;
	const char* str1 = "0_~- ";

#ifndef WITH_XBRZ
    tree[DiagResize_chk_xbrz].ob_state |= OS_DISABLED;
    tree[DiagResize_chk_xbrz].ob_flags ^= OF_SELECTABLE;
    tree[DiagResize_chk_xbrz].ob_flags |= OF_HIDETREE;
#endif
    char* obj_gui_ftext_curr_width = tree[DiagResize_FTEXTCurrW].ob_spec.tedinfo->te_ptext;
    char* obj_gui_ftext_curr_height = tree[DiagResize_FTEXTCurrH].ob_spec.tedinfo->te_ptext;
    char* obj_gui_ftext_new_width = tree[DiagResize_FTEXTNewW].ob_spec.tedinfo->te_ptext;
    char* obj_gui_ftext_new_height = tree[DiagResize_FTEXTNewH].ob_spec.tedinfo->te_ptext;
    
    int16_t window_width = wrez;
    int16_t window_height = hrez;

    int16_t original_width = this_win_master->wi_original_mfdb.fd_w;
    int16_t original_height = this_win_master->wi_original_mfdb.fd_h;    

	sprintf(&obj_gui_ftext_curr_width[ strlen(obj_gui_ftext_curr_width) - (original_width < 10000 ? (original_width < 1000 ? 3 : 4) : 5) ],"%d", original_width);
	replace_char(obj_gui_ftext_curr_width, str1[3], str1[4]);

	sprintf(&obj_gui_ftext_new_width[ strlen(obj_gui_ftext_new_width) - (window_width < 10000 ? (window_width < 1000 ? 3 : 4) : 5) ],"%d", window_width);
	replace_char(obj_gui_ftext_new_width, str1[3], str1[1]);

	sprintf(&obj_gui_ftext_curr_height[ strlen(obj_gui_ftext_curr_height) - (original_height < 10000 ? (original_height < 1000 ? 3 : 4) : 5) ],"%d", original_height);
	replace_char(obj_gui_ftext_curr_height, str1[3], str1[4]);

	sprintf(&obj_gui_ftext_new_height[ strlen(obj_gui_ftext_new_height) - (window_height < 10000 ? (window_height < 1000 ? 3 : 4) : 5) ],"%d", window_height);
	replace_char(obj_gui_ftext_new_height, str1[3], str1[1]);
    
}

void st_Form_Events_Change_Resolution(int16_t this_win_handle) {

    struct_window* this_win_form = detect_window(this_win_handle);
    struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);
    OBJECT* tree = this_win_form->wi_data->rsc.tree;
	const char* str1 = "0_~- ";

    int16_t original_width = this_win_master->wi_original_mfdb.fd_w;
    int16_t original_height = this_win_master->wi_original_mfdb.fd_h;
    int16_t focus_object = this_win_form->wi_data->rsc.current_object;

    char* obj_gui_ftext_new_width = tree[DiagResize_FTEXTNewW].ob_spec.tedinfo->te_ptext;
    char* obj_gui_ftext_new_height = tree[DiagResize_FTEXTNewH].ob_spec.tedinfo->te_ptext;    
    switch (focus_object)
    {
        case DiagResize_chk_yuv:

            tree[DiagResize_chk_manual_resize].ob_flags |= OF_SELECTABLE;
            tree[DiagResize_chk_manual_resize].ob_state ^= OS_DISABLED;
            st_Refresh_Object(&tree[DiagResize_chk_manual_resize], this_win_form);

            st_Form_Update_Change_Resolution(wrez, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(hrez, &tree[DiagResize_FTEXTNewH], this_win_form);            
            st_Enable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Enable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);

            form_button(tree, DiagResize_chk_manual_resize, 1, 0);

            this_win_master->wi_data->xbrz_scale = 0;

            break;
        case DiagResize_chk_xbrz:

            tree[DiagResize_chk_manual_resize].ob_state |= OS_DISABLED;
            tree[DiagResize_chk_manual_resize].ob_flags ^= OF_SELECTABLE;
            st_Refresh_Object(&tree[DiagResize_chk_manual_resize], this_win_form);

            st_Form_Update_Change_Resolution(original_width << 1, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(original_height << 1, &tree[DiagResize_FTEXTNewH], this_win_form); 
            form_button(tree, DiagResize_chk_resizex2, 1, 0);
            this_win_master->wi_data->xbrz_scale = 2;
            objc_draw(  tree, 0, MAX_DEPTH, 
                        this_win_form->work_area.g_x , 
                        this_win_form->work_area.g_y , 
                        this_win_form->work_area.g_w, this_win_form->work_area.g_h );                
        break;          
        case DiagResize_chk_manual_resize:
        if(!(tree[DiagResize_chk_manual_resize].ob_state & OS_DISABLED)){
            st_Form_Update_Change_Resolution(wrez, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(hrez, &tree[DiagResize_FTEXTNewH], this_win_form); 
            st_Enable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Enable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);
            this_win_master->wi_data->xbrz_scale = 0; 
        }
        break;
        case DiagResize_chk_resizex2:
            st_Form_Update_Change_Resolution(original_width << 1, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(original_height << 1, &tree[DiagResize_FTEXTNewH], this_win_form); 
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);
            if(tree[DiagResize_chk_manual_resize].ob_state & OS_DISABLED){
                this_win_master->wi_data->xbrz_scale = 2;
            }            
            break;
        case DiagResize_chk_resizex3:
            st_Form_Update_Change_Resolution(original_width * 3, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(original_height * 3, &tree[DiagResize_FTEXTNewH], this_win_form);
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);
            if(tree[DiagResize_chk_manual_resize].ob_state & OS_DISABLED){
                this_win_master->wi_data->xbrz_scale = 3;
            }
        break;
        case DiagResize_chk_resizex4:
            st_Form_Update_Change_Resolution(original_width << 2, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(original_height << 2, &tree[DiagResize_FTEXTNewH], this_win_form); 
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);
            if(tree[DiagResize_chk_manual_resize].ob_state & OS_DISABLED){
                this_win_master->wi_data->xbrz_scale = 4;
            }
        break;
        case DiagResize_chk_resizex5:
            st_Form_Update_Change_Resolution(original_width * 5, &tree[DiagResize_FTEXTNewW], this_win_form);
            st_Form_Update_Change_Resolution(original_height * 5, &tree[DiagResize_FTEXTNewH], this_win_form); 
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewW], this_win_form);
            st_Disable_Editable_Object(&tree[DiagResize_FTEXTNewH], this_win_form);
            if(tree[DiagResize_chk_manual_resize].ob_state & OS_DISABLED){
                this_win_master->wi_data->xbrz_scale = 5;
            }
        break;                
    default:
        break;
    }
    switch (this_win_form->wi_data->rsc.next_object)
    {
    case DiagResize_CancelButton:
        break;
    case DiagResize_OkButton:

        replace_char(tree[DiagResize_FTEXTNewW].ob_spec.tedinfo->te_ptext, str1[1], str1[0]);
        replace_char(tree[DiagResize_FTEXTNewH].ob_spec.tedinfo->te_ptext, str1[1], str1[0]);
        if(this_win_master->wi_data->autoscale){
            this_win_master->wi_data->autoscale = FALSE;
            this_win_master->wi_data->remap_displayed_mfdb = TRUE;
            this_win_master->refresh_win(this_win_master->wi_handle);
        }

        this_win_master->wi_data->img.export_width = atoi(tree[DiagResize_FTEXTNewW].ob_spec.tedinfo->te_ptext);
        this_win_master->wi_data->img.export_height = atoi(tree[DiagResize_FTEXTNewH].ob_spec.tedinfo->te_ptext);
        this_win_master->wi_data->fx_requested = TRUE;
        this_win_master->wi_data->resized = TRUE;
		this_win_master->refresh_win(this_win_master->wi_handle);
		send_message(this_win_master->wi_handle, WM_SIZED);
        break;
    default:
        break;
    }

    return;

}
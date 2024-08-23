#include "rsc_common.h"

#include "../utils/utils.h"

void st_Disable_Editable_Object(OBJECT* this_object, struct_window* this_win_form){
    this_object->ob_flags ^= OF_EDITABLE;
    this_object->ob_state |= OS_DISABLED;
    st_Refresh_Object(this_object, this_win_form);
}

void st_Enable_Editable_Object(OBJECT* this_object, struct_window* this_win_form){
    this_object->ob_flags |= OF_EDITABLE;
    this_object->ob_state ^= OS_DISABLED;
    st_Refresh_Object(this_object, this_win_form);
}

void st_Win_Refresh_Object(OBJECT* this_tree, int16_t this_object){
    GRECT	box;
    objc_offset(this_tree, this_object, &box.g_x, &box.g_y);
    box.g_w = this_tree[this_object].ob_width;
    box.g_h = this_tree[this_object].ob_height;				
    objc_draw(this_tree, ROOT, MAX_DEPTH, box.g_x, 
    box.g_y, 
    box.g_w, box.g_h);
}

void st_Refresh_Object(OBJECT* this_object, struct_window* this_win_form){
    u_int16_t this_padding_fix = 3;   
    objc_draw(  this_win_form->wi_data->rsc.tree, 0, MAX_DEPTH, 
                this_win_form->work_area.g_x + this_object->ob_x, 
                this_win_form->work_area.g_y + this_object->ob_y, 
                this_object->ob_width + this_padding_fix, this_object->ob_height + this_padding_fix );
                // printf("\nthis_win_form->work_area.g_x %d + this_object->ob_x %d = %d, this_win_form->work_area.g_y %d + this_object->ob_y %d = %d, this_object->ob_width + this_padding_fix = %d, this_object->ob_height + this_padding_fix = %d\n",
                // this_win_form->work_area.g_x, this_object->ob_x,
                // this_win_form->work_area.g_x + this_object->ob_x, 
                // this_win_form->work_area.g_y, this_object->ob_y, 
                // this_win_form->work_area.g_y + this_object->ob_y, 
                // this_object->ob_width + this_padding_fix, this_object->ob_height + this_padding_fix);
                return;
}

void reset_text(OBJECT *obj, char *str) {
    TEDINFO *obspec = obj->ob_spec.tedinfo;	/* Get TEDINFO address  */
    obspec->te_ptext = str;			/* Set new text pointer */
    obspec->te_txtlen = strlen((const char *)str);	/* Set new length	*/
}
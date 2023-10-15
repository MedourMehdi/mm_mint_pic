#include "winform.h"
#include "forms.h"

void refresh_form(int16_t this_win_handle);

void* st_Init_Form(void* p_param){
	struct_window *this_win = (struct_window*)p_param;

    this_win->wi_data->rsc_media = TRUE;

/* Init. editing	*/
    this_win->wi_data->rsc.startob = 0;
    this_win->wi_data->rsc.first_edit = &this_win->wi_data->rsc.startob;
    this_win->wi_data->rsc.next_object = *this_win->wi_data->rsc.first_edit;
    this_win->wi_data->rsc.edit_object = 0;

/* Initial hotspot cndx */
    this_win->wi_data->rsc.hotspot_object = NIL;
    new_objc_xywh(this_win->wi_data->rsc.tree, ROOT, &r1);

/* Main event loop	*/
    this_win->wi_data->rsc.we_continue = TRUE;

/*  CUSTOM CONFIGURATION */
    this_win->refresh_win = &refresh_form;
    this_win->wi_data->rsc.process_function = NULL;
/*  CUSTOM CONFIGURATION */

	return NULL;
}

void refresh_form(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    wind_update(BEG_UPDATE);

    st_Start_Window_Process(this_win);

    this_win->wi_data->rsc.tree->ob_x = this_win->work_area.g_x + 4;
    this_win->wi_data->rsc.tree->ob_y = this_win->work_area.g_y + 4;

    objc_draw(this_win->wi_data->rsc.tree, 0, MAX_DEPTH, 
        this_win->work_area.g_x + 4, this_win->work_area.g_y + 4, this_win->work_area.g_w, this_win->work_area.g_h);

	st_Set_Mouse( FALSE );
    graf_mouse(ARROW,0L);
    win_refresh_from_buffer(this_win);
    st_Set_Mouse( TRUE );

    // st_End_Window_Process(NULL);

    wind_update(END_UPDATE);
}

void st_Form_Handle(struct_window* this_win) {
    if(this_win->wi_data->rsc_media == TRUE) {
        if(this_win->wi_data->rsc.we_continue) {
            if (this_win->wi_data->rsc.edit_object != this_win->wi_data->rsc.next_object) {
                if (this_win->wi_data->rsc.next_object != 0) {
                    this_win->wi_data->rsc.edit_object = this_win->wi_data->rsc.next_object;
                    this_win->wi_data->rsc.next_object = 0;
                    objc_edit(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.edit_object, 0, &this_win->wi_data->rsc.char_pos, EDINIT);
                }
            }

            if (events & MU_M1) {
                this_win->wi_data->rsc.hotspot_object = form_hotspot(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.hotspot_object, mx, my, &r1, &r1_flags);
            }

            if (events & MU_KEYBD) { 
                // this_win->wi_data->rsc.we_continue = new_form_keybd(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.edit_object, this_win->wi_data->rsc.next_object, kc, &this_win->wi_data->rsc.next_object, &kc);
                this_win->wi_data->rsc.we_continue = form_keybd(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.edit_object, this_win->wi_data->rsc.next_object, ks, &this_win->wi_data->rsc.next_object, &ks);
                if (kc && this_win->wi_data->rsc.edit_object) {
                    objc_edit(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.edit_object, kc, &this_win->wi_data->rsc.char_pos, EDCHAR);
                }
            }

            if (events & MU_BUTTON) {
                this_win->wi_data->rsc.next_object = objc_find(this_win->wi_data->rsc.tree, ROOT, MAX_DEPTH, mx, my);
                this_win->wi_data->rsc.current_object = this_win->wi_data->rsc.next_object;
                if (this_win->wi_data->rsc.next_object == NIL) {
                    this_win->wi_data->rsc.next_object = 0;
                } else {
                    this_win->wi_data->rsc.we_continue = new_form_button(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.next_object, mc, &this_win->wi_data->rsc.next_object, &this_win->wi_data->rsc.hotspot_object);
                    // this_win->wi_data->rsc.we_continue = form_button(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.next_object, mc, &this_win->wi_data->rsc.next_object);

                }
            }
            if (!this_win->wi_data->rsc.we_continue || 
                (this_win->wi_data->rsc.next_object != this_win->wi_data->rsc.edit_object && this_win->wi_data->rsc.next_object != 0)) {
                if (this_win->wi_data->rsc.edit_object != 0) {
                    objc_edit(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.edit_object, 0, &this_win->wi_data->rsc.char_pos, EDEND);
                }
            }
        }
        // printf("In loop this_win->wi_handle %d\n", this_win->wi_handle);
        if(this_win->wi_data->rsc.process_function != NULL){
            // printf("before process form\n");
            this_win->wi_data->rsc.process_function(this_win->wi_handle);
            // printf("after process form\n");
        }
        if(!this_win->wi_data->rsc.we_continue) {
            if (this_win->wi_data->rsc.hotspot_object != (this_win->wi_data->rsc.next_object & 0x7fff)){
                if (this_win->wi_data->rsc.hotspot_object != NIL){
                    objc_toggle(this_win->wi_data->rsc.tree, this_win->wi_data->rsc.hotspot_object);
                }
            }
            *this_win->wi_data->rsc.first_edit = this_win->wi_data->rsc.edit_object;
            // this_win->wi_data->rsc.process_function(this_win->wi_handle);
            close_window(this_win->wi_handle);
        }
    }
}
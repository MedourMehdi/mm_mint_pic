#include "progress_bar.h"
#include "rsc_common.h"
#include "../new_window.h"
#include "../utils/utils.h"
#include "rsc_def.h"


struct_win_progress_bar* st_Win_Progress_Bar_Alloc(){
	struct_win_progress_bar* progress_bar = (struct_win_progress_bar*)mem_alloc(sizeof(struct_win_progress_bar));

	progress_bar->update_title = TRUE;
	progress_bar->update_info_line = TRUE;
	progress_bar->update_resquested = TRUE;

	progress_bar->win_form_handle = NIL;
	progress_bar->current_value = 1;

	char* title = NULL;
	char* info_line = NULL;

	progress_bar->locked = FALSE;
	progress_bar->done = FALSE;

    return progress_bar;
}

void st_Win_Progress_Bar_Destroy(struct_win_progress_bar* progress_bar){
    mem_free(progress_bar);
}

void* new_progress_window(void* p_param){

	struct_window*	this_win_master;

	const char*	this_rsc_file_to_load = MAIN_RSC_PATH;

	char rsc_file_to_load[strlen(current_path) + strlen(this_rsc_file_to_load) + 1] = {'\0'};
	strcpy(rsc_file_to_load, current_path);
	strcat(rsc_file_to_load, this_rsc_file_to_load);	

	const char*		window_form_title = "Please wait...";
	int16_t			rsc_object_index = Progress;
	int16_t 		this_win_form_pos[2];

	this_win_form_pos[0] = 30 + ((win_progress_bar_counter) * 40);
	this_win_form_pos[1] = 60 + ((win_progress_bar_counter) * 120);

	int16_t this_win_form_handle = new_win_form_rsc(rsc_file_to_load, window_form_title , rsc_object_index, this_win_form_pos);

	if(this_win_form_handle == NIL){
		form_alert(1, "[1][Error opening this form|Please get the source code and debug it!][Okay]");
	} else {

		struct_window* this_win_form = detect_window(this_win_form_handle);

		if(p_param == NULL){
			this_win_master = this_win_form;
		} else {
			this_win_master = (struct_window*)p_param;
		}

		if(this_win_master->wi_win_progress_bar == NULL){
			this_win_master->wi_win_progress_bar = st_Win_Progress_Bar_Alloc();
		}

		this_win_form->wi_data->rsc.win_master_handle = this_win_master->wi_handle;

		this_win_master->wi_form = &this_win_form->wi_data->rsc;

		this_win_form->wi_win_progress_bar = this_win_master->wi_win_progress_bar;
		this_win_form->wi_win_progress_bar->win_form_handle = this_win_form->wi_handle;
		// printf("this_win_form->wi_handle %d / this_win_form->wi_win_progress_bar->win_form_handle %d ", this_win_form->wi_win_progress_bar->win_form_handle, this_win_form->wi_handle);
		this_win_form->wi_data->rsc.process_function = &process_progress_bar;
		
		this_win_form->refresh_win(this_win_form->wi_handle);
		win_progress_bar_counter += 1;

	}
	return (void*)this_win_master;
}

void process_progress_bar(int16_t this_win_handle){

    struct_window* this_win_form = detect_window(this_win_handle);

	if(this_win_form == NULL){
		printf("Error = process_progress_bar(%d) -> this_win_form is NULL!\n", this_win_handle);
		return;
	}

	// printf("this_win_form->wi_data->rsc.win_master_handle = %d", this_win_form->wi_data->rsc.win_master_handle);
    // struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);

    OBJECT* tree = this_win_form->wi_data->rsc.tree;

    u_int32_t width_pbar, width_total;

	this_win_form->wi_win_progress_bar->locked = true;
	if(!this_win_form->wi_win_progress_bar->done ){

		if(this_win_form->wi_win_progress_bar->update_title && this_win_form->wi_win_progress_bar->title != NULL){
			reset_text(&tree[Progress_PROTITLE], this_win_form->wi_win_progress_bar->title);
			st_Win_Refresh_Object(tree, Progress_PROTITLE );
			this_win_form->wi_win_progress_bar->update_title = false;

		}
		if(this_win_form->wi_win_progress_bar->update_info_line && this_win_form->wi_win_progress_bar->info_line != NULL){

			width_total = tree[Progress_PROBOX].ob_width - 1;	/* Take border into account */
			width_pbar = MAX(1, (this_win_form->wi_win_progress_bar->current_value * width_total) / 100);
			tree[Progress_PROBAR].ob_width = width_pbar;

			if (this_win_form->wi_win_progress_bar->current_value){
				st_Win_Refresh_Object(tree, Progress_PROBAR );
			}

			reset_text(&tree[Progress_PLINE], this_win_form->wi_win_progress_bar->info_line);
			st_Win_Refresh_Object(tree, Progress_PLINE );

			this_win_form->wi_win_progress_bar->update_info_line = false;
		}
	}

	this_win_form->wi_win_progress_bar->locked = false;

    return;
}

void st_Win_Progress_Bar_Update_Info_Line(struct_win_progress_bar* progress_bar, int16_t current_value, const char *progress_txt){
	if(progress_bar){
		progress_bar->current_value = current_value;
		progress_bar->info_line = (char*)progress_txt;
		progress_bar->update_info_line = true;
		process_progress_bar(progress_bar->win_form_handle);
	}
}

void st_Win_Progress_Bar_Update_Title(struct_win_progress_bar* progress_bar, const char *title){
	if(progress_bar){
		progress_bar->title = (char*)title;
		progress_bar->update_title = true;
		process_progress_bar(progress_bar->win_form_handle);
	}
}

void st_Win_Progress_Bar_Finish(int16_t this_win_handle){
	struct_window* this_win_master = detect_window(this_win_handle);
	if(this_win_master){
		int16_t this_win_form_handle = this_win_master->wi_win_progress_bar->win_form_handle;
		this_win_master->wi_win_progress_bar->done = TRUE;
		struct_win_progress_bar* tmp_ptr = this_win_master->wi_win_progress_bar;
		// printf("close_window(this_win_form_handle %d)\n", this_win_form_handle );
		close_window(this_win_form_handle);
		win_progress_bar_counter -= 1;
		if(this_win_form_handle == this_win_handle){
			st_Win_Progress_Bar_Destroy(tmp_ptr);
		}
	}
}

void* st_Win_Progress_Init(int16_t this_win_master_handle, const char *title, int16_t current_value, const char *progress_txt ){
	struct_window* this_win_master = NULL;
	if(this_win_master_handle != NIL){
		this_win_master = detect_window(this_win_master_handle);
		new_progress_window((void*)this_win_master);
	}else{
		this_win_master = (struct_window*)new_progress_window((void*)this_win_master);
	}

	if(this_win_master){
		this_win_master->wi_win_progress_bar->title = NULL;
		this_win_master->wi_win_progress_bar->info_line = NULL;
		this_win_master->wi_win_progress_bar->done = FALSE;

		st_Win_Progress_Bar_Update_Title(this_win_master->wi_win_progress_bar, title);

		st_Win_Progress_Bar_Update_Info_Line(this_win_master->wi_win_progress_bar, current_value, progress_txt);

		return (struct_win_progress_bar*)this_win_master->wi_win_progress_bar;
	}else{
		printf("st_Win_Progress_Init Error\n");
		return NULL;
	}

}
#include "windows.h"
#include "utils/utils.h"
#include <errno.h>

pthread_t threads[NUM_THREADS] = {NULL};
int *taskids[NUM_THREADS] = {NULL};
int16_t total_thread = 0;

boolean clip_status = FALSE;
struct_window win_struct_array[MAX_WINDOWS] = {0};
int16_t number_of_opened_windows = 0;
boolean mouse_status = TRUE;

void do_arrow_y(int16_t my_win_handle, int16_t arrow_msg);
void do_arrow_x(int16_t my_win_handle, int16_t arrow_msg);

MFDB screen_mfdb = {0};

void st_Set_Mouse(boolean status){
	switch (status)
	{
	case TRUE:
		if(mouse_status != TRUE){
			graf_mouse(M_ON,0L);
			mouse_status = TRUE;
		}
		break;
	case FALSE:
		if(mouse_status == TRUE){
			graf_mouse(M_OFF,0L);
			mouse_status = FALSE;
		}
		break;
	default:
		break;
	}
}

void send_message(int16_t my_win_handle, int16_t my_message){
	TRACE(("Sending msg %d\n", my_message))
	int16_t my_win_message[8];
	struct_window *this_win;
	this_win = detect_window(my_win_handle);
	// update_struct_window(this_win);
	my_win_message[0] = my_message;
	my_win_message[1] = gl_apid;
	my_win_message[2] = 0;
	my_win_message[3] = my_win_handle;
	my_win_message[4] = this_win->ext_area.g_x;
	my_win_message[5] = this_win->ext_area.g_y;
	my_win_message[6] = this_win->ext_area.g_w;
	my_win_message[7] = this_win->ext_area.g_h;		
	appl_write(gl_apid, 16, &my_win_message);
}

struct_window *detect_window(int16_t my_win_handle){
	int16_t i = 0;
	struct_window *this_win = NULL;
	while(i < MAX_WINDOWS && number_of_opened_windows > 0){
		if(&win_struct_array[i] != NULL){
			if ( my_win_handle == win_struct_array[i].wi_handle ){
				this_win = &win_struct_array[i];
				break;
			}
		}
		i++;
	}
	return this_win;
}

boolean rsc_already_loaded(const char* rsc_file_name){
	int16_t i = 0;
	while(i < MAX_WINDOWS && number_of_opened_windows > 0){
		if(win_struct_array[i].wi_handle != 0){
			if(win_struct_array[i].wi_form != NULL){
				if ( strcmp(win_struct_array[i].wi_form->rsc_file, rsc_file_name) == 0 ){
					return true;
				}
			}
		}
		i++;
	}
	return false;
}

void update_struct_window(struct_window *this_win){
	TRACE(("update_struct_window(%d)\n", this_win->wi_handle))
	/* working (window internal) area */
	wind_get( this_win->wi_handle, WF_WORKXYWH, 
		&this_win->work_area.g_x, &this_win->work_area.g_y, &this_win->work_area.g_w, &this_win->work_area.g_h );
	/* external area */
	wind_get( this_win->wi_handle, WF_CURRXYWH, 
		&this_win->ext_area.g_x, &this_win->ext_area.g_y, &this_win->ext_area.g_w, &this_win->ext_area.g_h );
	/* full size external area */

	wind_get( this_win->wi_handle, WF_FULLXYWH, 
		&this_win->full_ext_area.g_x, &this_win->full_ext_area.g_y, &this_win->full_ext_area.g_w, &this_win->full_ext_area.g_h );
	if(this_win->wi_data->window_size_limited == TRUE && this_win->wi_data->autoscale == FALSE){
		int16_t total_length_ext_area_w, total_length_ext_area_h, dummy;
		wind_calc(WC_BORDER, this_win->wi_style, dummy, dummy, this_win->total_length_w, this_win->total_length_h,
			&dummy,	&dummy, &total_length_ext_area_w, &total_length_ext_area_h);
		this_win->full_ext_area.g_w = MIN(total_length_ext_area_w, this_win->full_ext_area.g_w);
		this_win->full_ext_area.g_h = MIN(total_length_ext_area_h, this_win->full_ext_area.g_h);
	}
	/* previous external area */
	wind_get( this_win->wi_handle, WF_PREVXYWH, 
		&this_win->previous_ext_area.g_x, &this_win->previous_ext_area.g_y, &this_win->previous_ext_area.g_w, &this_win->previous_ext_area.g_h );
	/* previous working area */
	wind_calc( WC_WORK, this_win->wi_style, 
		this_win->previous_ext_area.g_x, this_win->previous_ext_area.g_y, this_win->previous_ext_area.g_w, this_win->previous_ext_area.g_h,
		&this_win->previous_work_area.g_x, &this_win->previous_work_area.g_y, &this_win->previous_work_area.g_w, &this_win->previous_work_area.g_h );
	grect_to_array( &this_win->work_area, this_win->work_pxy);
	grect_to_array( &this_win->ext_area, this_win->ext_pxy);
}

void open_window(struct_window *this_win){

	int16_t x_win, y_win, w_win, h_win;

	this_win->wi_handle = wind_create(this_win->wi_style, xdesk, ydesk, wdesk, hdesk );
	wind_set_str(this_win->wi_handle, WF_NAME, this_win->wi_name);

    char *wi_info;
	if (this_win->wi_info != NULL){
		wi_info = this_win->wi_info;
	} else {
		wi_info = (char *)"NO NAME WINDOW";
	}
	wind_set_str( this_win->wi_handle, WF_INFO, wi_info);

	st_Set_Mouse( FALSE );

	x_win = (number_of_opened_windows << 4) + (wdesk >> 2);
	y_win = (number_of_opened_windows << 4) + (hdesk >> 2); /* height of desk divided by 4 */



	if(this_win->wi_data->rsc_media == TRUE){
		this_win->total_length_w = this_win->wi_data->rsc.tree->ob_width + 10;
		this_win->total_length_h = this_win->wi_data->rsc.tree->ob_height + 10;		
		w_win = this_win->total_length_w;
		h_win = this_win->total_length_h;

		x_win = wdesk - (x_win + w_win) < 0 ? x_win = wdesk - w_win : x_win;
		y_win = hdesk - (y_win + h_win) < 0 ? y_win = hdesk - h_win : y_win;

		wind_calc(WC_BORDER, this_win->wi_style, x_win, y_win, this_win->total_length_w, this_win->total_length_h,
			&x_win,	&y_win, &w_win, &h_win);
	} else if(this_win->wi_data->thumbnail_master == TRUE){
		wind_calc(WC_BORDER, this_win->wi_style, x_win, y_win, this_win->total_length_w, this_win->total_length_h,
			&x_win,	&y_win, &w_win, &h_win);
		w_win = MIN(w_win, wdesk - x_win );
		h_win = MIN( h_win, hdesk - y_win );
		x_win = MAX(((MAX(number_of_opened_windows - 1, 1)  << 4) + (wdesk >> 2)) - w_win, 1);
		y_win = MAX(((number_of_opened_windows - 1) << 4) + (hdesk >> 2), 40);
	}
	else {
		w_win = wdesk >> 1;
		h_win = hdesk >> 1;
	}

	wind_set(this_win->wi_handle, WF_WHEEL, 1, 1, 0, 0);

	if(wdesk - (x_win + w_win) < 0){
		x_win = wdesk - w_win;
	}

	if(hdesk - (y_win + h_win) < 0){
		y_win = hdesk - h_win;
	}

	if ( wind_open(this_win->wi_handle, x_win , y_win, w_win, h_win ) ){
		number_of_opened_windows += 1;
	} else {
		form_alert(1, "[1][ERROR WHILE OPENING WINDOW][Okay]");
	}

	wind_get( this_win->wi_handle, WF_WORKXYWH, 
		&this_win->work_area.g_x, &this_win->work_area.g_y, &this_win->work_area.g_w,&this_win->work_area.g_h );

	wind_get( this_win->wi_handle, WF_CURRXYWH, 
		&this_win->ext_area.g_x, &this_win->ext_area.g_y, &this_win->ext_area.g_w, &this_win->ext_area.g_h );

	this_win->previous_work_area.g_h = this_win->work_area.g_h;
	this_win->previous_work_area.g_w = this_win->work_area.g_w;
	this_win->previous_work_area.g_x = this_win->work_area.g_x;
	this_win->previous_work_area.g_y = this_win->work_area.g_y;

	this_win->previous_ext_area.g_h = this_win->ext_area.g_h;
	this_win->previous_ext_area.g_w = this_win->ext_area.g_w;
	this_win->previous_ext_area.g_x = this_win->ext_area.g_x;
	this_win->previous_ext_area.g_y = this_win->ext_area.g_y;

    grect_to_array( &this_win->work_area, this_win->work_pxy);
    wipe_pxy_area(this_win->work_pxy);

	st_Set_Mouse( TRUE );
}

void wipe_pxy_area(int16_t *pxy){
	vsf_interior( st_vdi_handle, 0 );	
	st_Set_Clipping( CLIPPING_ON, pxy );
	vr_recfl( st_vdi_handle, pxy );
	st_Set_Clipping( CLIPPING_OFF, pxy );
}

void st_Set_Clipping(int16_t flag, int16_t *pxy_array){
	switch (flag)
	{
	case TRUE:
		if(clip_status != TRUE){
			vs_clip( st_vdi_handle, flag, pxy_array );
			clip_status = TRUE;
		}
		break;
	case FALSE:
		if(clip_status == TRUE){
			vs_clip( st_vdi_handle, flag, pxy_array );
			clip_status = FALSE;
		}
		break;
	default:
		break;
	}
}

void reorder_struct_window(void){
	int16_t i;
	for( i = 0; i < MAX_WINDOWS - 1; i++ ){
		if(win_struct_array[i].wi_handle == 0 && win_struct_array[i + 1].wi_handle != 0){
			memcpy(&win_struct_array[i], &win_struct_array[i + 1], sizeof(struct_window));
			memset(&win_struct_array[i + 1], 0, sizeof(struct_window));
		}
	}
}

int16_t close_window( int16_t this_win_handle ){
	struct_window *this_win;
	this_win = detect_window( this_win_handle );

	if( this_win != NULL ){
		/* First we close the window */
		if(this_win->wi_form == NULL){
			wind_close(this_win_handle);
		} else {
			form_alert(1, "[1][Error closing this window|A form is in progress|Please close it to continue][Okay]");
			return 0;
		}
/*CONTROl BAR*/
		if(this_win->wi_control_bar != NULL){
			mem_free(this_win->wi_control_bar->control_bar_list);
			mem_free(this_win->wi_control_bar);					
		}
/*FFMPEG VIDEO*/		
		if(this_win->wi_ffmpeg != NULL){
			mem_free(this_win->wi_ffmpeg);				
		}
/*WI_DATA*/
		if( this_win->wi_data != NULL ){
/*FORM*/
			if(this_win->wi_data->rsc_media == TRUE){
				/* It's a form window - We have an RSC file loaded by AES */
				struct_window* this_win_master = detect_window(this_win->wi_data->rsc.win_master_handle);
				this_win_master->wi_form = NULL;
				rsrc_free();
			}

/*THUMB*/
			if(this_win->wi_data->thumbnail_master == TRUE) {
				mem_free((char*)this_win->wi_data->path);
				mem_free((char*)this_win->wi_data->extension);
				
				this_win->wi_thumb->thumb_clean_func(this_win->wi_thumb);
			}

			if(this_win->wi_data->thumbnail_slave == TRUE){
				if(this_win->wi_thumb != NULL){
					struct_window* this_win_master = detect_window(this_win->wi_thumb->master_win_handle);
					this_win->wi_data->img.img_id = NIL;
					if(this_win_master != NULL){
						wind_set(this_win_master->wi_handle,WF_TOP,0,0,0,0);
						this_win_master->wi_thumb->thumbs_selected_nb = this_win->wi_data->img.img_index;
						this_win_master->wi_thumb->thumbs_area_refresh = TRUE;
						st_Start_Window_Process(this_win_master);
						this_win_master->refresh_win(this_win_master->wi_handle);
						st_End_Window_Process(this_win_master);
					}
				}
			}

			if(this_win->wi_data->original_buffer != NULL){
				mem_free( this_win->wi_data->original_buffer );
			}
			if(this_win->wi_data->path != NULL){
				mem_free(this_win->wi_data->path );
			}			
			if(this_win->wi_data->extension != NULL){
				mem_free(this_win->wi_data->extension );
			}		
			/* Shutdown of data struct */
			mem_free( this_win->wi_data );
		}
		/* Free mallocs done previously */
		mfdb_free(&this_win->wi_rendered_mfdb);
		mfdb_free(&this_win->wi_original_mfdb);
		if(this_win->wi_to_display_mfdb != NULL){
			mfdb_free(this_win->wi_to_display_mfdb);
		}
		mem_free(this_win->wi_name);
		if ( wind_delete( this_win_handle ) ){
			number_of_opened_windows -= 1;
		}
		this_win->wi_handle = 0;
		memset(this_win, 0, sizeof(struct_window));
		
		return 1;
	}
	else 
	{
		form_alert(1, "[1][ERROR WHILE CLOSING struct_window][Okay]");
		return 0;
	}
}

void win_refresh_from_buffer(struct_window *this_win){
	if(!this_win->wi_data->rsc_media){
		if(this_win->wi_to_display_mfdb->fd_addr != NULL){
			TRACE(("win_refresh_from_buffer(%d)\n", this_win->wi_handle))
			if(msg_buffer[0] != WM_HSLID && msg_buffer[0] != WM_VSLID){
				win_slider_size(this_win->wi_handle, MIN(this_win->work_area.g_w, this_win->total_length_w), HORIZONTAL_MOVE);
				if(this_win->total_length_w != this_win->work_area.g_w){
					do_hslide(this_win->wi_handle, 1000L * this_win->current_pos_x / (this_win->total_length_w - this_win->work_area.g_w));
				} else {
					do_hslide(this_win->wi_handle, 1000L * this_win->current_pos_x);
				}
				win_slider_size(this_win->wi_handle, MIN(this_win->work_area.g_h, this_win->total_length_h), VERTICAL_MOVE);
				if(this_win->total_length_h != this_win->work_area.g_h){
					do_vslide(this_win->wi_handle, 1000L * this_win->current_pos_y / (this_win->total_length_h - this_win->work_area.g_h));
				} else {
					do_vslide(this_win->wi_handle, 1000L * this_win->current_pos_y);	
				}
			}
			if(msg_buffer[0] != WM_SIZED){
				send_message(this_win->wi_handle, WM_REDRAW);
			}
		}
	}
}

void win_slider_size(int16_t my_win_handle, int16_t length_seen, int16_t direction){
	struct_window * this_win;
	int16_t size;
	int16_t old_size;
	int16_t dummy;
	int16_t total_length;
	this_win = detect_window(my_win_handle);

	if (direction == VERTICAL_MOVE){
		total_length = this_win->total_length_h;
	}else{
		total_length = this_win->total_length_w;
	}
	size = MIN(1000, 1000L * length_seen / total_length);
	if (direction == VERTICAL_MOVE){
		wind_get(my_win_handle, WF_VSLSIZE, &old_size, &dummy, &dummy,&dummy);
		if (old_size != size && size > 0){
			wind_set(my_win_handle, WF_VSLSIZE, size, 0, 0, 0);
		}
	}else{
		wind_get(my_win_handle, WF_HSLSIZE, &old_size, &dummy, &dummy,&dummy);
		if (old_size != size && size > 0){
			wind_set(my_win_handle, WF_HSLSIZE, size, 0, 0, 0);
		}
	}
}

void do_hslide(int16_t my_win_handle, int16_t new_hposition){
	struct_window *this_win;
	this_win = detect_window(my_win_handle);
	int16_t old_position = 1000L * this_win->current_pos_x / (this_win->total_length_w - this_win->work_area.g_w);
	new_hposition = MAX(1, MIN(new_hposition, 1000));
	if(old_position != new_hposition){
		this_win->current_pos_x = MAX(0, (this_win->total_length_w - this_win->work_area.g_w) * (long)new_hposition / 1000L);
	}
	wind_set(my_win_handle, WF_HSLIDE, new_hposition, 0, 0, 0);
}

void do_vslide(int16_t my_win_handle, int16_t new_vposition){
	struct_window *this_win;
	this_win = detect_window(my_win_handle);
	int16_t old_position =  1000L * this_win->current_pos_y / (this_win->total_length_h - this_win->work_area.g_h);
	new_vposition = MAX(1, MIN(new_vposition, 1000));
	if(old_position != new_vposition){
		this_win->current_pos_y = MAX(0, (this_win->total_length_h - this_win->work_area.g_h) * (long)new_vposition / 1000L);
	}
	wind_set(my_win_handle, WF_VSLIDE, new_vposition, 0, 0, 0);
}

void do_arrow(int16_t my_win_handle, int16_t arrow_msg){
	if (arrow_msg == WA_UPPAGE || arrow_msg == WA_DNPAGE || arrow_msg == WA_UPLINE || arrow_msg ==WA_DNLINE){
		do_arrow_y(my_win_handle, arrow_msg);
	} else
	{
		do_arrow_x(my_win_handle, arrow_msg);
	}
}

void do_arrow_y(int16_t my_win_handle, int16_t arrow_msg){
	struct_window *this_win;
	int16_t new_current_pos_y;
	int16_t modulo_posy;	

	this_win = detect_window(my_win_handle);
	switch (arrow_msg)
	{
		case WA_UPPAGE:
			if(this_win->work_area.g_h <= this_win->current_pos_y && this_win->current_pos_y > 0){
				new_current_pos_y = this_win->current_pos_y - this_win->work_area.g_h;
			} else {
				new_current_pos_y = 0;
			}
		break;
		case WA_DNPAGE:
			if (this_win->current_pos_y < this_win->total_length_h - (2 * this_win->work_area.g_h)){
				new_current_pos_y = this_win->current_pos_y + this_win->work_area.g_h;
			} else
			{
				new_current_pos_y = this_win->total_length_h - this_win->work_area.g_h - 1;
			}
		break;
		case WA_UPLINE:
			if(this_win->y_unit < this_win->current_pos_y && this_win->current_pos_y > 0){
				new_current_pos_y = this_win->current_pos_y - this_win->y_unit;
				modulo_posy = (long)new_current_pos_y % this_win->y_unit;
				if(modulo_posy != 0){new_current_pos_y += (this_win->y_unit - modulo_posy);}
			} else {
				new_current_pos_y = 0;
			}
		break;
		case WA_DNLINE:
			if (this_win->current_pos_y < this_win->total_length_h - this_win->work_area.g_h){
				new_current_pos_y = this_win->current_pos_y + this_win->y_unit;
				modulo_posy = (long)new_current_pos_y % this_win->y_unit;
				if(modulo_posy <= this_win->y_unit ){new_current_pos_y += (this_win->y_unit - modulo_posy);}
			} else {
				new_current_pos_y = this_win->current_pos_y;
			}
		break;
		default:
		break;
	}
	this_win->current_pos_y = new_current_pos_y;
}

void do_arrow_x(int16_t my_win_handle, int16_t arrow_msg){
	struct_window *this_win;
	int16_t new_current_pos_x;
	int16_t modulo_posx;

	this_win = detect_window(my_win_handle);
	switch (arrow_msg)
	{
		case WA_LFPAGE:
				new_current_pos_x = MAX(0,this_win->current_pos_x - this_win->work_area.g_w);
		break;
		case WA_RTPAGE:
				new_current_pos_x = MIN(this_win->current_pos_x + this_win->work_area.g_w - this_win->x_unit, (this_win->total_length_w - this_win->work_area.g_w));
		break;										
		case WA_LFLINE:
				new_current_pos_x = MAX(0,this_win->current_pos_x - this_win->x_unit);
		break;
		case WA_RTLINE:
				new_current_pos_x = MIN((this_win->total_length_w - this_win->work_area.g_w  - this_win->x_unit),this_win->current_pos_x + this_win->x_unit);
		break;
		default:
		break;
	}
		modulo_posx = (long)new_current_pos_x % this_win->x_unit;
		if(modulo_posx != 0){new_current_pos_x += modulo_posx;}
		this_win->current_pos_x = new_current_pos_x;
}

void buffer_to_screen(int16_t my_win_handle, GRECT *raster_dest){

	graf_mouse(M_OFF,0L);
	struct_window *this_win;
	this_win = detect_window(my_win_handle);
	update_struct_window(this_win);
	bool wipe = false;

	int16_t clip = 0;
	int16_t xy[8], xy_clip[4];
	int16_t w, h, x, y;
	int16_t clipx, clipy, clipw, cliph;
	int16_t deltax = 0;
	int16_t deltay = 0;

	w = this_win->work_area.g_w;
	h = this_win->work_area.g_h;
	x = this_win->work_area.g_x;
	y = this_win->work_area.g_y;

	if(this_win->wi_data->image_media == TRUE){
		int16_t diff_x = MAX(0, this_win->work_area.g_w - (this_win->total_length_w - this_win->current_pos_x));
		if( diff_x > 0){
			deltax = this_win->current_pos_x - diff_x;
		} else {
			deltax = this_win->current_pos_x;
		}
		int16_t diff_y = MAX(0, this_win->work_area.g_h - (this_win->total_length_h - this_win->current_pos_y));
		if( diff_y > 0){
			deltay = this_win->current_pos_y - diff_y;
		} else {
			deltay = this_win->current_pos_y;
		}
	}

	clipx = raster_dest->g_x;
	clipy = raster_dest->g_y;
	clipw = raster_dest->g_w;
	cliph = raster_dest->g_h;

	if((clipw != 0) && ( cliph != 0)) {
		clip = 1;
		xy_clip[0] = clipx; xy_clip[1] = clipy; xy_clip[2] = clipx + clipw - 1 ; xy_clip[3]= clipy + cliph - 1;
		st_Set_Clipping(1,xy_clip);
	}

	/* Source buffer */
	xy[0] = deltax; xy[1] = deltay; xy[2] = w + deltax - 1; xy[3] = h + deltay - 1;
	/* Destination Buffer */
	xy[4] = x; xy[5] = y; xy[6] = x+w-1; xy[7] = y+h-1;

	if(this_win->wi_data->image_media == TRUE){
		if(this_win->work_area.g_w > this_win->total_length_w){
			wipe = TRUE;
			xy[0] = 0;
			xy[2] = this_win->total_length_w - 1; 
			if(this_win->wi_data->thumbnail_master == FALSE && !this_win->wi_data->video_media){
				xy[4] = x + ((this_win->work_area.g_w - this_win->total_length_w) / 2);
			}else{
				xy[4] = x;
			}
			
			xy[6] = xy[4] + this_win->total_length_w - 1;
		}
		if(this_win->work_area.g_h > this_win->total_length_h){
			wipe = TRUE;
			xy[1] = 0;
			xy[3] = this_win->total_length_h - 1;
			if(this_win->wi_data->thumbnail_master == FALSE && !this_win->wi_data->video_media){
				xy[5] = y + ((this_win->work_area.g_h - this_win->total_length_h) / 2);
			} else {
				xy[5] = y;
			}
			xy[7] = xy[5] + this_win->total_length_h - 1;
		} 
	}

	if(wipe){
	    vr_recfl(st_vdi_handle,this_win->work_pxy);	
    	vsf_interior(st_vdi_handle,0);
	}
	vro_cpyfm(st_vdi_handle, S_ONLY, xy, this_win->wi_to_display_mfdb, &screen_mfdb);

	st_Set_Clipping(0,xy_clip);
	graf_mouse(M_ON,0L);
}

void redraw_window(int16_t my_win_handle){
	GRECT rect;
	int16_t pxy_dest[4];
	struct_window *this_win;
	this_win = detect_window(my_win_handle);

	if(this_win != NULL){
		TRACE(("redraw_window(%d)\n", my_win_handle))
		st_Set_Mouse(FALSE);
		wind_update(BEG_UPDATE);
		wind_get(my_win_handle, WF_FIRSTXYWH, &rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
		while(rect.g_h  != 0 && rect.g_w != 0){
			if ( rc_intersect((GRECT *)&msg_buffer[4], &rect) ){
				grect_to_array(&rect,pxy_dest);
				if(this_win->wi_to_display_mfdb != NULL){
					if(this_win->wi_to_display_mfdb->fd_addr != NULL){
						buffer_to_screen(this_win->wi_handle, &rect);
					}
				}
				if(this_win->wi_data->rsc_media == TRUE){
					objc_draw(this_win->wi_data->rsc.tree, 0, MAX_DEPTH, 
						rect.g_x, rect.g_y, rect.g_w, rect.g_h);
				}
			}
			wind_get(my_win_handle,WF_NEXTXYWH,&rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
		}
		wind_update(END_UPDATE);
		graf_mouse(ARROW,0L);
		st_Set_Mouse( TRUE );
	}
}

void full_size_window(int16_t my_win_handle){
	struct_window *this_win;
	this_win = detect_window(my_win_handle);
	if (
		this_win->ext_area.g_x == this_win->full_ext_area.g_x && 
		this_win->ext_area.g_y == this_win->full_ext_area.g_y && 
		this_win->ext_area.g_w == this_win->full_ext_area.g_w && 
		this_win->ext_area.g_h == this_win->full_ext_area.g_h
		) {
		wind_set(my_win_handle,WF_CURRXYWH,
		this_win->previous_ext_area.g_x,
		this_win->previous_ext_area.g_y,
		this_win->previous_ext_area.g_w,
		this_win->previous_ext_area.g_h
		);
	} else {
		wind_set(my_win_handle,WF_CURRXYWH,
		this_win->full_ext_area.g_x,
		this_win->full_ext_area.g_y,
		this_win->full_ext_area.g_w,
		this_win->full_ext_area.g_h
		);
	}
	update_struct_window(this_win);
	win_slider_size(this_win->wi_handle, MIN(this_win->work_area.g_w, this_win->total_length_w), HORIZONTAL_MOVE);
	do_hslide(this_win->wi_handle, 0);
	win_slider_size(this_win->wi_handle, MIN(this_win->work_area.g_h, this_win->total_length_h), VERTICAL_MOVE);
	do_vslide(this_win->wi_handle, 0);
}

void st_Init_Default_Win(struct_window *this_win){

/* Default is to set everything to FALSE */
    this_win->x_unit = 10; this_win->y_unit = 10;
	this_win->wi_data->video_media = FALSE;
    this_win->wi_data->image_media = FALSE;
	this_win->wi_data->doc_media = FALSE;
	this_win->wi_data->crop_requested = FALSE;
	this_win->wi_data->play_on = FALSE;
	this_win->wi_data->fx_on = FALSE;
	this_win->wi_data->fx_requested = FALSE;
    this_win->wi_data->thumbnail_master = FALSE;
	this_win->wi_data->thumbnail_slave = FALSE;
	this_win->wi_data->control_bar_media = FALSE;
    this_win->wi_data->rsc_media = FALSE;
	this_win->wi_data->autoscale = FALSE;
	this_win->wi_data->resized = FALSE;
	this_win->wi_data->window_size_limited = FALSE;
	this_win->wi_data->wi_buffer_modified = FALSE;
	this_win->wi_data->stop_original_data_load = FALSE;
	this_win->wi_data->wi_pth = NULL;
	this_win->rendering_time = FALSE;
	this_win->refresh_win = NULL;
    this_win->prefers_file_instead_mem = DO_WE_USE_FILE;
   
	this_win->current_pos_x = 0; this_win->current_pos_y = 0;
	this_win->wi_control_bar	= NULL;
	this_win->wi_progress_bar	= NULL;
	this_win->wi_video			= NULL;
	this_win->wi_form			= NULL;
	this_win->wi_thumb			= NULL;
	this_win->wi_crop			= NULL;
	this_win->wi_ffmpeg			= NULL;
	this_win->win_is_topped		= FALSE;
	this_win->total_length_w = 0; this_win->total_length_h = 0;

    /* Dest MFDB. */
    this_win->wi_to_display_mfdb = NULL;
	this_win->wi_to_work_in_mfdb = NULL;
	this_win->wi_original_mfdb.fd_addr = NULL;
	this_win->wi_rendered_mfdb.fd_addr = NULL;
	this_win->wi_buffer_mfdb.fd_addr = NULL;

}

void st_Start_Window_Process(struct_window *this_win){
	if(!this_win->wi_data->video_media){
		st_Set_Mouse( FALSE );
		graf_mouse(BUSY_BEE,0L);
		st_Set_Mouse( TRUE );
	}
}

void st_End_Window_Process(struct_window *this_win){
	
	st_Set_Mouse( FALSE );
	if(!this_win->wi_data->video_media){		
		graf_mouse(ARROW,0L);
	}
	if(this_win != NULL){
    	win_refresh_from_buffer(this_win);
	}
    st_Set_Mouse( TRUE );
}

void st_Limit_Work_Area(struct_window *this_win){
	/* If image size is originaly smaller than the window size then we lock the resizing */
	if( ( this_win->work_area.g_h > MAX(this_win->total_length_h, MIN_WINDOWS_HSIZE) || 
		this_win->work_area.g_w > MAX(this_win->total_length_w, MIN_WINDOWS_WSIZE) )
		// && (!this_win->wi_data->window_size_limited)
	)
	{
		TRACE(("st_Limit_Work_Area(%d)\n", this_win->wi_handle))
		int16_t window_area_buffer[4];
		this_win->work_area.g_h = MIN(this_win->total_length_h, this_win->work_area.g_h);
		this_win->work_area.g_w = MIN(this_win->total_length_w, this_win->work_area.g_w);	
		wind_calc(WC_BORDER,this_win->wi_style, this_win->work_area.g_x, this_win->work_area.g_y, this_win->work_area.g_w, this_win->work_area.g_h, &window_area_buffer[0], &window_area_buffer[1], &window_area_buffer[2], &window_area_buffer[3]);
		wind_set(this_win->wi_handle,WF_CURRXYWH, window_area_buffer[0], window_area_buffer[1], window_area_buffer[2], window_area_buffer[3]);
		send_message(this_win->wi_handle, WM_SIZED);
	}
}

struct_window* get_win_thumb_slave_by_image_id(int16_t master_win_handle, u_int32_t img_id){
    int16_t i = 0;
	struct_window *this_win = NULL;
	while(i < MAX_WINDOWS && number_of_opened_windows > 0){
        if(win_struct_array[i].wi_data != NULL && win_struct_array[i].wi_thumb != NULL){
            if ( win_struct_array[i].wi_data->thumbnail_master != TRUE ){
				// if(!strcmp(win_struct_array[i].wi_data->path, this_path) && win_struct_array[i].wi_data->img.img_id == img_id){
				if( win_struct_array[i].wi_thumb->master_win_handle == master_win_handle && win_struct_array[i].wi_data->img.img_id == img_id){
					this_win = &win_struct_array[i];
					break;
				}
            }
        }
		i++;
	}
    return this_win;
}

struct_window* get_win_thumb_master_by_file(const char* this_path){
    int16_t i = 0;
	struct_window* this_win = NULL;
	while(i < MAX_WINDOWS && number_of_opened_windows > 0){
        if(win_struct_array[i].wi_data != NULL && win_struct_array[i].wi_thumb != NULL){
            if ( win_struct_array[i].wi_data->thumbnail_master == TRUE ){
                if(!strcmp(win_struct_array[i].wi_data->path, this_path)){
                    this_win = &win_struct_array[i];
                    break;
                }
            }
        }
		i++;
	}
	return this_win;
}

/* Pth stuff */
int st_Open_Thread(void* func(void*), void* th_param){
	long ret = 0;
	for(int index = 0; index < NUM_THREADS; ++index){
		if (threads[index] == NULL){
			// printf("Debug - th_param %s", (char*)th_param);
			ret = pthread_create( &threads[index], NULL, func, th_param );
			total_thread += 1;
			return index;
		}
	}
	return ret;
}

/* Wait for execution of each thread */
void st_Wait_For_Threads(){
    for(int index = 0; index < NUM_THREADS; ++index){
		if (threads[index] != NULL){
			if(pthread_join(threads[index],NULL) == 0) {
				total_thread -= 1;
				threads[index] = NULL;
			}
		}   
    }
}

void st_Win_Close_All(void){
	int16_t i = 0;
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle > 0){
			send_message( win_struct_array[i].wi_handle, WM_CLOSED);
		}
	i++;
	}
}

void st_Win_Set_Ready(struct_window* this_win, u_int16_t width, u_int16_t height){
        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;
        this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
        this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
		this_win->wi_data->wi_buffer_modified = FALSE;
}

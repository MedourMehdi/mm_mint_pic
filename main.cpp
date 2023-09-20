#include "headers.h"
#include <stdio.h>
#include <time.h>
#include <mint/cookie.h>
#include <mint/sysvars.h>

#include "utils/utils.h"
#include "windows.h"
#include "file.h"

#include "img_heif/img_heif.h"
#include "img_png/img_png.h"
#include "img_webp/img_webp.h"
#include "img_jpeg/img_jpeg.h"
#include "img_tiff/img_tiff.h"
#include "img_bmp/img_bmp.h"
#include "img_tga/img_tga.h"

#include "img_dummy/img_dummy.h"

#include "png_ico/png_ico.h"

#include "utils_rsc/progress.h"
#include "utils_rsc/winform.h"
#include "rsc_processing/diag.h"
#include "thumbs/thumbs.h"
#include "utils_gfx/crop.h"

boolean exit_call = FALSE;
int16_t st_vdi_handle;
int16_t wchar, hchar, wbox, hbox;
int16_t original_palette[16];
int16_t xdesk, ydesk, wdesk, hdesk;
int16_t min_win_hsize, min_win_wsize;
int16_t work_in[12], work_out[57];

boolean original_palette_saved;

int16_t hrez, wrez;
int16_t number_of_colors;

int16_t screen_workstation_format;
int16_t screen_workstation_bits_per_pixel;

boolean mouse_status = TRUE;
boolean clip_status = FALSE;

struct_window win_struct_array[MAX_WINDOWS] = {0};
int16_t number_of_opened_windows = 0;

int16_t butdown = FALSE;						/* button state tested for UP/DOWN */
int16_t r1_flags;
int16_t	r2_flags;
GRECT r1, r2;
int16_t mx, my; /* Mouse Position */
int16_t mb, mc; /* Mouse button - clicks */
int16_t ks, kc; /* Key state/code */
uint32_t msg_timer = 0L;
int16_t events; /* What events are valid ? */

int16_t msg_buffer[8];

struct_window *selected_window;

char alert_message[96];

/*
COOKIE related section
*/
uint16_t mint_version;
uint8_t computer_type;
uint8_t cpu_type;
u_int16_t tos_version;
bool edDi_present = true;
bool emutos_rom = false;

int16_t vdi_palette[256][3]; /* Set as external in header.h */
int16_t pix_palette[256];
int16_t palette_ori[256] = {0};

int16_t	clock_unit = 5;
u_int32_t time_start;
u_int32_t time_end;
u_int32_t duration;

struct_progress_bar *global_progress_bar;

void *event_loop( void *result);
void* exec_eventloop(void* p_param);
bool init_app();
int16_t st_VDI_Pixel_Format(VdiHdl vdi_handle);
bool new_win_img(const char *new_file);
bool new_win_start();
void st_Win_Print_Dummy(int16_t this_win_handle);
int16_t new_win_form_rsc(const char *new_file, const char* win_title, int16_t object_index);
void exit_app();

int16_t new_win_thumbnails(const char* win_title, int16_t slave_win_handle);
int16_t new_win_crop(struct_crop* this_crop, const char* win_title);

/* Colors definitions for background transarency */
u_int32_t grey_color = 0x7AC0C0C0;

/* 
*	mm_png_lib
*/

/* Functions executed when you click an icon from the control bar */

void* st_Img_Open(void*);
void* st_Img_Windowed(void*);
void* st_Img_Reload(void* p_param);
void* st_Restart(void* p_param);
void* st_Img_Resize(void* p_param);
void* st_Img_Export(void* p_param);
void* st_Img_Crop(void* p_param);
void* st_Img_ZoomIn(void* p_param);
void* st_Img_ZoomOut(void* p_param);
void* st_Img_Rotate(void* p_param);

/* MFDB for each icons */

struct_st_ico_png st_ico_1_mfdb, st_ico_2_mfdb, st_ico_3_mfdb, st_ico_4_mfdb, st_ico_5_mfdb, st_ico_6_mfdb, st_ico_7_mfdb, st_ico_8_mfdb, st_ico_9_mfdb, st_ico_10_mfdb;
struct_st_ico_png st_ico_11_mfdb, st_ico_12_mfdb;
/*	Here you declare an array of struct with :
*		- index: an unique index associated to the icon you want to display in the control bar. Negative index signals the end of the array and must end it
*		- main icon path: must match the icon path, i.e. where your icon is located
*		- mask like icon path: when you click the main icon the mask path is showed - Let it null if no mask icon is needed
*		- main icon mfdb, must be declared before the struct array
*		- mask icon mfdb, must be declared before the struct array
*		- funtion executed when the icon was clicked - void function with a void pointer parameter
*		- x coordonate relative to the control bar
*		- y coordonate relative to the control bar
*		- mask icon status, this is handled by the lib so this value should always be FALSE
*/

struct_st_ico_png_list control_bar_winimage_list[] = {
	{	1,		 "ico24/open.png",		NULL,		&st_ico_1_mfdb,		NULL,		st_Img_Open, 	12,		4 ,		FALSE	},
	{	2,		 "ico24/export.png",		NULL,		&st_ico_2_mfdb,		NULL,		st_Img_Export, 	48,		4 ,		FALSE	},
	{	3,		 "ico24/collapse.png",		"ico24/expand.png",		&st_ico_3_mfdb,		&st_ico_4_mfdb,		st_Img_Windowed, 	80,		4 ,		FALSE	},
	{	4,		 "ico24/reload.png",		NULL,		&st_ico_5_mfdb,		NULL,		st_Img_Reload, 	112,		4 ,		FALSE	},
	{	5,		 "ico24/resize.png",		NULL,		&st_ico_6_mfdb,		NULL,		st_Img_Resize, 	144,		4 ,		FALSE	},
	{	6,		 "ico24/cut.png",		NULL,		&st_ico_7_mfdb,		NULL,		st_Img_Crop, 	176,		4 ,		FALSE	},
	{	7,		 "ico24/rotate.png",		NULL,		&st_ico_8_mfdb,		NULL,		st_Img_Rotate, 	208,		4 ,		FALSE	},
	{	8,		 "ico24/zoom_in.png",		NULL,		&st_ico_9_mfdb,		NULL,		st_Img_ZoomIn, 	240,		4 ,		FALSE	},
	{	9,		 "ico24/zoom_out.png",		NULL,		&st_ico_10_mfdb,		NULL,		st_Img_ZoomOut, 	272,		4 ,		FALSE	},
	{	-1,		NULL,					NULL, 				NULL, 			 		NULL,				NULL,			0,		0 ,		0	},
};

struct_st_ico_png_list control_bar_winstart_list[] = {
	{	1,		 "ico24/open.png",		NULL,		&st_ico_11_mfdb,		NULL,		st_Img_Open, 	12,		4 ,		FALSE	},
	{	2,		 "ico24/cut.png",		NULL,		&st_ico_12_mfdb,		NULL,		st_Img_Crop, 	48,		4 ,		FALSE	},
	{	-1,		NULL,					NULL, 				NULL, 			 		NULL,				NULL,			0,		0 ,		0	},
};

/*	Your control bar declaration */

// struct_st_control_bar my_control_bar; /* I use a malloc inside my init function so no need for me to declare on here */

/* st_Init_WinImage_Control_Bar() - Init the originals values for your control bar in an Init function */

void st_Init_WinImage_Control_Bar(void* p_param);

void st_Init_WinImage_Control_Bar(void* p_param){
	/* depend of your application - I need this in order to get a win_handle linked to this control bar */
	struct_window *this_win = (struct_window*)p_param;
	if( this_win->wi_to_display_mfdb->fd_addr != NULL ){
	this_win->wi_control_bar = (struct_st_control_bar*)mem_alloc(sizeof(struct_st_control_bar));
	/* The array of struct you declared below - It contain indexes, path, etc... */
	this_win->wi_control_bar->control_bar_list = (struct_st_ico_png_list*)mem_alloc(sizeof(control_bar_winimage_list));
	memcpy(this_win->wi_control_bar->control_bar_list, &control_bar_winimage_list, sizeof(control_bar_winimage_list));
	/* A right padding if you want an icon is showed at the opposite of the others i.e. for example main icon to the left but one of them to the right */
	this_win->wi_control_bar->last_ico_padding_right = 72;
	/* When control_bar_h is equal to zero the control bar was hidden - this value represent the height of the control bar */
	this_win->wi_control_bar->control_bar_h = 0;
	/* If you want some transparency filter set transparency to TRUE */
	this_win->wi_control_bar->transparency = TRUE;
	if(cpu_type < 40){
		this_win->wi_control_bar->transparency = FALSE;
		/* Disabling transparency computing on ST */
	}
	this_win->wi_control_bar->background_mfdb = NULL;
	this_win->wi_control_bar->need_to_reload_control_mfdb = TRUE;
	/* Transparency color - ARGB: value 'A' represent the transparecny level */
	this_win->wi_control_bar->transparency_color = grey_color;
	/* VDI handle */
	this_win->wi_control_bar->vdi_handle = &st_vdi_handle;
	/* Screen MFDB - You may obtained it with a declaration like MFDB screen_mfdb = {0}; */
	this_win->wi_control_bar->virtual_screen_mfdb = &screen_mfdb;
	/* We want hide the control bar with the right click */
	this_win->wi_control_bar->force_unhide = FALSE;
	}
}

void st_Init_WinStart_Control_Bar(void* p_param);

void st_Init_WinStart_Control_Bar(void* p_param){
	struct_window *this_win = (struct_window*)p_param;

	// if( this_win->wi_to_display_mfdb->fd_addr != NULL ){

		this_win->wi_control_bar = (struct_st_control_bar*)mem_alloc(sizeof(struct_st_control_bar));
		this_win->wi_control_bar->control_bar_list = (struct_st_ico_png_list*)mem_alloc(sizeof(control_bar_winstart_list));
		memcpy(this_win->wi_control_bar->control_bar_list, &control_bar_winstart_list, sizeof(control_bar_winstart_list));
		this_win->wi_control_bar->last_ico_padding_right = 24;
		this_win->wi_control_bar->control_bar_h = CONTROLBAR_H;
		this_win->wi_control_bar->transparency = FALSE;
		this_win->wi_control_bar->background_mfdb = NULL;
		this_win->wi_control_bar->need_to_reload_control_mfdb = TRUE;
		this_win->wi_control_bar->transparency_color = grey_color;
		this_win->wi_control_bar->vdi_handle = &st_vdi_handle;
		this_win->wi_control_bar->virtual_screen_mfdb = &screen_mfdb;
		this_win->wi_control_bar->force_unhide = TRUE;
	// }
}

/*	
*	st_Control_Bar_Refresh_MFDB() rebuild the MFDB control bar with transprency if it was enabled
*		-	control bar structure declared previously & assiociated to your icons + funtions
*		-	window MFDB, containing the original image buffer
*		-	window elevator x & y values
*		-	window working area width & heigh
*
*	st_Control_Bar_Buffer_to_Screen() render the control bar in the window.
*/

void st_Reload_Control_Bar(struct_window *this_win, struct_st_control_bar* control_bar);

void st_Reload_Control_Bar(struct_window *this_win, struct_st_control_bar* control_bar){
	if( this_win->wi_to_display_mfdb->fd_addr != NULL && this_win->wi_control_bar != NULL ){
		if(this_win->wi_control_bar->control_bar_h > 0){
			if(screen_workstation_bits_per_pixel <= 8 || this_win->work_area.g_x > this_win->total_length_w || this_win->work_area.g_y > this_win->total_length_h || cpu_type < 30){
				control_bar->need_to_reload_control_mfdb = control_bar->st_control_bar_mfdb.fd_w == wrez ? false : true;
				st_Control_Bar_Refresh_Classic(control_bar, wrez, screen_workstation_bits_per_pixel);
			} else {
				st_Control_Bar_Refresh_MFDB(control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);
			}
			st_Control_Bar_Buffer_to_Screen(control_bar, &control_bar->rect_control_bar);
		}
	}
}

/* Custom functions associated to our icons */

void* st_Img_Rotate(void* p_param){
	struct_window*	this_win = (struct_window*)p_param;
	this_win->wi_data->fx_requested = TRUE;
	this_win->wi_data->img.rotate_degree = this_win->wi_data->img.rotate_degree <= 180 ? this_win->wi_data->img.rotate_degree + 90 : 0;
	send_message(this_win->wi_handle, WM_SIZED);
	return NULL;
}

void* st_Img_ZoomIn(void* p_param){
	struct_window*	this_win = (struct_window*)p_param;
	this_win->wi_data->autoscale = FALSE;
	this_win->wi_data->fx_requested = TRUE;
	this_win->wi_data->img.scaled_pourcentage = this_win->wi_data->img.scaled_pourcentage < 100 ? this_win->wi_data->img.scaled_pourcentage + 10 : 100;
	this_win->wi_data->img.scaled_width = (this_win->wi_data->img.original_width * (this_win->wi_data->img.scaled_pourcentage + 100)) / 100;
	this_win->wi_data->img.scaled_height = (this_win->wi_data->img.original_height * (this_win->wi_data->img.scaled_pourcentage + 100)) / 100;
	send_message(this_win->wi_handle, WM_SIZED);
	return NULL;
}

void* st_Img_ZoomOut(void* p_param){
	struct_window*	this_win = (struct_window*)p_param;
	this_win->wi_data->autoscale = FALSE;
	this_win->wi_data->fx_requested = TRUE;
	if(this_win->wi_data->img.rotate_degree !=0){
		this_win->wi_data->fx_requested = TRUE;
	}
	this_win->wi_data->img.scaled_pourcentage = this_win->wi_data->img.scaled_pourcentage > -90 ? this_win->wi_data->img.scaled_pourcentage - 10 : -90;
	this_win->wi_data->img.scaled_width = (this_win->wi_data->img.original_width * (this_win->wi_data->img.scaled_pourcentage + 100)) / 100;
	this_win->wi_data->img.scaled_height = (this_win->wi_data->img.original_height * (this_win->wi_data->img.scaled_pourcentage + 100)) / 100;

	send_message(this_win->wi_handle, WM_SIZED);
	return NULL;
}

void* st_Restart(void* p_param){
	form_alert(1, "[1][System is going to restart][Okay]");
	st_Supexec((int32_t(*)())reset);

	return NULL;
}

void* st_Img_Crop(void* p_param){
	struct_window*	this_win = (struct_window*)p_param;
	this_win->wi_data->crop_requested = true;
	wind_update(BEG_MCTRL);
	graf_mouse(THIN_CROSS,0L);
	return NULL;
}

void* st_Img_Resize(void* p_param){

	struct_window*	this_win_master = (struct_window*)p_param;
	const char*		rsc_file_to_load = "rsc/diag.rsc";
	const char*		window_form_title = "Resize an image";
	int16_t			rsc_object_index = 0;


	if(this_win_master->wi_form == NULL){
		int16_t this_win_form_handle = new_win_form_rsc(rsc_file_to_load, window_form_title , rsc_object_index);
		if(this_win_form_handle == NIL){
			form_alert(1, "[1][Error opening this form|Please get the source code and debug it!][Okay]");
		}
		else{
		struct_window* this_win_form = detect_window(this_win_form_handle);
		this_win_form->wi_data->rsc.win_master_handle = this_win_master->wi_handle;
		this_win_master->wi_form = &this_win_form->wi_data->rsc;
		this_win_form->wi_data->rsc.process_function = &st_Form_Events_Change_Resolution;

		st_Form_Init_Change_Resolution(this_win_form_handle);
		}
	} else { 
		form_alert(1, "[1][There is already a form opened for this window.|Please close it before process an other task][Okay]");
	}
	return NULL;
}

void* st_Img_Export(void* p_param){

	struct_window*	this_win_master = (struct_window*)p_param;
	const char*		rsc_file_to_load = "rsc/diag.rsc";
	const char*		window_form_title = "Export an image";
	int16_t			rsc_object_index = 1;

	if(this_win_master->wi_form == NULL){
		int16_t this_win_form_handle = new_win_form_rsc(rsc_file_to_load, window_form_title , rsc_object_index);
		if(this_win_form_handle == NIL){
			form_alert(1, "[1][Error opening this form|Please get the source code and debug it!][Okay]");
		}
		else{
			struct_window* this_win_form = detect_window(this_win_form_handle);
			this_win_form->wi_data->rsc.win_master_handle = this_win_master->wi_handle;
			this_win_master->wi_form = &this_win_form->wi_data->rsc;
			this_win_form->wi_data->rsc.process_function = &process_diag_export;
		}
	} else { 
		form_alert(1, "[1][There is already a form opened for this window.|Please close it before process an other task][Okay]");
	}
	return NULL;
}

void* st_Img_Open(void* param){
	char final_path[128] = {'\0'};
	char filename[128];
	
	if(file_selector(final_path, (char*)"Open new image", filename) != FALSE){
		new_win_img(final_path);
	}

	return NULL;
}

void* st_Img_Reload(void* param){
	struct_window *this_win = (struct_window*)param;

	this_win->wi_data->needs_refresh = TRUE;
	this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
	this_win->wi_data->fx_on = FALSE;
    this_win->wi_data->img.scaled_pourcentage = 0;
    this_win->wi_data->img.rotate_degree = 0;	
	this_win->refresh_win(this_win->wi_handle);

	send_message(this_win->wi_handle, WM_REDRAW);

	return NULL;
}

void* st_Img_Windowed(void* param){
	struct_window *this_win = (struct_window*)param;
	this_win->wi_data->autoscale = this_win->wi_data->autoscale == TRUE ? FALSE : TRUE;
	this_win->wi_data->img.scaled_pourcentage = 0;
	this_win->refresh_win(this_win->wi_handle);
	send_message(this_win->wi_handle, WM_REDRAW);
	return NULL;
}

/* End for declarations and definitions foR functions associated to our icons */

/* Main */

int main(int argc, char *argv[]){

	char* this_file = (char*)mem_alloc(128);
	memset(this_file, 0, 128);

    if(!init_app()){
		goto quit;
	}
	
	global_progress_bar = st_Progress_Bar_Alloc_Enable();

	if(!st_Ico_PNG_Init(control_bar_winimage_list)){
		goto quit;
	}

	if (argc > 1){
		for(int16_t i = 1; i < argc; i++) {
				strcat(this_file, argv[i]);
				if(i < (argc - 1)){strcat(this_file, " ");}
		}
		remove_quotes(this_file, this_file);
		if(!new_win_img(this_file)){
			goto close_ico_png;
		}
	} else {
		// form_alert(1, "[1][Please provide a file in argument][Bye]");
		// goto close_ico_png;
		if(!st_Ico_PNG_Init(control_bar_winstart_list)){
			goto close_ico_png;
		}
		if(!new_win_start()){
			goto close_ico_png;
		}		
	}

	while (!exit_call) {
		exec_eventloop(NULL);
	}

	st_Ico_PNG_Release(control_bar_winstart_list);
close_ico_png:
	st_Ico_PNG_Release(control_bar_winimage_list);

	st_Progress_Bar_Finish(global_progress_bar);

quit:
	mem_free(this_file);
	exit_app();
    return 0;
}

bool init_app(){
	bool ret = true;
    appl_init();

    st_vdi_handle = graf_handle(&wchar, &hchar, &wbox, &hbox);

    min_win_hsize = MIN_WINDOWS_HSIZE;
	min_win_wsize = MIN_WINDOWS_WSIZE;

    wind_get(0, WF_WORKXYWH, &xdesk, &ydesk, &wdesk, &hdesk);

    work_in[0] = st_Getrez() + 2;
    int16_t i;
    for(i = 0; i < 10; i++){
		work_in[i] = 1;
	}
    work_in[10] = 2;
    v_opnvwk(work_in, &st_vdi_handle, work_out);

    hrez = work_out[1] + 1;
	wrez = work_out[0] + 1;
	number_of_colors = work_out[13];

	vq_extnd(st_vdi_handle,1,work_out);
	screen_workstation_bits_per_pixel = work_out[4];
	screen_workstation_format = st_VDI_Pixel_Format(st_vdi_handle);
	if ( screen_workstation_bits_per_pixel < 1){
		screen_workstation_bits_per_pixel = 1;
	}

	if (screen_workstation_bits_per_pixel < 16){
		st_Save_Pal(palette_ori, 1 << screen_workstation_bits_per_pixel);
		st_VDI_SavePalette_RGB(vdi_palette);
	}

	int32_t cookie_mch, cookie_mint, cookie_cpu, cookie_eddi = 0;
	if(Getcookie(*(int32_t *) "_MCH",&cookie_mch)){
		computer_type = 0;
	} else {
		computer_type = cookie_mch >> 16;
	}
	if(Getcookie(*(int32_t *) "MiNT",&cookie_mint)){
		mint_version = 0;
	} else {
		mint_version = cookie_mint;
	}
	if(mint_version < 0x0100){
		if(st_form_alert_choice(FORM_EXCLAM, (char*)"This app requiers Mint > 1") == 1){
			ret = false;
		}		
	}	
	Getcookie(*(int32_t *) "EdDI", &cookie_eddi);
	if( !cookie_eddi ){
		edDi_present = false;
		if(screen_workstation_bits_per_pixel == 8){
			sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d\nCookie EdDI needed", 
					screen_workstation_format, screen_workstation_bits_per_pixel );
			if(st_form_alert_choice(FORM_EXCLAM, alert_message) == 1){
				ret = false;
			}
		}		
	}
	if(Getcookie(*(int32_t *) "_CF_",&cookie_cpu)){
		if(Getcookie(*(int32_t *) "_CPU_",&cookie_cpu)){
			cpu_type = 0;
		} else {
			cpu_type = cookie_cpu;
		}
	} else {
		cpu_type = 54;
	}

	tos_version = (int)((OSHEADER *)get_sysvar(_sysbase))->os_version;

	if( (*(uint32_t *)&((OSHEADER *)get_sysvar(_sysbase))->p_rsv2) == 0x45544f53){
		emutos_rom = true;
	}

    st_Set_Mouse( FALSE );
	graf_mouse(ARROW,0L);
	st_Set_Mouse( TRUE );
	return ret;
}

int16_t st_VDI_Pixel_Format(VdiHdl vdi_handle){
	/*
	*	Workstation format:
	*		0	-	Interleaved planes, word-wide (ATARI graphic)
	*		1	-	Whole planes (standard format)
	*		2	-	Packed pixels
	*		-1	-	Unknown format
	*/
	int16_t screen_vdi_info[272];
	vq_scrninfo(vdi_handle, screen_vdi_info);
	memcpy ( pix_palette, screen_vdi_info + 16, 512 );
	return screen_vdi_info[0];
}

void exit_app(){
	rsrc_free();
	v_clsvwk(st_vdi_handle);
	appl_exit();
}

void* exec_eventloop(void* p_param){
	while ( exit_call != 1 )
	{
		event_loop(NULL);
	}
	return NULL;
}

void *event_loop(void *result)
{
	events = evnt_multi(
		MU_MESAG|MU_BUTTON|MU_KEYBD,
		256 | 2, 3, butdown, /* button state tested for UP/DOWN */
		r1_flags, r1.g_x, r1.g_y, r1.g_w, r1.g_h, /* M1 event */
		r2_flags, r2.g_x, r2.g_y, r2.g_w, r2.g_h, /* M2 event */
		msg_buffer, /* Pointer to msg */
		0,
		&mx, &my, &mb,
		&ks, &kc,
		&mc /* Single/double clicks */
		);
	if( events & MU_MESAG )
	{
		switch (msg_buffer[0])
		{
		case 0x4711:
			new_win_img(*((char **)&msg_buffer[3]));
			break;
		case AP_TERM:	
			break;
		case MN_SELECTED:
			break;
		case WM_REDRAW:
			if(number_of_opened_windows > 0 && exit_call != 1 && msg_buffer[3] > 0 ){
				selected_window = detect_window(msg_buffer[3]);
				if(selected_window != NULL){
					redraw_window(msg_buffer[3]);
					if(selected_window->wi_control_bar != NULL){
						if(selected_window->wi_control_bar->control_bar_h > 0 && selected_window->wi_to_display_mfdb->fd_addr != NULL){
							st_Control_Bar_Redraw( selected_window->wi_control_bar, msg_buffer[3]);
						}
					}
				}
			}
			break;
		case WM_ONTOP:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				selected_window->win_is_topped = TRUE;
			}
			wind_set(msg_buffer[3],WF_TOP,0,0,0,0);
			break;
		case WM_BOTTOM :
			wind_set(msg_buffer[3],WF_BOTTOM,0,0,0,0);
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){ selected_window->win_is_topped = FALSE;	}
			break ;
		case WM_UNTOPPED:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){ selected_window->win_is_topped = FALSE;	}
			break;
		case WM_TOPPED:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				selected_window->win_is_topped = TRUE;
			}
			wind_set(msg_buffer[3],WF_TOP,0,0,0,0);
			break;
		case WM_NEWTOP:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				selected_window->win_is_topped = TRUE;
			}
			wind_set(msg_buffer[3],WF_TOP,0,0,0,0);
			break;
		case WM_FULLED:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				full_size_window(selected_window->wi_handle);
				selected_window->refresh_win(selected_window->wi_handle);
				if(selected_window->wi_control_bar != NULL){
					st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
				}
			}
			break;
		case WM_SIZED:
			int16_t window_area_buffer[4];
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				wind_set(msg_buffer[3],WF_TOP,0,0,0,0);
				msg_buffer[6] = MAX(msg_buffer[6], min_win_wsize);
				msg_buffer[7] = MAX(msg_buffer[7], min_win_hsize);
				wind_calc(WC_WORK,selected_window->wi_style, msg_buffer[4], msg_buffer[5], msg_buffer[6], msg_buffer[7],&window_area_buffer[0],&window_area_buffer[1],&window_area_buffer[2],&window_area_buffer[3]);
				if(selected_window->wi_data->window_size_limited == TRUE && selected_window->wi_data->autoscale == FALSE){
					if(selected_window->wi_data->thumbnail_master == TRUE){
						window_area_buffer[2] = MIN(selected_window->wi_thumb->thumbs_max_area_w, window_area_buffer[2]);
						window_area_buffer[3] = MIN(selected_window->wi_thumb->thumbs_max_area_h, window_area_buffer[3]);
					} else if(selected_window->wi_data->control_bar_media == TRUE){
						window_area_buffer[2] = MIN(selected_window->total_length_w, window_area_buffer[2]);
						window_area_buffer[3] = MIN(selected_window->total_length_h, window_area_buffer[3]);
					}else {
						window_area_buffer[2] = MAX( MIN(selected_window->total_length_w, window_area_buffer[2]), MIN_WINDOWS_WSIZE);
						window_area_buffer[3] = MAX( MIN(selected_window->total_length_h, window_area_buffer[3]), MIN_WINDOWS_HSIZE);
					}
				}
				wind_calc(WC_BORDER,selected_window->wi_style, window_area_buffer[0], window_area_buffer[1], window_area_buffer[2], window_area_buffer[3],&window_area_buffer[0],&window_area_buffer[1],&window_area_buffer[2],&window_area_buffer[3]);
				wind_set(msg_buffer[3],WF_CURRXYWH, window_area_buffer[0], window_area_buffer[1], window_area_buffer[2], window_area_buffer[3]);
				update_struct_window(selected_window);

				
				if(selected_window->wi_data->thumbnail_master != TRUE){
					selected_window->refresh_win(selected_window->wi_handle);
				}

				if(selected_window->wi_control_bar != NULL){
					st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
				}
				if(selected_window->wi_data->thumbnail_master == TRUE){
					if(selected_window->wi_thumb->thumbs_cols != selected_window->wi_thumb->thumb_w_Item / selected_window->work_area.g_w){
						selected_window->refresh_win(selected_window->wi_handle);
					}
				}
			}
			break;
		case WM_MOVED:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				if ( msg_buffer[4] + msg_buffer[6] > xdesk + wdesk){ msg_buffer[4] = xdesk + wdesk - msg_buffer[6]; }
				if ( msg_buffer[5] + msg_buffer[7] > ydesk + hdesk){ msg_buffer[5] = ydesk + hdesk - msg_buffer[7];	}
				wind_set(msg_buffer[3],WF_CURRXYWH, msg_buffer[4], msg_buffer[5], msg_buffer[6], msg_buffer[7]);
				update_struct_window(selected_window);
				if(selected_window->wi_to_display_mfdb != NULL){
					if(selected_window->wi_data->thumbnail_master == TRUE){
						st_Thumb_Desk_PXY_Update(selected_window->wi_thumb, selected_window->work_pxy);
					}else{
						st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
					}
					
				} else {
					if(selected_window->wi_data->rsc_media == TRUE){
						selected_window->refresh_win(msg_buffer[3]);
					}
				}
			}
			break;
		case WM_VSLID:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				do_vslide(msg_buffer[3], msg_buffer[4]);
				selected_window->refresh_win(msg_buffer[3]);
				st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
			}
			break;
		case WM_HSLID:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				do_hslide (msg_buffer[3], msg_buffer[4]);
				selected_window->refresh_win(msg_buffer[3]);
				st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
			}
			break;
		case WM_ARROWED:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				do_arrow(msg_buffer[3], msg_buffer[4]);
				selected_window->refresh_win(msg_buffer[3]);
				st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
			}
			break;
		case WM_CLOSED:
			selected_window = detect_window(msg_buffer[3]);

			if(selected_window != NULL){
				if(close_window(selected_window->wi_handle)){

					if (number_of_opened_windows > 0){
						// reorder_struct_window(); /* fix me */
					}
					else{ 
						exit_call = TRUE; 
					}
				}
			}
			break;
		case WM_ICONIFY:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				wind_get(msg_buffer[3],WF_CURRXYWH,&selected_window->previous_ext_area.g_x,&selected_window->previous_ext_area.g_y,&selected_window->previous_ext_area.g_w,&selected_window->previous_ext_area.g_h);
				wind_set(msg_buffer[3],WF_ICONIFY,msg_buffer[4],msg_buffer[5],msg_buffer[6],msg_buffer[7]);
			}
			break;
		case WM_UNICONIFY:
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				wind_set(msg_buffer[3], WF_UNICONIFY, selected_window->previous_ext_area.g_x,selected_window->previous_ext_area.g_y,selected_window->previous_ext_area.g_w,selected_window->previous_ext_area.g_h);
			}
			break;
		default:
			break;
		}
	}
	if (events & MU_BUTTON)
	{	
		int16_t this_win_handle, dummy;
		wind_get(0,WF_TOP,&this_win_handle,&dummy,&dummy,&dummy);
		selected_window = detect_window(this_win_handle);

		if(selected_window != NULL){

			if(selected_window->wi_data->crop_requested == true && butdown == false){

				if(st_Crop_Finish(selected_window->wi_handle, mx, my)){
					time_t t = time(NULL);
					struct tm tm = *localtime(&t);
					char this_time[24] = {'\0'};
					sprintf(this_time, "crop_%d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		
					new_win_crop(selected_window->wi_crop, (const char*)this_time);
				}
			}

			if(selected_window->wi_data->rsc_media == TRUE){
				st_Form_Handle(selected_window);
			}
			else if(selected_window->wi_to_display_mfdb != NULL){
				if( mb && selected_window->work_area.g_x < mx < selected_window->work_area.g_w && selected_window->work_area.g_y < my < selected_window->work_area.g_h ){
					wind_set(this_win_handle,WF_TOP,0,0,0,0);
					switch (mb)
					{
					case 2:
						if(selected_window->wi_control_bar != NULL && !selected_window->wi_control_bar->force_unhide){
							selected_window->wi_control_bar->control_bar_h = selected_window->wi_control_bar->control_bar_h > 0 ? 0 : CONTROLBAR_H;
							st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
							wipe_pxy_area(selected_window->work_pxy);
							win_refresh_from_buffer(selected_window);
						}
						break;
					case 1:
						if(selected_window->wi_control_bar != NULL){
							if(selected_window->wi_control_bar->control_bar_h > 0){
								st_Control_Bar_PNG_Handle(mx, my, mb, selected_window->wi_control_bar, (void *)selected_window);
							}
						}
						if(selected_window->wi_thumb != NULL && selected_window->wi_data->thumbnail_master == TRUE){
							st_Handle_Click_Thumbnail(selected_window, mx, my, mb);
						}
						break;
					default:
						break;
					}
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
				}
			}
		}
	}
	if(events & MU_KEYBD){
		int16_t this_win_handle, dummy;
		wind_get(0,WF_TOP,&this_win_handle,&dummy,&dummy,&dummy);
		selected_window = detect_window(this_win_handle);
		if(selected_window != NULL){
			if(selected_window->wi_data->rsc_media == TRUE){
				st_Form_Handle(selected_window);
			}else{
				switch (kc)
				{
				case 0x4800:
					/* up arrow */
					do_arrow(this_win_handle, WA_UPLINE);
					selected_window->refresh_win(this_win_handle);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					break;
				case 0x5000:
					/* dn arrow */
					do_arrow(this_win_handle, WA_DNLINE);
					selected_window->refresh_win(this_win_handle);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					break;
				case 0x4D00:
					/* right arrow */
					do_arrow(this_win_handle, WA_RTLINE);
					selected_window->refresh_win(this_win_handle);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					break;
				case 0x4B00:
					/* left arrow */
					do_arrow(this_win_handle, WA_LFLINE);
					selected_window->refresh_win(this_win_handle);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					break;														
				default:
					break;
				}
			}
		}
	}
	return NULL;
}

bool new_win_img(const char *new_file){
	int16_t i = 0;
	u_int32_t start_time, end_time;
	const char *file_extension;
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle == 0){
			win_struct_array[i].wi_style = WIN_STYLE_IMG;
			if(win_struct_array[i].wi_data == NULL){
				/* Init wi_data structure */
				win_struct_array[i].wi_data = (struct_metadata *)mem_alloc(sizeof(struct_metadata));
				win_struct_array[i].wi_data->path = NULL;
				/* Fill window title structure */
				char* file = basename(new_file);
				win_struct_array[i].wi_name = (char *)mem_alloc(sizeof(file) + 5);
				strcpy(win_struct_array[i].wi_name, file);
				/* Init window structure to default values */
				st_Init_Default_Win(&win_struct_array[i]);
				/* Manage if the file is already opened and have more than one picture */
				struct_window* win_master_thumb = get_win_thumb_master_by_file(new_file);
				if(win_master_thumb != NULL){
					win_struct_array[i].wi_data->original_buffer = win_master_thumb->wi_data->original_buffer;
					win_struct_array[i].wi_data->path = (char*)mem_alloc(strlen(win_master_thumb->wi_data->path) + 1);
					strcpy((char*)win_struct_array[i].wi_data->path, win_master_thumb->wi_data->path);
					win_struct_array[i].wi_data->file_lock = win_master_thumb->wi_data->file_lock;
					win_struct_array[i].wi_data->extension = win_master_thumb->wi_data->extension;
					win_struct_array[i].prefers_file_instead_mem = win_master_thumb->prefers_file_instead_mem;
					
					win_struct_array[i].wi_data->img.img_id = win_master_thumb->wi_data->img.img_id;
					win_struct_array[i].wi_data->img.img_index = win_master_thumb->wi_data->img.img_index;
					
					memcpy (&win_struct_array[i].wi_data->STAT_FILE, &win_master_thumb->wi_data->STAT_FILE, sizeof(win_master_thumb->wi_data->STAT_FILE));

					win_struct_array[i].wi_thumb = win_master_thumb->wi_thumb;

					char this_thumb_id[4];
					sprintf(win_struct_array[i].wi_name, "%s #%d",win_struct_array[i].wi_name,  win_struct_array[i].wi_data->img.img_index);

					open_window(&win_struct_array[i]);
				} else {
					open_window(&win_struct_array[i]);
					if(!open_file(&win_struct_array[i], new_file)){
						return false;
					}
				}
				file_extension = win_struct_array[i].wi_data->extension;
				if (check_ext(file_extension, "HEI") || check_ext(file_extension, "HEIF") || check_ext(file_extension, "HEIC") ){
					st_Init_HEIF(&win_struct_array[i]);
				} else if (check_ext(file_extension, "PNG")){
					st_Init_PNG(&win_struct_array[i]);
				} else if (check_ext(file_extension, "WEB") || check_ext(file_extension, "WEBP")){
					st_Init_WEBP(&win_struct_array[i]);
				} else if (check_ext(file_extension, "JPG") || check_ext(file_extension, "JPEG") || check_ext(file_extension, "JPE")){
					st_Init_JPEG(&win_struct_array[i]);
				} else if (check_ext(file_extension, "TIF") || check_ext(file_extension, "TIFF")){
					st_Init_TIFF(&win_struct_array[i]);
				} else if (check_ext(file_extension, "BMP")){
					st_Init_BMP(&win_struct_array[i]);
				} else if (check_ext(file_extension, "TGA")){
					st_Init_TGA(&win_struct_array[i]);
				}

				else {
					form_alert(1, "[1][Wrong file extension][Okay]");
					return false;
				}
				if(win_struct_array[i].prefers_file_instead_mem != TRUE){
					if(!file_to_memory(&win_struct_array[i])){
						form_alert(1, "[1][File to Mem error][Okay]");
						return false;						
					}
				}
				else{
					win_struct_array[i].wi_data->original_buffer = NULL;
				}
				
                st_Set_Clipping(CLIPPING_ON, win_struct_array[i].work_pxy);

                if(win_struct_array[i].rendering_time == FALSE){
                    start_time = st_Supexec(get200hz);
                }
                win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);
                if(win_struct_array[i].rendering_time == FALSE){
                    end_time = st_Supexec(get200hz);
                    win_struct_array[i].rendering_time = (end_time - start_time) * 5;
                }
                st_Set_Clipping(CLIPPING_OFF, win_struct_array[i].work_pxy);

				st_Init_WinImage_Control_Bar((void*)&win_struct_array[i]);

				if(win_struct_array[i].wi_data->thumbnail_slave == TRUE){
					char* file = basename(win_struct_array[i].wi_data->path);
					char thumbs_title[strlen(file) + 16] = {0};
					sprintf(thumbs_title, "%s (%d elements)", file, win_struct_array[i].wi_data->img.img_total);
					new_win_thumbnails(thumbs_title, win_struct_array[i].wi_handle);
				}
				win_struct_array[i].wi_data->thumbnail_slave = TRUE;

			}
			wind_set(win_struct_array[i].wi_handle,WF_TOP,0,0,0,0);
			win_struct_array[i].win_is_topped = TRUE;
			send_message(win_struct_array[i].wi_handle, WM_REDRAW);
			return true;
		}
		i++;
	}
	return false;
}

bool new_win_start(){
	int16_t i = 0;
	
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle == 0){
			win_struct_array[i].wi_style = WIN_STYLE_FORM;
			if(win_struct_array[i].wi_data == NULL){
				win_struct_array[i].wi_data = (struct_metadata *)mem_alloc(sizeof(struct_metadata));
				st_Init_Default_Win(&win_struct_array[i]);
				win_struct_array[i].wi_name = (char *)mem_alloc(WINDOW_TITLE_MAXLEN);
				strcpy(win_struct_array[i].wi_name, "MM PIC");
				win_struct_array[i].wi_data->control_bar_media = TRUE;
                open_window(&win_struct_array[i]);

				st_Init_Dummy(&win_struct_array[i]);

				st_Init_WinStart_Control_Bar((void*)&win_struct_array[i]);

				win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);

			}
			return true;
		}
		i++;
	}
	return false;
}

int16_t new_win_form_rsc(const char *new_file, const char* win_title, int16_t object_index){
	int16_t i = 0;
	
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle == 0){
			win_struct_array[i].wi_style = WIN_STYLE_FORM;
			if(win_struct_array[i].wi_data == NULL){
				win_struct_array[i].wi_data = (struct_metadata *)mem_alloc(sizeof(struct_metadata));
				st_Init_Default_Win(&win_struct_array[i]);
				win_struct_array[i].wi_data->rsc.rsc_file = new_file;
				if(rsc_already_loaded(new_file) == false){
					if(!rsrc_load(win_struct_array[i].wi_data->rsc.rsc_file)){
						form_alert(1, "[1][new_win_form_rsc -> RSC Error][Okay]");
						return NIL;
					}
				}
				rsrc_gaddr(R_TREE, object_index, &win_struct_array[i].wi_data->rsc.tree);
				win_struct_array[i].wi_name = (char *)mem_alloc(WINDOW_TITLE_MAXLEN);
				strcpy(win_struct_array[i].wi_name, win_title);
				st_Init_Form((void*)&win_struct_array[i]);
                open_window(&win_struct_array[i]);
				win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);
			}
			return win_struct_array[i].wi_handle;
		}
		i++;
	}
	return NIL;
}

int16_t new_win_thumbnails(const char* win_title, int16_t slave_win_handle){

	struct_window *dest_win;
	dest_win = detect_window(slave_win_handle);
    if(dest_win == NULL){
        return NIL;
    }

	int16_t i = 0;
	
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle == 0){
			win_struct_array[i].wi_style = WIN_STYLE_THUMBS;
			if(win_struct_array[i].wi_data == NULL){
				win_struct_array[i].wi_data = (struct_metadata *)mem_alloc(sizeof(struct_metadata));
				st_Init_Default_Win(&win_struct_array[i]);
				win_struct_array[i].wi_name = (char *)mem_alloc(WINDOW_TITLE_MAXLEN);
				strcpy(win_struct_array[i].wi_name, win_title);

				win_struct_array[i].wi_data->original_buffer = dest_win->wi_data->original_buffer;

				win_struct_array[i].wi_data->path = (const char*)mem_alloc(strlen(dest_win->wi_data->path));
				strcpy((char*)win_struct_array[i].wi_data->path, dest_win->wi_data->path);

				win_struct_array[i].wi_data->extension = (const char*)mem_alloc(strlen(dest_win->wi_data->extension));
				strcpy((char*)win_struct_array[i].wi_data->extension, dest_win->wi_data->extension);

				win_struct_array[i].wi_data->file_lock = dest_win->wi_data->file_lock;
				memcpy(&win_struct_array[i].wi_data->STAT_FILE, &dest_win->wi_data->STAT_FILE, sizeof (dest_win->wi_data->STAT_FILE));

				win_struct_array[i].wi_data->image_media = TRUE;
				win_struct_array[i].wi_data->window_size_limited = TRUE;
				win_struct_array[i].wi_data->thumbnail_master = TRUE;

				win_struct_array[i].wi_thumb = dest_win->wi_thumb;
				win_struct_array[i].wi_thumb->wi_original_thumbs_mfdb = NULL;
				
				win_struct_array[i].wi_thumb->thumbs_area_refresh = TRUE;
				win_struct_array[i].wi_thumb->master_win_handle = 0;
				
				win_struct_array[i].refresh_win = st_Thumb_Refresh;

				win_struct_array[i].wi_to_display_mfdb = (MFDB*)st_Thumb_MFDB_Update((void*)dest_win->wi_thumb);

				win_struct_array[i].total_length_w = dest_win->wi_thumb->thumbs_area_w;
				win_struct_array[i].total_length_h = dest_win->wi_thumb->thumbs_area_h;
				
				// win_struct_array[i].wi_thumb->thumbs_area_refresh = TRUE;

                open_window(&win_struct_array[i]);

				win_struct_array[i].wi_thumb->master_win_handle = win_struct_array[i].wi_handle;
				win_struct_array[i].wi_thumb->open_win_func = &new_win_img;

				win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);
			}
			return win_struct_array[i].wi_handle;
		}
		i++;
	}
	return NIL;
}

int16_t new_win_crop(struct_crop* this_crop, const char* win_title){
	int16_t i = 0;
	
	while(i < MAX_WINDOWS){
		if(win_struct_array[i].wi_handle == 0){
			win_struct_array[i].wi_style = WIN_STYLE_IMG;
			if(win_struct_array[i].wi_data == NULL){
				win_struct_array[i].wi_data = (struct_metadata *)mem_alloc(sizeof(struct_metadata));
				st_Init_Default_Win(&win_struct_array[i]);
				win_struct_array[i].wi_crop = this_crop;

				memcpy(&win_struct_array[i].wi_original_mfdb, this_crop->wi_crop_original, sizeof(MFDB));

				st_Init_Crop(&win_struct_array[i]);

				win_struct_array[i].wi_to_display_mfdb = &win_struct_array[i].wi_original_mfdb;
				win_struct_array[i].total_length_w = win_struct_array[i].wi_original_mfdb.fd_w;
				win_struct_array[i].total_length_h = win_struct_array[i].wi_original_mfdb.fd_h;				

				win_struct_array[i].wi_name = (char *)mem_alloc(WINDOW_TITLE_MAXLEN);
				strcpy(win_struct_array[i].wi_name, win_title);
				win_struct_array[i].wi_data->path = (const char *)mem_alloc(strlen(win_title) + 15);
				strcpy((char*)win_struct_array[i].wi_data->path, win_title);

                open_window(&win_struct_array[i]);

				st_Init_WinImage_Control_Bar((void*)&win_struct_array[i]);

				win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);

				win_struct_array[i].wi_data->wi_original_modified = TRUE;
				// wind_set(win_struct_array[i].wi_handle,WF_TOP,0,0,0,0);
				// win_struct_array[i].win_is_topped = TRUE;
				// send_message(win_struct_array[i].wi_handle, WM_REDRAW);	
			}
			return win_struct_array[i].wi_handle;
		}
		i++;
	}
	return NIL;
}

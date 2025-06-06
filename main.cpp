#include "headers.h"
#include <stdio.h>
#include <time.h>
#include <mint/cookie.h>
#include <mint/sysvars.h>

#include <pthread.h>

#include "utils/utils.h" /* mem_alloc */
#include "windows.h"

#include "png_ico/png_ico.h" /* For control bar refresh functions */
#include "utils_rsc/winform.h" /* st_Form_Handle */
#include "thumbs/thumbs.h" /* st_Thumb_Desk_PXY_Update + st_Handle_Click_Thumbnail */
#include "utils_gfx/crop.h" /* st_Crop_Finish */
#include "new_window.h" /* Opening new windows */
#include "control_bar.h" /* Init control bar icons MFDB*/

#include "utils/dragdrop.h"

#include "rsc_processing/progress_bar.h"

boolean exit_call = FALSE;
int16_t st_vdi_handle;
int16_t wchar, hchar, wbox, hbox;

int16_t xdesk, ydesk, wdesk, hdesk;
int16_t work_in[12], work_out[57];

int16_t hrez, wrez;

int16_t screen_workstation_format;
int16_t screen_workstation_bits_per_pixel;
bool reverse_bits;

int16_t butdown = FALSE;						/* button state tested for UP/DOWN */
int16_t r1_flags;
int16_t	r2_flags;
GRECT r1, r2;
int16_t mx, my; /* Mouse Position */
int16_t mb, mc; /* Mouse button - clicks */
int16_t ks, kc; /* Key state/code */
u_int32_t msg_timer = 0L;
int16_t events; /* What events are valid ? */
long event_timer_default = 200L;
long event_timer_video = 8L;
// long event_timer_video = 1L;
long event_timer_used = event_timer_default;
// long event_timer_used = event_timer_video;
// long event_timer_used = 0L;
int16_t msg_buffer[8];

struct_window *selected_window;

char alert_message[96];
/*
COOKIE related section
*/
#define SND_GSXB	(1<<5)
#define SND_MILANBLASTER	0x00000024
u_int16_t mint_version;
u_int8_t computer_type;
/*
High	Low	Type
0x0000	0x0000	Atari ST (260 ST,520 ST,1040 ST,Mega ST,...)
0x0000	0x4D34	Medusa T40 without SCSI
0x0001	0x0000	Atari STE (1040 STE, ST Book, STylus/STPad)
0x0001	0x0010	Mega STE
0x0001	0x0100	Sparrow (Falcon pre-production machine)
0x0002	0x0000	Atari TT or Hades
0x0002	0x4D34	Medusa T40 with SCSI
0x0003	0x0000	Atari-Falcon030
0x0004	0x0000	Milan
0x0005	0x0000	ARAnyM >=v0.8.5beta

#define ATARI_ST 0x00000000
#define MEDUSA_T40_WITHOUT_SCSI 0x00004D34
#define MEDUSA_T40_WITH_SCSI 0x00004D34
#define ATARI_STE 0x00010000
#define ATARI_MEGA_STE 0x00010010

*/
u_int8_t cpu_type;
u_int16_t tos_version;
bool edDi_present = true;
bool emutos_rom = false;
// bool gsxb_present = false;
bool milanblaster_present = false;

int16_t vdi_palette[256][3]; /* Set as external in header.h */
int16_t pix_palette[256];
int16_t palette_ori[256] = {0};

char current_path[256] = {'\0'};

char *pfile, *va_file;

#ifdef USE_LNX_PATH
bool path_to_lnx = TRUE;
#else
bool path_to_lnx = FALSE;
#endif

bool use_cached_icons = FALSE;

void *event_loop( void *result);
void* exec_eventloop(void* p_param);
bool init_app();
int16_t st_VDI_Pixel_Format(VdiHdl vdi_handle);
void exit_app();

/* Main */

int main(int argc, char *argv[]){

	struct_win_progress_bar* this_progress_bar = NULL;

	char* this_file = (char*)mem_alloc(256);

	memset(this_file, 0, 256);

	void* func_param;

	st_Get_App_Dir(current_path, argv[0]);

    if(!init_app()){ goto quit;	}

	this_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(NIL, "Initializing application...", 10,  "Checking Gem & VDI values");

	st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 30, "Caching Images Control Bar Icons");

	if(!st_Ico_PNG_Init_Image()){ goto close_ico_image;	}

	st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 50, "Caching Documents Control Bar Icons");

	if(!st_Ico_PNG_Init_Document()){ goto close_ico_document; }

	st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 70, "Caching Video Control Bar Icons");

	if(!st_Ico_PNG_Init_Video()){ goto close_ico_video; }

	st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 90, "Caching Main Control Bar Icons");

	if(!st_Ico_PNG_Init_Main()){goto close_ico_main;}

	st_Open_Thread(&new_win_start_threaded, NULL);

	st_Win_Progress_Bar_Finish(this_progress_bar->win_form_handle);

	// st_Open_Thread(&exec_eventloop, NULL); /* Work here if Timer event is used */

	if (argc > 1){
		for(int16_t i = 1; i < argc; i++) {
			strcat(this_file, argv[i]);
			TRACE(("Arg %d / %s\n", i, argv[i]))
			if(i < (argc - 1)){strcat(this_file, " ");}
		}

		pfile = this_file;
		do {
			va_file = (char*)mem_alloc(128);
			memset(va_file, 0, 128);
			pfile = GetNextVaStartFileName( pfile, va_file ) ;
			if(st_FileExistsAccess(va_file)){
				// printf("Open %s\n", va_file);
				new_win_img(va_file);
			}else{
				sprintf(alert_message, "File not found %s\n", va_file);
				st_form_alert(FORM_EXCLAM, alert_message);
			}
			mem_free(va_file);
		} while ( pfile ) ;
	} 
	// else {
	// 	st_Open_Thread(&new_win_start_threaded, NULL);
	// }
	/* Uncomment if you don't want the drag and drop main bar */
	/* Don't forget to comment on the calls above */
	/*
	else {
		if(!st_Ico_PNG_Init_Main()){goto close_ico_main;}
			st_Open_Thread(&new_win_start_threaded, NULL);
	}	
	*/

	st_Open_Thread(&exec_eventloop, NULL);

	while(total_thread > 0){
		pthread_yield_np();
		st_Wait_For_Threads();
	}

close_ico_main:
	st_Ico_PNG_Release_Main();
close_ico_video:
	st_Ico_PNG_Release_Video();
close_ico_document:
	st_Ico_PNG_Release_Document();
close_ico_image:
	st_Ico_PNG_Release_Image();

quit:
	mem_free(this_file);
	exit_app();
    return 0;
}

bool init_app(){
	bool ret = true;
	TRACE(("appl_init()\n"))

    int app_id = appl_init();
	
    st_vdi_handle = graf_handle(&wchar, &hchar, &wbox, &hbox);

    wind_get(0, WF_WORKXYWH, &xdesk, &ydesk, &wdesk, &hdesk);

    work_in[0] = st_Getrez() + 2;
    int16_t i;
    for(i = 0; i < 10; i++){
		work_in[i] = 1;
	}
    work_in[10] = 2;
    v_opnvwk(work_in, &st_vdi_handle, work_out);

    hrez = work_out[1] + 1;
	TRACE(("hrez %d\n",hrez))
	wrez = work_out[0] + 1;
	TRACE(("wrez %d\n",wrez))

	vq_extnd(st_vdi_handle,1,work_out);
	screen_workstation_bits_per_pixel = work_out[4];
	screen_workstation_format = st_VDI_Pixel_Format(st_vdi_handle);

	if ( screen_workstation_bits_per_pixel < 1){
		screen_workstation_bits_per_pixel = 1;
	// }else if(screen_workstation_bits_per_pixel < 16){
	} else {
		st_Save_Pal(palette_ori, MIN(1 << screen_workstation_bits_per_pixel, 256));
		st_VDI_SavePalette_RGB(vdi_palette);
	}
	// }else{
	// 	st_Save_Pal(palette_ori, 256);
	// 	st_VDI_SavePalette_RGB(vdi_palette);
	// }

	long cookie_mch, cookie_mint, cookie_cpu, cookie_eddi = 0, cookie_snd;
	if(Getcookie(*(long *) "_MCH",&cookie_mch)){
		computer_type = 0;
	} else {
		computer_type = cookie_mch >> 16;
	}
	if(Getcookie(*(long *) "MiNT",&cookie_mint)){
		mint_version = 0;
	} else {
		mint_version = cookie_mint;
		char app_name_string[16] = {'\0'};
		strcpy(app_name_string, "  ");
		strcat(app_name_string, THIS_APP_NAME);
		menu_register( app_id, app_name_string );
	}
	if(mint_version < 0x0100){
		if(st_form_alert_choice(FORM_EXCLAM, (char*)"This app requiers Mint > 1", (char*)"Cancel", (char*)"Continue") == 1){
			ret = false;
		}
	}
	// printf("MINT VERS. VALUE %04X\n", mint_version);
	// if(mint_version < 0x010D){
	// 	event_timer_video = 8L;
	// }
	Getcookie(*(long *) "EdDI", &cookie_eddi);
	if( !cookie_eddi ){
		edDi_present = false;
		if(screen_workstation_bits_per_pixel == 8){
			sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d\nCookie EdDI needed", 
					screen_workstation_format, screen_workstation_bits_per_pixel );
			if(st_form_alert_choice(FORM_EXCLAM, alert_message, (char*)"Cancel", (char*)"Continue") == 1){
				ret = false;
			}
		}
	}
	if(Getcookie(*(long *) "_CF_",&cookie_cpu)){
		if(Getcookie(*(long *) "_CPU_",&cookie_cpu)){
			cpu_type = 0;
		} else {
			cpu_type = cookie_cpu;
		}
	} else {
		cpu_type = 54;
	}
	tos_version = (int)((OSHEADER *)get_sysvar(_sysbase))->os_version;
	if( (*(u_int32_t *)&((OSHEADER *)get_sysvar(_sysbase))->p_rsv2) == 0x45544f53){
		emutos_rom = true;
	}
	if (Getcookie(C__SND, &cookie_snd) == C_FOUND) {
		// if (cookie_snd & SND_GSXB) {
		// 	gsxb_present = true;
		// }
		if (cookie_snd & SND_MILANBLASTER) {
			milanblaster_present = true;
		}		
	} 

	// st_Get_Current_Dir(current_path);

#ifdef WITH_CACHE
if(screen_workstation_bits_per_pixel <= 8 && (screen_workstation_format < 1 || screen_workstation_bits_per_pixel < 8) ){
	use_cached_icons = TRUE;
}
#endif

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
	reverse_bits = (screen_vdi_info[16] < screen_vdi_info[48]);
	// printf("Reverse %d\n", reverse_bits);
	memcpy ( pix_palette, screen_vdi_info + 16, 512 );
	return screen_vdi_info[0];
}

void exit_app(){
	rsrc_free();
	v_clsvwk(st_vdi_handle);
	appl_exit();
}

void* exec_eventloop(void* p_param){
	while ( exit_call != TRUE ) {
		event_loop(NULL);
		pthread_yield_np();
	}
	TRACE(("End of event loop\n"))
	return NULL;
}

void *event_loop(void *result) {
	events = evnt_multi(
		// MU_MESAG|MU_BUTTON|MU_KEYBD,
		MU_MESAG|MU_BUTTON|MU_KEYBD|MU_TIMER,
		256 | 1, 3, butdown, /* button state tested for UP/DOWN */
		r1_flags, r1.g_x, r1.g_y, r1.g_w, r1.g_h, /* M1 event */
		r2_flags, r2.g_x, r2.g_y, r2.g_w, r2.g_h, /* M2 event */
		msg_buffer, /* Pointer to msg */
		event_timer_used,
		&mx, &my, &mb,
		&ks, &kc,
		&mc /* Single/double clicks */
		);
	if( events & MU_MESAG )
	{
		TRACE(("msg_buffer[0] = %d\n", msg_buffer[0]))
		switch (msg_buffer[0])
		{
		case 0x4711: /*VA_START*/
			pfile = *(char **)&msg_buffer[3];
			do {
				va_file = (char*)mem_alloc(128);
				memset(va_file, 0, 128);
				pfile = GetNextVaStartFileName( pfile, va_file ) ;
				new_win_img(va_file);
				mem_free(va_file);
			} while ( pfile ) ;
			break;
		case AP_DRAGDROP:
			char *buff, *file, *name;	
			int dd_hdl, dd_msg;
			int32_t size;
			char* ext;
			ext = (char*)mem_alloc(32);
			memset(ext, 0, 32);			
			dd_hdl = ddopen( msg_buffer[7], DD_OK);
			if( dd_hdl < 0){ break; }			
			strnset( ext, 0, 32);
			strcpy( ext, "ARGS");
			dd_msg = ddsexts( dd_hdl, ext );
			if( dd_msg < 0){ break; }
			file = (char*)mem_alloc(128); memset(file, 0, 128);
			name = (char*)mem_alloc(128); memset(name, 0, 128);					
			dd_msg = ddrtry( dd_hdl, name, file, ext, &size);
			if( !strncmp( ext, "ARGS", 4)) {
				ddreply(dd_hdl, DD_OK);
				buff = (char*)mem_alloc(size);
				memset(buff, 0, size);
				if( Fread( dd_hdl, size, buff) == size) {
					TRACE(("--> Drag&drop list %s <--", buff))
					buff[size] = '\0';
					pfile = buff;
					// printf("Files list %s\n", pfile);
					do {
						va_file = (char*)mem_alloc(128);
						memset(va_file, 0, 128);
						pfile = GetNextVaStartFileName( pfile, va_file ) ;
						// printf("Opening %s\n", va_file);
						new_win_img(va_file);
						mem_free(va_file);
					} while ( pfile ) ;
				}
				mem_free( buff);
				ddclose( dd_hdl);
			} else { ddreply(dd_hdl, DD_NAK);}
			ddclose( dd_hdl); mem_free(name); mem_free(file);mem_free(ext);
			break;
		case AP_RESCHG:
		case AP_TERM:
			st_Win_Close_All();
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
				if( selected_window->wi_data->autoscale ){
					selected_window->wi_data->remap_displayed_mfdb = TRUE;
				}
				full_size_window(selected_window->wi_handle);
				selected_window->refresh_win(selected_window->wi_handle);
				if(selected_window->wi_control_bar != NULL){
					st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					st_Control_Bar_Redraw( selected_window->wi_control_bar, msg_buffer[3]);
				}
			}
			break;
		case WM_SIZED:
			int16_t window_area_buffer[4];
			selected_window = detect_window(msg_buffer[3]);
			if(selected_window != NULL){
				wind_set(msg_buffer[3],WF_TOP,0,0,0,0);
				wind_calc(WC_WORK,selected_window->wi_style, msg_buffer[4], msg_buffer[5], msg_buffer[6], msg_buffer[7],&window_area_buffer[0],&window_area_buffer[1],&window_area_buffer[2],&window_area_buffer[3]);
				if(selected_window->wi_data->window_size_limited && !selected_window->wi_data->autoscale){
					if(selected_window->wi_data->thumbnail_master == TRUE){
						window_area_buffer[2] = MIN(selected_window->wi_thumb->thumbs_max_area_w, window_area_buffer[2]);
						window_area_buffer[3] = MIN(selected_window->wi_thumb->thumbs_max_area_h, window_area_buffer[3]);
					} else if(selected_window->wi_data->control_bar_media == TRUE || selected_window->wi_data->video_media){
						window_area_buffer[2] = MIN(selected_window->total_length_w, window_area_buffer[2]);
						window_area_buffer[3] = MIN(selected_window->total_length_h, window_area_buffer[3]);
					}else {
						window_area_buffer[2] = MAX( MIN(selected_window->total_length_w, window_area_buffer[2]), MIN_WINDOWS_WSIZE);
						window_area_buffer[3] = MAX( MIN(selected_window->total_length_h, window_area_buffer[3]), MIN_WINDOWS_HSIZE);
					}
				}
				if(selected_window->wi_data->autoscale || selected_window->wi_data->fx_on){					
					selected_window->wi_data->remap_displayed_mfdb = TRUE;
					send_message(selected_window->wi_handle, WM_REDRAW);
				}
				wind_calc(WC_BORDER,selected_window->wi_style, window_area_buffer[0], window_area_buffer[1], window_area_buffer[2], window_area_buffer[3],&window_area_buffer[0],&window_area_buffer[1],&window_area_buffer[2],&window_area_buffer[3]);
				wind_set(msg_buffer[3],WF_CURRXYWH, window_area_buffer[0], window_area_buffer[1], window_area_buffer[2], window_area_buffer[3]);

				update_struct_window(selected_window);

				if(selected_window->wi_data->thumbnail_master != TRUE ){
					selected_window->refresh_win(selected_window->wi_handle);
				}

				if(selected_window->wi_control_bar != NULL){
					GRECT *rect = &selected_window->wi_control_bar->rect_control_bar;
					form_dial(FMD_FINISH, 0, 0, 0, 0, rect->g_x, rect->g_y, rect->g_w, rect->g_h);
					st_Control_Bar_PXY_Update(selected_window->wi_control_bar, &selected_window->work_area);
					st_Reload_Control_Bar(selected_window, selected_window->wi_control_bar);
					send_message(selected_window->wi_handle, WM_REDRAW);
				}
				if(selected_window->wi_data->thumbnail_master == TRUE){
					if(selected_window->wi_thumb->thumbs_cols != selected_window->wi_thumb->thumb_w_Item / selected_window->work_area.g_w){
						selected_window->refresh_win(selected_window->wi_handle);
						send_message(selected_window->wi_handle, WM_REDRAW);
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
						// st_Thumb_Text_Refresh(msg_buffer[3]);
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
				if(selected_window->wi_data->wi_pth != NULL){
					selected_window->wi_data->wi_pth = NULL;
				}else{
					if(close_window(selected_window->wi_handle)){
						if (number_of_opened_windows < 1){
							exit_call = TRUE; 
						}
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
			pthread_yield_np();	
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
					/* Multitos can't handle filename greater than 8+3 */
					if(mint_version > 0x0108){
						sprintf(this_time, "crop_%d-%02d-%02d_%02d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
					}else{
						sprintf(this_time, "crop_%d",selected_window->wi_handle);
					}
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
						pthread_yield_np();
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
					pthread_yield_np();
					break;
				}
			}
		}
	}
	// pthread_yield_np();
	// printf("events %#04x\n",events);
	return NULL;
}

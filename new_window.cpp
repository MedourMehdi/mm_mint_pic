#include "new_window.h"
#include "control_bar.h"
#include "utils/utils.h"
#include "file.h"
#include "thumbs/thumbs.h"
#include "utils_gfx/crop.h"
#include "utils_rsc/winform.h"

#include "img_heif/img_heif.h"
#include "img_png/img_png.h"
#include "img_webp/img_webp.h"
#include "img_jpeg/img_jpeg.h"
#include "img_tiff/img_tiff.h"
#include "img_bmp/img_bmp.h"
#include "img_tga/img_tga.h"
#include "img_pi/img_pi.h"
#include "img_svg/img_svg.h"
#include "img_pdf/img_pdf.h"
#include "img_gif/img_gif.h"

#include "img_dummy/img_dummy.h"

void* new_win_img_threaded(void* _this_file){
	char this_file[128] = {'\0'};
	strcpy((char*)this_file, (char*)_this_file);
	if(!new_win_img(this_file)){
		sprintf(alert_message, "Error opening window\n%s", this_file);
		st_form_alert(FORM_STOP, alert_message); 		
	}
	return NULL;
}

void* new_win_start_threaded(void* ){
	new_win_start();
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
				win_struct_array[i].wi_name = (char *)mem_alloc(sizeof(file) + 8);
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
						TRACE(("Failed open_file()\n"))
						return false;
					}
				}
				void* (*video_function)(void*) = NULL;
				file_extension = win_struct_array[i].wi_data->extension;
				TRACE(("File Extension %s\n", file_extension))
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
				} else if (check_ext(file_extension, "PI1") || check_ext(file_extension, "PI3") ){
					st_Init_Degas(&win_struct_array[i]);
				} else if (check_ext(file_extension, "SVG")){
					st_Init_SVG(&win_struct_array[i]);
				} else if (check_ext(file_extension, "PDF")){
					st_Init_PDF(&win_struct_array[i]);
				} else if (check_ext(file_extension, "GIF")){
					if(win_master_thumb == NULL && screen_workstation_bits_per_pixel > 8 && st_form_alert_choice(FORM_QUESTION, (char*)"How do you want open it?", (char*)"Video", (char*)"Image") == 1){
						win_struct_array[i].wi_data->video_media = TRUE;
						video_function = st_Win_Play_GIF_Video;
					}
					st_Init_GIF(&win_struct_array[i]);
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
				if(win_struct_array[i].wi_data->video_media){
					st_Open_Thread(video_function, (void*)&win_struct_array[i].wi_handle);
				}else{
					win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);
				}
                if(win_struct_array[i].rendering_time == FALSE){
                    end_time = st_Supexec(get200hz);
                    win_struct_array[i].rendering_time = (end_time - start_time) * 5;
                }
				TRACE(("Rendering time %lums\n", win_struct_array[i].rendering_time))
                st_Set_Clipping(CLIPPING_OFF, win_struct_array[i].work_pxy);
				if(win_struct_array[i].wi_data->doc_media){
					st_Init_WinDoc_Control_Bar((void*)&win_struct_array[i]);
				} else if(win_struct_array[i].wi_data->video_media){
					st_Init_WinVideo_Control_Bar((void*)&win_struct_array[i]);
				}
				else{
					st_Init_WinImage_Control_Bar((void*)&win_struct_array[i]);
				}
				if(win_struct_array[i].wi_data->thumbnail_slave){
					char* file = basename(win_struct_array[i].wi_data->path);
					char thumbs_title[strlen(file) + 16] = {0};
					sprintf(thumbs_title, "%s (%d elements)", file, win_struct_array[i].wi_data->img.img_total);
					new_win_thumbnails(thumbs_title, win_struct_array[i].wi_handle);
				}
				win_struct_array[i].wi_data->thumbnail_slave = TRUE;

			}
			if(!win_struct_array[i].wi_data->video_media){
				wind_set(win_struct_array[i].wi_handle,WF_TOP,0,0,0,0);
				win_struct_array[i].win_is_topped = TRUE;
				send_message(win_struct_array[i].wi_handle, WM_REDRAW);
			}
			return true;
		}
		i++;
	}
	TRACE(("Loop finished at %d iter\n", i))
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

				if( !rsc_already_loaded(new_file) ){
					if( !rsrc_load(win_struct_array[i].wi_data->rsc.rsc_file) ){
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

				win_struct_array[i].wi_name = (char *)mem_alloc(strlen(win_title) + 1);
				strcpy(win_struct_array[i].wi_name, win_title);
				win_struct_array[i].wi_data->path = (const char *)mem_alloc(strlen(win_title) + 15);
				strcpy((char*)win_struct_array[i].wi_data->path, win_title);

                open_window(&win_struct_array[i]);

				st_Init_WinImage_Control_Bar((void*)&win_struct_array[i]);

				win_struct_array[i].refresh_win(win_struct_array[i].wi_handle);

				win_struct_array[i].wi_data->stop_original_data_load = TRUE;
			}
			return win_struct_array[i].wi_handle;
		}
		i++;
	}
	return NIL;
}

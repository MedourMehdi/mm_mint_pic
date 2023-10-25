#include "png_ico.h"

#include "../utils/utils.h"
#include "../utils_gfx/pix_convert.h"

char ico_path[256] = {'\0'};

bool st_PNG_Decompress(const char *file_name, MFDB *foreground_mfdb);/**/
void st_Ico32_PNG_Load(struct_st_control_bar* this_control_bar, struct_st_ico_png *ico);/**/
void st_Ico_PNG_Update(struct_st_ico_png_list *ico_list_array, int16_t new_pos_x, int16_t new_pos_y);/**/

void st_Control_Bar_PNG_Handle(int16_t mouse_x, int16_t mouse_y, int16_t mouse_button, struct_st_control_bar *st_control_bar, void* param){
	u_int16_t i = 0;
	struct_st_ico_png *ico_ptr;
	if(st_control_bar != NULL){
		if(mouse_button == 1){
			while(st_control_bar->control_bar_list[ i ].icon_id > 0) {
				if( ( mouse_x > st_control_bar->pxy_control_bar[0] + st_control_bar->control_bar_list[ i ].main_icon->x ) &&
				mouse_x < ( st_control_bar->pxy_control_bar[0] + st_control_bar->control_bar_list[ i ].main_icon->x2 ) && ( mouse_y > st_control_bar->pxy_control_bar[1] + st_control_bar->control_bar_list[ i ].main_icon->y ) &&
				(mouse_y < st_control_bar->pxy_control_bar[1] + st_control_bar->control_bar_list[ i ].main_icon->y2) )
				{
					st_control_bar->control_bar_list[ i ].main_func(param);
					if(st_control_bar->control_bar_list[ i ].mask_icon != NULL){
						if( st_control_bar->control_bar_list[ i ].icon_status == TRUE ) {
							ico_ptr = st_control_bar->control_bar_list[ i ].main_icon;
							st_control_bar->control_bar_list[ i ].icon_status = FALSE;
						} else {
							ico_ptr = st_control_bar->control_bar_list[ i ].mask_icon;
							st_control_bar->control_bar_list[ i ].icon_status = TRUE;
						}
					}
					break;
				}
				i++;
			}
		} else { /* right click */
			while(st_control_bar->control_bar_list[ i ].icon_id > 0) {
				if( st_control_bar->control_bar_list[ i ].icon_status == TRUE ) {
					ico_ptr = st_control_bar->control_bar_list[ i ].mask_icon;
				} else {
					ico_ptr = st_control_bar->control_bar_list[ i ].main_icon;
				}
				if(st_control_bar->st_control_bar_mfdb.fd_nplanes > 8){
					st_Ico32_PNG_Load( st_control_bar, ico_ptr );
				}
				i++;
			}
		}
	}
}

bool st_Ico_PNG_Init(struct_st_ico_png_list *ico_list_array){
	u_int16_t i = 0;
	int16_t ico_path_len = strlen(ico_path);
	if( ico_path_len < 1){
		strcpy(ico_path, current_path);
	}
	while(ico_list_array[ i ].icon_id > 0) {
		ico_list_array[ i ].main_icon->filename = ico_list_array[ i ].main_icon_path;
		char this_ico_path[ico_path_len + strlen(ico_list_array[ i ].main_icon_path) + 1] = {'\0'};
		strcpy(this_ico_path, ico_path);
		strcat(this_ico_path, ico_list_array[ i ].main_icon_path);
		if(!st_PNG_Decompress(this_ico_path, &ico_list_array[ i ].main_icon->st_ico_mfdb)){
			return false;
		}

		if(ico_list_array[ i ].mask_icon_path != NULL){

			ico_list_array[ i ].mask_icon->filename = ico_list_array[ i ].mask_icon_path;

			strcpy(this_ico_path, ico_path);
			strcat(this_ico_path, ico_list_array[ i ].mask_icon_path);

			if(!st_PNG_Decompress(this_ico_path, &ico_list_array[ i ].mask_icon->st_ico_mfdb)){
				return false;
			}
		}
        st_Ico_PNG_Update( &ico_list_array[ i ], ico_list_array[ i ].pos_x, ico_list_array[ i ].pos_y );
		i++;
	}
	return true;
}

void st_Ico_PNG_Release( struct_st_ico_png_list *ico_list_array ){
	u_int16_t i = 0;
	while(ico_list_array[ i ].icon_id > 0) {
		mem_free(&ico_list_array[ i ].main_icon->st_ico_mfdb.fd_addr);

		if(ico_list_array[ i ].mask_icon_path != NULL){
			ico_list_array[ i ].mask_icon->filename = ico_list_array[ i ].mask_icon_path;
			mem_free(&ico_list_array[ i ].mask_icon->st_ico_mfdb.fd_addr);
		}
		i++;
	}
}

bool st_PNG_Decompress(const char *file_name, MFDB *foreground_mfdb)
{
	FILE *fp;

	int x, y;
	int width, height, channels, interlace;
	int number_of_passes;

	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	
	png_bytep * row_pointers;

	u_int8_t header[BYTES_TO_CHECK];

	fp = fopen(file_name, "rb");
	if (!fp){
		sprintf(alert_message, "Error reading file\n%s", file_name);
        st_form_alert(FORM_STOP, alert_message);
		return false;
	}
	fread(header, 1, 8, fp);

	if (png_sig_cmp((png_const_bytep)header, 0, BYTES_TO_CHECK)){
		sprintf(alert_message, "Not recognized as PNG\n%s", file_name);
        st_form_alert(FORM_STOP, alert_message);
		return false;		
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr){
		sprintf(alert_message, "Error: png_create_read_struct\n%s", file_name);
        st_form_alert(FORM_STOP, alert_message);
		return false;
	}
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr){
		sprintf(alert_message, "Error: png_create_info_struct\n%s", file_name);
        st_form_alert(FORM_STOP, alert_message);
		return false;
	}

	png_init_io(png_ptr, fp);

	png_set_sig_bytes(png_ptr, BYTES_TO_CHECK);
	png_read_info(png_ptr, info_ptr);

	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16) {
        png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if(png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)){
		png_set_tRNS_to_alpha(png_ptr);
	}
    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }
    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        png_set_gray_to_rgb(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS) != 0){
        png_set_tRNS_to_alpha(png_ptr);
	}

    png_read_update_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);
	channels = png_get_channels( png_ptr, info_ptr);

	row_pointers = (png_bytep *)mem_alloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
				row_pointers[y] = (png_byte *)mem_alloc(png_get_rowbytes(png_ptr,info_ptr));

	png_read_image(png_ptr, row_pointers);

	int16_t i, j, k, l;

	u_int16_t a, red, green, blue, iWidth;
	u_int8_t *destination_buffer = (u_int8_t*)st_ScreenBuffer_Alloc_bpp(width, height, channels << 3);
	int16_t width_stride = mfdb_update_bpp(foreground_mfdb, (int8_t*)destination_buffer, width, height, channels << 3);	
	iWidth = MFDB_STRIDE(width);
	if(channels == 4){
		for (y = (height - 1); y != -1; y--)
		{
			l =  y << 2;
			for (x = 0; x < width; x++)
			{
				j = x << 2;
				i = j + (l * iWidth);
				red = row_pointers[y][j++];
				green = row_pointers[y][j++];
				blue = row_pointers[y][j++];
				a = row_pointers[y][j++];

				destination_buffer[i++] = a;
				destination_buffer[i++] = red;
				destination_buffer[i++] = green;
				destination_buffer[i++] = blue;
			}
			for(k = width_stride; k > 0; k--){
				destination_buffer[i++] = 0x00;
				destination_buffer[i++] = 0xFF;
				destination_buffer[i++] = 0xFF;
				destination_buffer[i++] = 0xFF;
			}
		}
	}
	
    png_read_end(png_ptr, info_ptr);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(fp);
	for (y=0; y<height; y++){
		mem_free(row_pointers[y]);
	}
	mem_free(row_pointers);
	return true;
}

void st_Ico32_PNG_Load(struct_st_control_bar* this_control_bar, struct_st_ico_png *ico){
	MFDB* 		dest_mfdb = &this_control_bar->st_control_bar_mfdb;
	u_int8_t*	destination_buffer = (u_int8_t *)dest_mfdb->fd_addr;
	u_int8_t*	ico_buffer = (u_int8_t*)ico->st_ico_mfdb.fd_addr;
	int16_t		width = ico->st_ico_mfdb.fd_w;
	int16_t		height = ico->st_ico_mfdb.fd_h;
	int16_t		x, y;
	u_int32_t	m, n, i, j, k;
	u_int8_t    fg_r, fg_g, fg_b, a, r, g, b, red, green, blue;
	int16_t		dst_width_stride = MFDB_STRIDE(dest_mfdb->fd_w) - dest_mfdb->fd_w;
	int16_t		src_width_stride = MFDB_STRIDE(width) - width;

	for(y = 0; y < height; y++){
		m = ((MFDB_STRIDE(width)) * y);
		n = ((MFDB_STRIDE(dest_mfdb->fd_w)) * (y + ico->y));
		for(x = 0; x < (MFDB_STRIDE(width)); x++){
			j = ((m + x) << 2);
			// i = ((n + x + ico->x) << 2);
			i = (n + x + ico->x);
			switch (this_control_bar->st_control_bar_mfdb.fd_nplanes)
			{
			case 16:
				i = i << 1;
				k = i;
				break;
			case 24:
				// i = i * 3;
				i = (i << 1) + i;
				k = i - 1;
				break;
			case 32:
				i = i << 2;
				k = i;
				break;							
			default:
				break;
			}
 			a = ico_buffer[j++];
			r = ico_buffer[j++];
			g = ico_buffer[j++];
			b = ico_buffer[j++];
			if( a == 255){
				fg_r	= r;
				fg_g	= g;
				fg_b 	= b;
			}
			else if ( a == 0){
				fg_r	= destination_buffer[k + 1];
				fg_g	= destination_buffer[k + 2];
				fg_b	= destination_buffer[k + 3];
			}
			else{
				COMPUTE_TRANSPARENCY(fg_r, div_255_fast(a), r, destination_buffer[k + 1]);
				COMPUTE_TRANSPARENCY(fg_g, div_255_fast(a), g, destination_buffer[k + 2]);
				COMPUTE_TRANSPARENCY(fg_b, div_255_fast(a), b, destination_buffer[k + 3]);
			}
			if(this_control_bar->st_control_bar_mfdb.fd_nplanes == 16){
				if(a == 255){
					u_int32_t pix32 = (a << 24) | ((fg_r & 0xFF) << 16) | ((fg_g & 0xFF) << 8) | (fg_b & 0xFF);
					u_int16_t* pix16 = (u_int16_t*)&destination_buffer[i];
					*pix16 = ARGB_to_RGB565((u_int8_t*)&pix32);
				}
				i += 2;
			} else if(this_control_bar->st_control_bar_mfdb.fd_nplanes == 24){
				destination_buffer[i++] = fg_r;
				destination_buffer[i++] = fg_g;
				destination_buffer[i++] = fg_b;
			} else {
				destination_buffer[i++] = a;
				destination_buffer[i++] = fg_r;
				destination_buffer[i++] = fg_g;
				destination_buffer[i++] = fg_b;
			}
		}
	}
}

void st_Ico_PNG_Update(struct_st_ico_png_list *ico_list_array, int16_t new_pos_x, int16_t new_pos_y){

		ico_list_array->pos_x = new_pos_x < 0 ? ico_list_array->pos_x : new_pos_x;
		ico_list_array->pos_y = new_pos_y < 0 ? ico_list_array->pos_y : new_pos_y;

		ico_list_array->main_icon->x = ico_list_array->pos_x;
		ico_list_array->main_icon->y = ico_list_array->pos_y;

		ico_list_array->main_icon->x2 = ico_list_array->main_icon->x + ico_list_array->main_icon->st_ico_mfdb.fd_w;
		ico_list_array->main_icon->y2 = ico_list_array->main_icon->y + ico_list_array->main_icon->st_ico_mfdb.fd_h;

		if(ico_list_array->mask_icon_path != NULL){

			ico_list_array->mask_icon->x = ico_list_array->pos_x;
			ico_list_array->mask_icon->y = ico_list_array->pos_y;

			ico_list_array->mask_icon->x2 = ico_list_array->mask_icon->x + ico_list_array->mask_icon->st_ico_mfdb.fd_w;
			ico_list_array->mask_icon->y2 = ico_list_array->mask_icon->y + ico_list_array->mask_icon->st_ico_mfdb.fd_h;

		}
}

void st_Control_Bar_Buffer_to_Screen(struct_st_control_bar* control_bar, GRECT* raster_dest){

	int16_t xy[8], xy_clip[4];
	int16_t w, h, x, y;
	int16_t clipx, clipy, clipw, cliph;

	VdiHdl*	my_vdi_handle = control_bar->vdi_handle;
	MFDB*	screen = control_bar->virtual_screen_mfdb;
	x = 0;
	y = 0;
	w = control_bar->st_control_bar_mfdb.fd_w;
	h = control_bar->st_control_bar_mfdb.fd_h;

	if( control_bar->rect_control_bar.g_x < raster_dest->g_x){
		x = raster_dest->g_x - control_bar->rect_control_bar.g_x;
	}
	if( control_bar->rect_control_bar.g_y < raster_dest->g_y){
		y = raster_dest->g_y - control_bar->rect_control_bar.g_y;
	}

	clipx = raster_dest->g_x;
	clipy = raster_dest->g_y;
	clipw = raster_dest->g_w;
	cliph = raster_dest->g_h;

	if((clipw != 0) && ( cliph != 0)) {
		xy_clip[0] = clipx; xy_clip[1] = clipy; xy_clip[2] = clipx + clipw - 1; xy_clip[3] = clipy + cliph;
		vs_clip( *my_vdi_handle, TRUE, xy_clip );
	}

	/* Source buffer */
	xy[0] = x; xy[1] = y; xy[2] = w - 1; xy[3] = h - 1;
	/* Destination Buffer */
	xy[4] = xy_clip[0]; xy[5] = xy_clip[1]; xy[6] = xy_clip[2]; xy[7] = xy_clip[3];
	graf_mouse(M_OFF,0L);
	vro_cpyfm(*my_vdi_handle, S_ONLY, xy, &control_bar->st_control_bar_mfdb, screen);
	graf_mouse(M_ON,0L);
	vs_clip(*my_vdi_handle, FALSE, xy_clip);
}

void st_Control_Bar_PXY_Update(struct_st_control_bar *this_control_bar, GRECT *win_work_area){
	if(this_control_bar != NULL){
		this_control_bar->pxy_control_bar[0] = win_work_area->g_x;
		this_control_bar->pxy_control_bar[2] = win_work_area->g_x + win_work_area->g_w - 1;
		this_control_bar->pxy_control_bar[3] = win_work_area->g_y + win_work_area->g_h;
		this_control_bar->pxy_control_bar[1] = this_control_bar->pxy_control_bar[3] - this_control_bar->control_bar_h;
		array_to_grect(this_control_bar->pxy_control_bar, &this_control_bar->rect_control_bar);
	}
}

void st_Control_Bar_Refresh_MFDB(struct_st_control_bar *control_bar,  MFDB *background_mfdb, int16_t elevator_posx, int16_t elevator_posy, int16_t win_work_area_width, int16_t win_work_area_height){

	control_bar->background_mfdb = background_mfdb;

	int16_t nb_components = background_mfdb->fd_nplanes >> 3;
	u_int16_t control_bar_height = CONTROLBAR_H;
	u_int8_t* dst_buffer = st_ScreenBuffer_Alloc_bpp(win_work_area_width, control_bar_height, nb_components << 3);
	if(dst_buffer == NULL){
		sprintf(alert_message,"Error\nOut of memory");
		st_form_alert(FORM_STOP, alert_message);
	}
	if(control_bar->st_control_bar_mfdb.fd_addr != NULL){
		mem_free(control_bar->st_control_bar_mfdb.fd_addr);
	}
	mfdb_update_bpp(&control_bar->st_control_bar_mfdb, (int8_t *)dst_buffer, win_work_area_width, control_bar_height, nb_components << 3); 

	int16_t xy[8];
	/* Source MFDB */
	xy[0] = MAX(elevator_posx , 0); xy[1] = MAX(elevator_posy , 0) + win_work_area_height - control_bar->st_control_bar_mfdb.fd_h;
	xy[2] = xy[0] + win_work_area_width; xy[3] = xy[1] + control_bar->st_control_bar_mfdb.fd_h;
	/* Destination MFDB */
	xy[4] = 0; xy[5] = 0; 
	xy[6] = control_bar->st_control_bar_mfdb.fd_w; xy[7] = control_bar->st_control_bar_mfdb.fd_h;
	
	graf_mouse(M_OFF,0L);
	vro_cpyfm(st_vdi_handle, S_ONLY, xy, control_bar->background_mfdb, &control_bar->st_control_bar_mfdb);	
	graf_mouse(M_ON,0L);

	if(control_bar->transparency == TRUE){
		switch (background_mfdb->fd_nplanes)
		{
		case 16:
			st_Color_Transparency_RGB565((u_int16_t *)control_bar->st_control_bar_mfdb.fd_addr, &control_bar->transparency_color, control_bar->st_control_bar_mfdb.fd_w, control_bar->st_control_bar_mfdb.fd_h );
			break;
		case 24:
			st_Color_Transparency_RGB888((u_int8_t *)control_bar->st_control_bar_mfdb.fd_addr, &control_bar->transparency_color, control_bar->st_control_bar_mfdb.fd_w, control_bar->st_control_bar_mfdb.fd_h );
			break;
		case 32:
			st_Color_Transparency_ARGB((u_int32_t *)control_bar->st_control_bar_mfdb.fd_addr, &control_bar->transparency_color, control_bar->st_control_bar_mfdb.fd_w, control_bar->st_control_bar_mfdb.fd_h );
			break;					
		default:
			break;
		}
	}
	st_Control_Bar_PNG_Handle(0, 0, 0, control_bar, NULL);
}

void st_Control_Bar_Redraw(struct_st_control_bar* control_bar, int16_t my_win_handle){
	GRECT rect;
	int16_t pxy_dest[4];
	graf_mouse(M_OFF,0L);
	wind_update(BEG_UPDATE);
	wind_get(my_win_handle, WF_FIRSTXYWH, &rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
	while(rect.g_h != 0 && rect.g_w != 0){
		if ( rc_intersect((GRECT *)&msg_buffer[4], &rect) ){
			if ( rc_intersect(&control_bar->rect_control_bar, &rect) ){
				st_Control_Bar_Buffer_to_Screen(control_bar, &rect);
			}
		}
		wind_get(my_win_handle,WF_NEXTXYWH,&rect.g_x, &rect.g_y, &rect.g_w, &rect.g_h);
	}
	wind_update(END_UPDATE);
	graf_mouse(ARROW,0L);
	graf_mouse(M_ON,0L);
}

void st_Control_Bar_Refresh_Classic(struct_st_control_bar *control_bar, int16_t control_bar_requested_width, int16_t bpp){

	if(control_bar->need_to_reload_control_bar){
		u_int32_t fill_color = 0xFFB7ADAD;
		if(bpp < 8){
			fill_color = 0xFFFFFFFF;
		}
		MFDB* dst_mfdb = NULL;
		u_int8_t* dst_buffer = st_ScreenBuffer_Alloc_bpp(control_bar_requested_width, CONTROLBAR_H, 32);
		if(dst_buffer == NULL){
			sprintf(alert_message,"Error\nOut of memory");
			st_form_alert(FORM_STOP, alert_message);
		}

		void* old_ptr = control_bar->st_control_bar_mfdb.fd_addr;

		MFDB* MFDB32 = mfdb_alloc_bpp((int8_t *)dst_buffer, control_bar_requested_width, CONTROLBAR_H, 32);
		st_MFDB_Fill(MFDB32, fill_color);

		mfdb_update_bpp(&control_bar->st_control_bar_mfdb, (int8_t *)dst_buffer, control_bar_requested_width, CONTROLBAR_H, 32); 
		st_Control_Bar_PNG_Handle(0, 0, 0, control_bar, NULL);
		
		switch (bpp)
		{
		case 1:
            dst_mfdb = st_MFDB32_To_MFDB1bpp(MFDB32);
			mfdb_update_bpp(&control_bar->st_control_bar_mfdb, (int8_t*)dst_mfdb->fd_addr, dst_mfdb->fd_w, dst_mfdb->fd_h, bpp );
			mfdb_free(MFDB32);
			break;
		case 4:
            dst_mfdb = st_MFDB32_To_MFDB4bpp_Gray(MFDB32);
			mfdb_update_bpp(&control_bar->st_control_bar_mfdb, (int8_t*)dst_mfdb->fd_addr, dst_mfdb->fd_w, dst_mfdb->fd_h, bpp );
			mfdb_free(MFDB32);
			break;				
		case 8:
            dst_mfdb = st_MFDB32_To_MFDB8bpp(MFDB32);
			mfdb_update_bpp(&control_bar->st_control_bar_mfdb, (int8_t*)dst_mfdb->fd_addr, dst_mfdb->fd_w, dst_mfdb->fd_h, bpp);
			mfdb_free(MFDB32);
			break;
        case 32: /* 32 bits per pixels */
			mem_free(MFDB32);
            break;
        case 24: /* 24 bits per pixels */
            dst_mfdb = st_MFDB32_To_MFDB24(MFDB32);
			control_bar->st_control_bar_mfdb.fd_addr = dst_mfdb->fd_addr;
			control_bar->st_control_bar_mfdb.fd_nplanes = dst_mfdb->fd_nplanes;	
			mfdb_free(MFDB32);
            break;
        case 16: /* 16 bits per pixels */
            dst_mfdb = st_MFDB32_To_MFDB16(MFDB32);
			control_bar->st_control_bar_mfdb.fd_addr = dst_mfdb->fd_addr;
			control_bar->st_control_bar_mfdb.fd_nplanes = dst_mfdb->fd_nplanes;	
			mfdb_free(MFDB32);
            break;
		default:
			break;
		}
		if(old_ptr != NULL){mem_free(old_ptr);}
		control_bar->need_to_reload_control_bar = FALSE;
	}
}
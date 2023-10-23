#include "./thumbs.h"
#include "../utils_gfx/pix_convert.h"
#include "../utils/utils.h"

#include "../utils_gfx/ttf.h"

#ifndef PRIMARY_IMAGE_ID
#define PRIMARY_IMAGE_ID    -1
#endif

void st_Thumb_Win_PXY_Update(struct_st_thumbs_list *thumbs_list_array, int16_t new_pos_x, int16_t new_pos_y);
MFDB* st_Init_Outline_MFDB(struct_thumbs *this_win_thumb, u_int32_t background_color);
MFDB* st_Outline_MFDB(struct_thumbs *this_win_thumb, u_int32_t thumb_idx);

void st_Thumb_Win_PXY_Update(struct_st_thumbs_list *thumbs_list_array, int16_t new_pos_x, int16_t new_pos_y){
    int16_t *x1, *y1, *x2, *y2;

    x1 = &thumbs_list_array->thumb_win_pxy[0];
    x2 = &thumbs_list_array->thumb_win_pxy[2];
    y1 = &thumbs_list_array->thumb_win_pxy[1];
    y2 = &thumbs_list_array->thumb_win_pxy[3];

    *x1 = new_pos_x < 0 ? *x1 : new_pos_x;
    *y1 = new_pos_y < 0 ? *y1 : new_pos_y;
    *x2 = *x1 + thumbs_list_array->thumb_mfdb->fd_w;
    *y2 = *y1 + thumbs_list_array->thumb_mfdb->fd_h;

}

void st_Thumb_Desk_PXY_Update(struct_thumbs *this_win_thumb, int16_t* win_pxy){
    int16_t x1, y1, x2, y2;
    struct_st_thumbs_list *thumb_ptr = this_win_thumb->thumbs_list_array;


    while (thumb_ptr != NULL){

        struct_st_thumbs_list *thumb_ptr_next = thumb_ptr->next;

        x1 = thumb_ptr->thumb_win_pxy[0];
        x2 = thumb_ptr->thumb_win_pxy[2];
        y1 = thumb_ptr->thumb_win_pxy[1];
        y2 = thumb_ptr->thumb_win_pxy[3];

        thumb_ptr->thumb_desk_pxy[0] = win_pxy[0] + x1;
        thumb_ptr->thumb_desk_pxy[2] = win_pxy[0] + x2;
        thumb_ptr->thumb_desk_pxy[1] = win_pxy[1] + y1;
        thumb_ptr->thumb_desk_pxy[3] = win_pxy[1] + y2;

        thumb_ptr = thumb_ptr_next;
    }
}

struct_thumbs* st_Thumb_Alloc(int16_t thumbs_nb, int16_t slave_win_handle, int16_t padx, int16_t pady, int16_t thumbnail_w_size, int16_t thumbnail_h_size){

    struct_thumbs *this_win_thumb = (struct_thumbs*)mem_alloc(sizeof(struct_thumbs));

    this_win_thumb->wi_original_thumbs_mfdb = NULL;
    this_win_thumb->thumb_background_color = 0xFFFFFFFF;
    this_win_thumb->thumb_selected_color = 0xFF0000FF;

    this_win_thumb->padx = padx;
    this_win_thumb->pady = pady;

    this_win_thumb->slave_win_handle = slave_win_handle;

    this_win_thumb->thumbs_nb = thumbs_nb;

    this_win_thumb->thumb_w_size = thumbnail_w_size;
    this_win_thumb->thumb_h_size = thumbnail_h_size;

    this_win_thumb->thumb_w_Item = (this_win_thumb->padx << 1 ) + this_win_thumb->thumb_w_size;
    this_win_thumb->thumb_h_Item = (this_win_thumb->pady << 1 ) + this_win_thumb->thumb_h_size;

    this_win_thumb->thumbs_cols = 1;
    this_win_thumb->thumbs_rows = thumbs_nb;

    // this_win_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc((this_win_thumb->thumbs_nb ) * sizeof(struct_st_thumbs_list));

    this_win_thumb->thumbs_area_w = this_win_thumb->thumb_w_Item + this_win_thumb->padx;
    this_win_thumb->thumbs_area_h = (this_win_thumb->thumb_w_Item * this_win_thumb->thumbs_nb) + this_win_thumb->pady;

    this_win_thumb->thumbs_max_area_w = (this_win_thumb->thumb_w_Item * this_win_thumb->thumbs_nb) + this_win_thumb->pady;
    this_win_thumb->thumbs_max_area_h = (this_win_thumb->thumb_h_Item * this_win_thumb->thumbs_nb) + this_win_thumb->pady;

    this_win_thumb->thumb_clean_func = st_Thumb_Free;

    this_win_thumb->thumb_background_mfdb = st_Init_Outline_MFDB(this_win_thumb, this_win_thumb->thumb_background_color);
    this_win_thumb->thumb_selected_mfdb = st_Init_Outline_MFDB(this_win_thumb, this_win_thumb->thumb_selected_color);

    this_win_thumb->thumbs_open_new_win = TRUE; /* If set to TRUE then each images in thumbs win will open a new window */

    this_win_thumb->thumbs_selected_nb = NIL;

    return this_win_thumb;

}

void* st_Thumb_Free(void* p_param){

    struct_thumbs *this_win_thumb = (struct_thumbs *)p_param;

    struct_st_thumbs_list* thumb_ptr = this_win_thumb->thumbs_list_array;

    while (thumb_ptr != NULL)
    {
        struct_st_thumbs_list* thumb_ptr_next = thumb_ptr->next;

        mfdb_free(thumb_ptr->thumb_mfdb);
        mem_free(thumb_ptr);

        thumb_ptr = thumb_ptr_next;

    }
    
    mfdb_free(this_win_thumb->wi_original_thumbs_mfdb);
    mfdb_free(this_win_thumb->thumb_background_mfdb);
    mfdb_free(this_win_thumb->thumb_selected_mfdb);
    mem_free(this_win_thumb);
    this_win_thumb = NULL;
    return NULL;
}

MFDB* st_Init_Outline_MFDB(struct_thumbs *this_win_thumb, u_int32_t background_color){

    int16_t width = this_win_thumb->thumb_w_size + (this_win_thumb->padx << 1); 
    int16_t height = this_win_thumb->thumb_h_size + (this_win_thumb->pady << 1);

    u_int8_t* mfdb32_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
    MFDB* mfdb32 = mfdb_alloc_bpp((int8_t*)mfdb32_buffer, width, height, 32);
    st_MFDB_Fill(mfdb32, background_color);
    MFDB* mfdb_bpp;

    switch (screen_workstation_bits_per_pixel){
        case 1:	
            mfdb_bpp = st_MFDB32_To_MFDB1bpp(mfdb32);
            mfdb_free(mfdb32);
            break;
        case 4:
            mfdb_bpp = st_MFDB32_To_MFDB4bpp(mfdb32);
            mfdb_free(mfdb32);
            break;
        case 8:
            mfdb_bpp = st_MFDB32_To_MFDB8bpp(mfdb32);
            mfdb_free(mfdb32);
            break;
        case 16:
            mfdb_bpp = st_MFDB32_To_MFDB16(mfdb32);
            mfdb_free(mfdb32);
            break;   
        case 24:
            mfdb_bpp = st_MFDB32_To_MFDB24(mfdb32);
            mfdb_free(mfdb32);
            break;   
        case 32:
            mfdb_bpp = mfdb32;
            break;                                                    
        default:
            mfdb_bpp = NULL;
            break;
    }
    return mfdb_bpp;
}

MFDB* st_Outline_MFDB(struct_thumbs *this_win_thumb, u_int32_t thumb_idx){
    struct_st_thumbs_list *thumb_ptr = this_win_thumb->thumbs_list_array;
    // printf("Before st_Outline_MFDB while loop\n");
    while (thumb_ptr->thumb_id != thumb_idx){
        thumb_ptr = thumb_ptr->next;
    }
    // printf("After st_Outline_MFDB while loop\n");
    MFDB* this_mfdb = thumb_ptr->thumb_mfdb;

    u_int16_t width =  this_mfdb->fd_w + this_win_thumb->padx; 
    u_int16_t height = this_mfdb->fd_h + this_win_thumb->pady;    
    int16_t width_stride = MFDB_STRIDE(width) - width;

    MFDB *dst_mfdb = (MFDB*)mem_alloc(sizeof(MFDB));
    MFDB *src_mfdb;
    if(thumb_ptr->thumb_selected == TRUE){
        src_mfdb = this_win_thumb->thumb_selected_mfdb;
    }else{
        src_mfdb = this_win_thumb->thumb_background_mfdb;
    }
    mfdb_duplicate( src_mfdb, dst_mfdb);

    dst_mfdb->fd_w = width;
    dst_mfdb->fd_h = height;
    dst_mfdb->fd_wdwidth = MFDB_STRIDE(width + width_stride) >> 4;

    int16_t xy[8];

    xy[0] = 0; 
    xy[2] = this_mfdb->fd_w - 1;

    xy[1] = 0;
    xy[3] = this_mfdb->fd_h - 1;

    xy[4] = (width >> 1) - (this_mfdb->fd_w >> 1);
    xy[6] = this_mfdb->fd_w;

    xy[5] = (height >> 1) - (this_mfdb->fd_h >> 1);
    xy[7] = this_mfdb->fd_h;

    vro_cpyfm(st_vdi_handle, S_ONLY, xy, this_mfdb, dst_mfdb);

    return dst_mfdb;
}

void st_Handle_Click_Thumbnail(struct_window *this_win, int16_t mouse_x, int16_t mouse_y, int16_t mouse_button){
	printf("st_Handle_Click_Thumbnail\n");
    struct_thumbs* this_thumb_struct = this_win->wi_thumb;
    struct_st_thumbs_list *thumb_ptr = this_win->wi_thumb->thumbs_list_array;
    
    while (thumb_ptr != NULL)
    {
        if(
            (   mouse_x + this_win->current_pos_x > thumb_ptr->thumb_desk_pxy[0] ) 
            && ( mouse_x + this_win->current_pos_x < thumb_ptr->thumb_desk_pxy[2] )
            && ( mouse_y + this_win->current_pos_y > thumb_ptr->thumb_desk_pxy[1] ) 
            && ( mouse_y + this_win->current_pos_y < thumb_ptr->thumb_desk_pxy[3] )
        ) {
            if(thumb_ptr->thumb_selectable == TRUE){
                struct_window* dest_win;
                printf("selectable thumb id %d idx %d\n", thumb_ptr->thumb_id, thumb_ptr->thumb_index);
                this_win->wi_data->img.img_id = thumb_ptr->thumb_id;
                this_win->wi_data->img.img_index = thumb_ptr->thumb_index;
                this_thumb_struct->thumbs_selected_nb = thumb_ptr->thumb_index;
                if(this_thumb_struct->thumbs_open_new_win){
                    this_win->wi_thumb->open_win_func(this_win->wi_data->path);
                } else {
                    dest_win = detect_window(this_win->wi_thumb->slave_win_handle);
                    if(dest_win == NULL){
                        form_alert(1, "[1][ERROR WHILE OPENING IMAGE][Okay]");
                        return;
                    }

                    /* Disable old selected thumbs */
                    dest_win->wi_data->img.img_id = NIL;
                    this_win->wi_thumb->thumbs_selected_nb = dest_win->wi_data->img.img_index;
                    this_win->wi_thumb->thumbs_area_refresh = TRUE;
                    st_Start_Window_Process(this_win);
                    this_win->refresh_win(this_win->wi_handle);
                    st_End_Window_Process(this_win);
                    /* Enable new selected thumbs */
                    dest_win->wi_data->img.img_id = thumb_ptr->thumb_id;
                    dest_win->wi_data->stop_original_data_load = FALSE;
                    dest_win->wi_data->fx_on = FALSE;
                    dest_win->wi_data->remap_displayed_mfdb = TRUE;
                    dest_win->wi_data->img.scaled_pourcentage = 0;
                    dest_win->wi_data->img.rotate_degree = 0;	
                    dest_win->refresh_win(dest_win->wi_handle);
                    send_message(dest_win->wi_handle, WM_REDRAW);
                }

                st_Start_Window_Process(this_win);
                this_win->wi_thumb->thumbs_area_refresh = TRUE;
                st_Thumb_Refresh(this_win->wi_handle);
                st_End_Window_Process(this_win);
                wind_set(this_win->wi_handle,WF_TOP,0,0,0,0);
            }
            break;
        }
        thumb_ptr = thumb_ptr->next;
    }
    return;
}

void* st_Thumb_MFDB_Update(void *p_param){

    struct_thumbs *this_win_thumb = (struct_thumbs *)p_param;

    if(this_win_thumb != NULL){

        int16_t width = this_win_thumb->thumbs_area_w;
        int16_t height = this_win_thumb->thumbs_area_h;

        int16_t w_Item = this_win_thumb->thumb_w_Item;
        int16_t h_Item = this_win_thumb->thumb_h_Item;

        int16_t nb_total_rows, nb_total_cols = MAX((width) / w_Item , 1);

        if(this_win_thumb->thumbs_nb % nb_total_cols){
            nb_total_rows = (this_win_thumb->thumbs_nb / nb_total_cols) + 1;
        } else {
            nb_total_rows = MAX( (this_win_thumb->thumbs_nb / nb_total_cols), 1 );
        }

        if(this_win_thumb->thumbs_area_refresh == TRUE || this_win_thumb->thumbs_cols != nb_total_cols){


            int16_t w_nItems = ( this_win_thumb->thumb_w_Item * (this_win_thumb->thumbs_nb) ) + this_win_thumb->padx << 1;
            int16_t h_nItems = ( this_win_thumb->thumb_h_Item * (this_win_thumb->thumbs_nb) ) + this_win_thumb->pady << 1;

            if(this_win_thumb->thumbs_selected_nb < 0){
                u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp((nb_total_cols * this_win_thumb->thumb_w_Item) + (this_win_thumb->padx << 1), (nb_total_rows * this_win_thumb->thumb_h_Item) + (this_win_thumb->pady << 1), screen_workstation_bits_per_pixel);

                if(this_win_thumb->wi_original_thumbs_mfdb != NULL){
                    mem_free(this_win_thumb->wi_original_thumbs_mfdb->fd_addr);
                    mfdb_update_bpp(this_win_thumb->wi_original_thumbs_mfdb, (int8_t*)destination_buffer, (nb_total_cols * this_win_thumb->thumb_w_Item) + (this_win_thumb->padx << 1), (nb_total_rows * this_win_thumb->thumb_h_Item) + (this_win_thumb->pady << 1), screen_workstation_bits_per_pixel);
                }else{
                    this_win_thumb->wi_original_thumbs_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, (nb_total_cols * this_win_thumb->thumb_w_Item) + (this_win_thumb->padx << 1), (nb_total_rows * this_win_thumb->thumb_h_Item) + (this_win_thumb->pady << 1), screen_workstation_bits_per_pixel);
                }
                
                // if(this_win_thumb->wi_original_thumbs_mfdb != NULL){
                //     mfdb_free(this_win_thumb->wi_original_thumbs_mfdb);
                // }
                // this_win_thumb->wi_original_thumbs_mfdb = mfdb_alloc_bpp((int8_t*)destination_buffer, (nb_total_cols * this_win_thumb->thumb_w_Item) + (this_win_thumb->padx << 1), (nb_total_rows * this_win_thumb->thumb_h_Item) + (this_win_thumb->pady << 1), screen_workstation_bits_per_pixel);

                if(screen_workstation_bits_per_pixel == 32){
                    st_MFDB_Fill(this_win_thumb->wi_original_thumbs_mfdb, 0xCCCCCCCC);
                } 
                if(screen_workstation_bits_per_pixel == 24){
                    st_MFDB_Fill_bpp(this_win_thumb->wi_original_thumbs_mfdb, 0xCCCCCCCC, 24);
                }                 
                if(screen_workstation_bits_per_pixel == 16){
                    st_MFDB_Fill_bpp(this_win_thumb->wi_original_thumbs_mfdb, 0x0000B575, 16);
                }
            }

            int16_t xy[8];
            struct_st_thumbs_list* thumb_ptr = this_win_thumb->thumbs_list_array;
            xy[0] = 0; xy[1] = 0;
            
            while( thumb_ptr != NULL ){
                for( int16_t j = 0; j < nb_total_rows; j++) {
                    for ( int16_t k = 0; k < nb_total_cols; k++) {
                        if(thumb_ptr == NULL){
                            break;
                        }
                        struct_st_thumbs_list *thumb_ptr_next = thumb_ptr->next;
                        if(!thumb_ptr->thumb_visible){
                            thumb_ptr = thumb_ptr->next;
                            continue;
                        }
                        if(this_win_thumb->thumbs_selected_nb > NIL && this_win_thumb->thumbs_selected_nb != thumb_ptr->thumb_index){
                            thumb_ptr = thumb_ptr->next;
                            continue;
                        }
                        if(this_win_thumb->master_win_handle > 0){
                            if(get_win_thumb_slave_by_image_id(this_win_thumb->master_win_handle, thumb_ptr->thumb_id) != NULL){
                                thumb_ptr->thumb_selected = TRUE;
                            }else{
                                thumb_ptr->thumb_selected = FALSE;
                            }
                        }

                        int16_t thumb_xy[2];
                        thumb_xy[0] = (this_win_thumb->thumb_w_Item * k) + this_win_thumb->padx;
                        thumb_xy[1] = (this_win_thumb->thumb_h_Item * j) + this_win_thumb->pady;
                        st_Thumb_Win_PXY_Update(thumb_ptr, thumb_xy[0], thumb_xy[1]);

                        MFDB* thumb_mfdb = st_Outline_MFDB(this_win_thumb, thumb_ptr->thumb_id);

                        xy[2] = thumb_mfdb->fd_w; 
                        xy[3] = thumb_mfdb->fd_h - 1;
                        xy[4] = thumb_ptr->thumb_win_pxy[0]; 
                        xy[5] = thumb_ptr->thumb_win_pxy[1]; 
                        xy[6] = thumb_ptr->thumb_win_pxy[2] + this_win_thumb->padx; 
                        xy[7] = thumb_ptr->thumb_win_pxy[3] + this_win_thumb->pady; 
                        vro_cpyfm(st_vdi_handle, S_ONLY, xy, thumb_mfdb, this_win_thumb->wi_original_thumbs_mfdb);
                        mfdb_free(thumb_mfdb);
                        thumb_ptr = thumb_ptr_next;
                    }
                }
            }
            this_win_thumb->thumbs_rows = nb_total_rows;
            this_win_thumb->thumbs_cols = nb_total_cols;
            this_win_thumb->thumbs_selected_nb = NIL;
        }
        this_win_thumb->thumbs_area_refresh = FALSE;

    }
    return (void*)this_win_thumb->wi_original_thumbs_mfdb;
}

void st_Thumb_Refresh(int16_t win_thumb_handle){
	struct_window *this_win;
	this_win = detect_window(win_thumb_handle);
    if(this_win == NULL){
        return;
    }
    
    st_Start_Window_Process(this_win);
        update_struct_window(this_win);
        this_win->wi_thumb->thumbs_area_w = this_win->work_area.g_w;
        this_win->wi_thumb->thumbs_area_h = this_win->work_area.g_h;
        this_win->wi_to_display_mfdb = (MFDB*)st_Thumb_MFDB_Update((void*)this_win->wi_thumb);
        this_win->total_length_w = this_win->wi_to_display_mfdb->fd_w;
        this_win->total_length_h = this_win->wi_to_display_mfdb->fd_h;        
        st_Thumb_Desk_PXY_Update(this_win->wi_thumb, this_win->work_pxy);
    st_End_Window_Process(this_win);
    
	return;
}

void st_Check_Thumbs_Chain(struct_st_thumbs_list* thumb_ptr){
    while( thumb_ptr != NULL ){
        printf("\n###\tthumb_ptr->thumb_id\t%d\n",thumb_ptr->thumb_id);
        printf("###\tthumb_ptr->thumb_index\t%d\n",thumb_ptr->thumb_index);
        if(thumb_ptr->thumb_id != 0){
            printf("\n###\tthumb_ptr->prev->thumb_id\t%d\n",thumb_ptr->prev->thumb_id);
            printf("###\tthumb_ptr->prev->thumb_index\t%d\n",thumb_ptr->prev->thumb_index);
        }
        if(thumb_ptr->next != NULL){
            printf("\n###\tthumb_ptr->next->thumb_id\t%d\n",thumb_ptr->next->thumb_id);
            printf("###\tthumb_ptr->next->thumb_index\t%d\n",thumb_ptr->next->thumb_index);
        }
        thumb_ptr = thumb_ptr->next;
    }
}

void st_Thumb_List_Generic(struct_window *this_win, 
                            const char* title, const char* media_type, 
                            u_int16_t wanted_width, u_int16_t wanted_height,
                            u_int16_t wanted_padx, u_int16_t wanted_pady,
                            bool open_new_win){

    if(this_win->wi_data->img.img_total > 1){
        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)title);
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 1, (int8_t*)"Init");

        this_win->wi_data->thumbnail_slave = true;
        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win->wi_handle, wanted_padx, wanted_pady, wanted_width, wanted_height);

        this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
        struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
        struct_st_thumbs_list* prev_thumb_ptr = NULL;

        this_win->wi_thumb->thumbs_open_new_win = open_new_win;
        
        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;
        this_win->wi_thumb->thumbs_nb = this_win->wi_data->img.img_total;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {

            char progess_bar_indication[96];
            sprintf(progess_bar_indication, "Indexing media list: %s %d/%d", media_type, i+1, this_win->wi_thumb->thumbs_nb);
            st_Progress_Bar_Signal(this_win->wi_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), (int8_t*)progess_bar_indication);

            MFDB* thumb_original_mfdb;
            u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(wanted_width, wanted_height, 32);
            thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, wanted_width, wanted_height, 32);
            st_MFDB_Fill(thumb_original_mfdb,0x00FFFFFF);

            if(thumb_ptr == NULL){
                thumb_ptr = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
            }
            thumb_ptr->thumb_id = i;
            thumb_ptr->thumb_index = i + 1;
            thumb_ptr->thumb_selectable = TRUE;
            thumb_ptr->thumb_visible = TRUE;
            thumb_ptr->next = NULL;
            thumb_ptr->prev = prev_thumb_ptr;
            if(thumb_ptr->prev != NULL){
                thumb_ptr->prev->next = thumb_ptr;
            }

            char thumb_txt[10] = {'\0'};
            char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
            strcpy(font_path, current_path);
            strcat(font_path, TTF_DEFAULT_PATH);            
            sprintf(thumb_txt,"%s %d", media_type, thumb_ptr->thumb_index );
            print_ft_simple(4, thumb_original_mfdb->fd_h - 4, thumb_original_mfdb, font_path, 14, thumb_txt);

            if(screen_workstation_bits_per_pixel != 32){
                thumb_ptr->thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                thumb_ptr->thumb_mfdb = thumb_original_mfdb;
            }

            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + wanted_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += wanted_height + this_win->wi_thumb->pady;
            thumb_ptr->thumb_selected = FALSE;

            prev_thumb_ptr = thumb_ptr;
            thumb_ptr = NULL;

        }
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
        this_win->wi_thumb->thumbs_list_array->thumb_selected = TRUE;
    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
    }

}
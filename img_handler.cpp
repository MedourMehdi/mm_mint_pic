#include "img_handler.h"

#include "utils_gfx/pix_convert.h"
#include "utils/utils.h"

void st_Handle_FX(struct_window *this_win);

boolean st_Img32b_To_Window(struct_window *this_win){

    boolean ret = true;
    st_Start_Window_Process(this_win);
        if( this_win->wi_data->remap_displayed_mfdb == TRUE  || this_win->wi_data->fx_requested == TRUE ){
        u_int16_t work_area_w = this_win->work_area.g_w;
        u_int16_t work_area_h = this_win->work_area.g_h;
        if(this_win->wi_data->doc_media && this_win->wi_data->autoscale && this_win->wi_data->image_media){
            float factor_h;
            factor_h = this_win->wi_data->img.original_width  / (float)work_area_w;
            work_area_h = this_win->wi_data->img.original_height / factor_h;
        }
        switch (screen_workstation_bits_per_pixel) {
        case 32:
            if(this_win->wi_data->fx_requested == TRUE){
                st_Handle_FX(this_win);
                if(this_win->wi_data->fx_on == TRUE){
                    this_win->wi_to_work_in_mfdb = &this_win->wi_buffer_mfdb;
                } else {
                    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
                }
            }
            if(this_win->wi_data->autoscale == TRUE){
                st_Rescale_ARGB(this_win->wi_to_work_in_mfdb, &this_win->wi_rendered_mfdb, work_area_w, work_area_h);
                this_win->wi_to_display_mfdb = &this_win->wi_rendered_mfdb;
            } else {
                this_win->wi_to_display_mfdb = this_win->wi_to_work_in_mfdb;
            }
            break;
        default:
            if(this_win->wi_data->fx_requested == TRUE){
                st_Handle_FX(this_win);
                if(this_win->wi_data->fx_on == TRUE){
                    this_win->wi_to_work_in_mfdb = &this_win->wi_buffer_mfdb;
                    this_win->wi_data->wi_buffer_modified = FALSE;
                } else {
                    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
                }
            }
            if(this_win->wi_data->autoscale == TRUE){
                if(this_win->wi_rendered_bitdepth_mfdb != NULL){
                    mfdb_free(this_win->wi_rendered_bitdepth_mfdb);
                }                        
                st_Rescale_ARGB(this_win->wi_to_work_in_mfdb, &this_win->wi_rendered_mfdb, work_area_w, work_area_h);
                this_win->wi_rendered_bitdepth_mfdb = this_win->render_win(&this_win->wi_rendered_mfdb);
                this_win->wi_to_display_mfdb = this_win->wi_rendered_bitdepth_mfdb;
            } else {
                if(this_win->wi_data->wi_buffer_modified == FALSE){
                    if(this_win->wi_original_bitdepth_mfdb != NULL){
                        mfdb_free(this_win->wi_original_bitdepth_mfdb);
                    }
                    this_win->wi_original_bitdepth_mfdb = this_win->render_win(this_win->wi_to_work_in_mfdb);
                    this_win->wi_data->wi_buffer_modified = TRUE;
                }
                this_win->wi_to_display_mfdb = this_win->wi_original_bitdepth_mfdb;                       
            }
            break;
        }

        this_win->total_length_w = this_win->wi_to_display_mfdb->fd_w;
        this_win->total_length_h = this_win->wi_to_display_mfdb->fd_h;

        if(!this_win->wi_data->autoscale){st_Limit_Work_Area(this_win);}
        this_win->wi_data->remap_displayed_mfdb = FALSE;
    }
    st_End_Window_Process(this_win);
    return ret;
}

boolean st_Set_Renderer(struct_window *this_win){

    boolean ret = true;
    switch (screen_workstation_bits_per_pixel){
        case 1:	
            this_win->render_win = st_MFDB32_To_MFDB1bpp;
            break;
        case 2:
            this_win->render_win = NULL;
            ret = false;
            break;
        case 4:
            this_win->render_win = st_MFDB32_To_MFDB4bpp;
            break;
        case 8:
            this_win->render_win = st_MFDB32_To_MFDB8bpp;
            break;
        case 16:
            this_win->render_win = st_MFDB32_To_MFDB16;
            break;   
        case 24:
            this_win->render_win = st_MFDB32_To_MFDB24;
            break;   
        case 32:
            this_win->render_win = NULL;
            break;                                                    
        default:
            this_win->render_win = NULL;
            sprintf(alert_message, "st_Set_Renderer error: screen_workstation_bits_per_pixel == %d\n", screen_workstation_bits_per_pixel);
            st_form_alert(FORM_EXCLAM, alert_message);            
            ret = false;
            break;
    }
    return ret;
}

void st_Handle_FX(struct_window *this_win){
    if(this_win->wi_data->fx_requested == TRUE){

        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"Applying modifications");

        /**/
        MFDB *dst_mfdb = &this_win->wi_buffer_mfdb;

        /* This is the final rendered mfdb with all our FX */
        MFDB *fx_mfdb = (MFDB*)mem_alloc(sizeof(MFDB)); 

        /* We should use original data first */
        MFDB *src_mfdb = &this_win->wi_original_mfdb;

        /* We duplicate the original MFDB so we can work on it */
        mfdb_duplicate(src_mfdb, fx_mfdb);

        if(this_win->wi_data->resized && !this_win->wi_data->autoscale){
            st_Progress_Bar_Signal(this_win->wi_progress_bar, 50, (int8_t*)"Image Resizing");
            st_Rescale_ARGB(fx_mfdb, dst_mfdb, 
                    this_win->wi_data->img.export_width, this_win->wi_data->img.export_height);
            mfdb_duplicate(dst_mfdb, fx_mfdb);
        }

        /**/

        if(this_win->wi_data->img.scaled_pourcentage){
            st_Progress_Bar_Signal(this_win->wi_progress_bar, 50, (int8_t*)"Image Rescale");
            st_Rescale_ARGB(fx_mfdb, dst_mfdb, this_win->wi_data->img.scaled_width, this_win->wi_data->img.scaled_height);
            mfdb_duplicate(dst_mfdb, fx_mfdb);
        }          

        if(this_win->wi_data->img.rotate_degree){
            st_Progress_Bar_Signal(this_win->wi_progress_bar, 50, (int8_t*)"Image Rotation");
            st_Rotate_ARGB(fx_mfdb, dst_mfdb, this_win->wi_data->img.rotate_degree);
            mfdb_duplicate(dst_mfdb, fx_mfdb);
        }

        mfdb_duplicate(fx_mfdb ,dst_mfdb);
        
        mfdb_free(fx_mfdb);

        this_win->wi_data->fx_requested = FALSE;
        this_win->wi_data->fx_on = TRUE;

        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);

    }
}
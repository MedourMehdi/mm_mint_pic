#include "img_pi.h"
#include "../img_handler.h"

#include "../utils/utils.h"
#include "../utils_gfx/pix_convert.h"

#include "../rsc_processing/progress_bar.h"

void st_Win_Print_Degas(int16_t this_win_handle);
void _st_Read_Degas(int16_t this_win_handle, boolean file_process);

void st_Init_Degas(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_Degas;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_Degas(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_Degas(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_Degas(int16_t this_win_handle, boolean file_process){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        int8_t *data;
        u_int32_t data_size;
        u_int8_t *img_data;
        u_int16_t *img_palette, *img_resolution;
		u_int16_t width, height;   
        int16_t img_bpp;
        MFDB* img_mfdb;
        MFDB* MFDB32;

        int ok;
        const char *in_file;

        if(file_process == TRUE){
            in_file = this_win->wi_data->path;
            FILE* const in = fopen(in_file, "rb");
            if (!in) {
                fprintf(stderr, "cannot open input file '%s'\n", in_file);
                return;
            }
            fseek(in, 0, SEEK_END);
            data_size = ftell(in);
            fseek(in, 0, SEEK_SET);
            data = (int8_t *)mem_alloc(data_size);
            if (data == NULL) return;
            ok = (fread(data, data_size, 1, in) == 1);
            fclose(in);
            if (!ok) {
                fprintf(stderr, "Could not read %zu bytes of data from file %s\n",
                        data_size, in_file);
                mem_free(data);
                return;
            }
        } else {
            data = (int8_t *)this_win->wi_data->original_buffer;
            data_size = this_win->wi_data->STAT_FILE.st_size;            
        }

        img_resolution = (u_int16_t *)&data[0];
        img_palette = (u_int16_t *)&data[2];
        img_data = (u_int8_t*)&data[34];
        switch (img_resolution[0])
        {
        case 4:
            width = 640;
            height = 480;
            img_bpp = 4;
            img_mfdb = mfdb_alloc_bpp((int8_t*)img_data, width, height, img_bpp);
            MFDB32 =  st_MFDB4bpp_to_MFDB32(img_mfdb, (int16_t*)img_palette);
            break;
        case 2:
            width = 640;
            height = 400;
            img_bpp = 1;
            img_mfdb = mfdb_alloc_bpp((int8_t*)img_data, width, height, img_bpp);
            MFDB32 =  st_MFDB1bpp_to_MFDB32(img_mfdb);
            break;
        case 1:
            width = 640;
            height = 200;
            img_bpp = 2;
            img_mfdb = mfdb_alloc_bpp((int8_t*)img_data, width, height, img_bpp);
            break;
        case 0:
            width = 320;
            height = 200;
            img_bpp = 4;
            img_mfdb = mfdb_alloc_bpp((int8_t*)img_data, width, height, img_bpp);
            MFDB32 =  st_MFDB4bpp_to_MFDB32(img_mfdb, (int16_t*)img_palette);
            break;    
        default:
			sprintf(alert_message, "Format error\nRez value: %d", img_resolution[0]);
			st_form_alert(FORM_STOP, alert_message);           
            break;
        }

		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)MFDB32->fd_addr, width, height, 32);
        // this_win->wi_original_mfdb.fd_r3 = img_bpp;
        mfdb_free(img_mfdb);

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;	
	}

}

void st_Write_Degas(u_int8_t* src_buffer, int width, int height, const char* filename, int16_t* palette, int16_t nb_colors){

    struct_win_progress_bar* this_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(NIL, "DEGAS EXPORT", 30,  "DEGAS image encoding");

    u_int16_t *img_palette, img_resolution;
    int16_t img_bpp;
    MFDB* img_mfdb;

    MFDB* MFDB32 = mfdb_alloc_bpp((int8_t*)src_buffer, width, height, 32);

    size_t output_size = 0;

    u_int8_t *destination_buffer = NULL;

    if(width == 640 && height == 400){
        output_size = 32034;
        destination_buffer = (u_int8_t*)mem_alloc(output_size);
        img_bpp = 1;
        img_resolution = 2;
        img_palette = (u_int16_t*)mem_alloc(32);
        memset(img_palette, 0x00, 32);
        img_palette[0] = 0x0000; img_palette[1] = 0xFFFF;
        img_mfdb = st_MFDB32_To_MFDB1bpp(MFDB32);
    }else if(width == 320 && height == 200){
        output_size = 32034;
        destination_buffer = (u_int8_t*)mem_alloc(output_size);
        img_bpp = 4;
        img_resolution = 0x0000;
        img_palette = (u_int16_t*)mem_alloc(32);
        memset(img_palette, 0x00, 32);
        memcpy(img_palette, palette_ori, 32 );
        MFDB32->fd_r2 = img_bpp;
        img_mfdb = st_MFDB32_To_MFDB4bpp(MFDB32);
    }else if(width == 640 && height == 480){
        output_size = 153634; // -> palette == 16 word
        destination_buffer = (u_int8_t*)mem_alloc(output_size);
        img_bpp = 4;
        img_resolution = 0x0004;
        img_palette = (u_int16_t*)mem_alloc(32);
        memset(img_palette, 0x00, 32);
        memcpy(img_palette, palette_ori, 32 );
        MFDB32->fd_r2 = img_bpp;
        img_mfdb = st_MFDB32_To_MFDB4bpp(MFDB32);
    }else if(width == 320 && height == 480){
        output_size = 154114; // -> palette == 256 word
        destination_buffer = (u_int8_t*)mem_alloc(output_size);
        img_bpp = 8;
        img_resolution = 0x0007;
        img_palette = (u_int16_t*)mem_alloc(512);
        st_Save_Pal((int16_t*)img_palette, 256);
        MFDB32->fd_r2 = img_bpp;
        img_mfdb = st_MFDB32_To_MFDB8bpp(MFDB32);
    }else{
        sprintf(alert_message, "Format error\nW%dxH%d", width, height);
        st_form_alert(FORM_STOP, alert_message);
        
    }
    if(destination_buffer != NULL){
        int8_t *ptr_palette, *ptr_bpp, *ptr_data;
        u_int8_t *ptr_dst_data;

        int16_t width_size = width / (8 / img_bpp);

        ptr_palette = (int8_t*)img_palette;
        ptr_bpp = (int8_t*)&img_resolution;
        ptr_data = (int8_t*)img_mfdb->fd_addr;
        ptr_dst_data = &destination_buffer[34];
        memcpy(&destination_buffer[0], ptr_bpp, 2);
        memcpy(&destination_buffer[2], ptr_palette, 32);

        int16_t x, y, i, j;
        for(y = 0; y < height; y++){
                memcpy(&ptr_dst_data[ y * width_size ], &ptr_data[ y * width_size], width_size);
        }

        st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 70, "Degas: Writing pixels");

        FILE *fp;
        if((fp = fopen(filename,"wb")) == NULL){
            sprintf(alert_message, "Unable to open %s for writing", filename);
            st_form_alert(FORM_STOP, alert_message);

            goto clean;
        } else {
            int16_t err = fwrite(destination_buffer, output_size, 1, fp);
            if( err != 1) {
                sprintf(alert_message, "Write error\nerr %d\n%s", err, filename);
                st_form_alert(FORM_STOP, alert_message);
                goto clean;
            }
            fclose(fp);
        }
        mem_free(img_palette);
        mem_free(MFDB32);
    }
clean:        

    st_Win_Progress_Bar_Finish(this_progress_bar->win_form_handle);

    return ;
}
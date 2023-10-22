#include "img_webp.h"
#include "../img_handler.h"
#include "../utils_gfx/pix_convert.h"
#include "../utils/utils.h"

#include <webp/encode.h>
#include <webp/decode.h>
#include <webp/demux.h>

void st_Win_Print_WEBP(int16_t this_win_handle);
void _st_Read_WEBP(int16_t this_win_handle, boolean file_process);
void st_Win_Video_WEBP(int16_t this_win_handle);

void st_Init_Vid_WEBP(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Video_WEBP;
    if(this_win->wi_data->video_media){
        this_win->wi_data->img.img_id = 0;
        this_win->wi_data->img.img_index = 1; 
    } else {
        /* Progress Bar Stuff */
        this_win->wi_progress_bar = global_progress_bar;
    }
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Video_WEBP(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    // printf("--> IsAnimated %d <--\n", st_Detect_Webp_Animated(this_win_handle));
    if(this_win->wi_data->video_media){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }else{
        if(this_win->wi_data->stop_original_data_load == FALSE){
            this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
        }
        _st_Read_WEBP(this_win_handle, this_win->prefers_file_instead_mem);        
    }

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void *st_Win_Play_WEBP_Video(void *_this_win_handle){
    int16_t this_win_handle = *(int16_t*)_this_win_handle;    
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    u_int32_t time_start, time_end, duration, delay;

    WebPData webp_data;
    WebPDataInit(&webp_data);

    FILE* in = fopen(this_win->wi_data->path, "rb");
    fseek(in, 0, SEEK_END);
    size_t file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    uint8_t* file_data = (uint8_t*)WebPMalloc(file_size + 1);
    int ok = (fread(file_data, file_size, 1, in) == 1);
    fclose(in);
    file_data[file_size] = '\0';  // convenient 0-terminator
    webp_data.bytes = file_data;
    webp_data.size = file_size;
    WebPDemuxer* dmuxer = WebPDemux(&webp_data);

    this_win->wi_data->img.img_total = WebPDemuxGetI(dmuxer, WEBP_FF_FRAME_COUNT);
    uint32_t width = WebPDemuxGetI(dmuxer, WEBP_FF_CANVAS_WIDTH);
    uint32_t height = WebPDemuxGetI(dmuxer, WEBP_FF_CANVAS_HEIGHT);

    // printf("Total images %d, w %lu, h %lu\n", this_win->wi_data->img.img_total);

    int frame_idx = 1;
    WebPIterator iter;
    uint8_t* decode_data;

    u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
    if(destination_buffer == NULL){
        sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
        st_form_alert(FORM_EXCLAM, alert_message);
    }
    if(this_win->wi_original_mfdb.fd_addr != NULL){
        mem_free(this_win->wi_original_mfdb.fd_addr);
    }
    mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
    st_MFDB_Fill(&this_win->wi_original_mfdb, 0XFFFFFFFF);

    this_win->wi_data->img.scaled_pourcentage = 0;
    this_win->wi_data->img.rotate_degree = 0;
    this_win->wi_data->resized = FALSE;
    this_win->wi_data->img.original_width = width;
    this_win->wi_data->img.original_height = height;

    this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
    this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
    this_win->wi_data->stop_original_data_load = TRUE;
    this_win->wi_data->wi_buffer_modified = FALSE;

    this_win->refresh_win(this_win->wi_handle);

restart:

    while( (this_win->wi_data->img.img_id < this_win->wi_data->img.img_total) && this_win->wi_data->wi_pth != NULL ){
        if(this_win->wi_data->play_on || this_win->wi_data->img.img_id == 0){
            time_start = clock();        

        WebPDemuxGetFrame(dmuxer, this_win->wi_data->img.img_id, &iter);

        WebPDecodeARGBInto((uint8_t*)iter.fragment.bytes, iter.fragment.size, destination_buffer, (MFDB_STRIDE(width) * height) << 2, MFDB_STRIDE(width) << 2);

        delay = iter.duration;
        frame_idx++;

       
            time_end = clock();
            duration = 5 * (time_end - time_start);

            while( duration <  delay){
                duration = 5 * (clock() - time_start);
            }
            if(screen_workstation_bits_per_pixel != 32){
                this_win->wi_data->wi_buffer_modified = FALSE;
                this_win->wi_data->remap_displayed_mfdb = TRUE;
                this_win->refresh_win(this_win->wi_handle);
            }
            st_Control_Bar_Refresh_MFDB(this_win->wi_control_bar, this_win->wi_to_display_mfdb, this_win->current_pos_x, this_win->current_pos_y, this_win->work_area.g_w, this_win->work_area.g_h);          
            send_message(this_win_handle, WM_REDRAW);
            this_win->wi_data->img.img_id++;
            this_win->wi_data->img.img_index = this_win->wi_data->img.img_id + 1;
            pthread_yield_np();
        }
        if(this_win->wi_data->img.img_id == (this_win->wi_data->img.img_total - 1)){
            this_win->wi_data->img.img_id = 0;
            this_win->wi_data->img.img_index = this_win->wi_data->img.img_id + 1;
            goto restart;
        }
        pthread_yield_np();
    }
    WebPDemuxReleaseIterator(&iter);
    WebPDataClear(&webp_data);
    WebPDemuxDelete(dmuxer);
    send_message(this_win_handle, WM_CLOSED);
    return NULL;     

}


void _st_Read_WEBP(int16_t this_win_handle, boolean file_process)
{
	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }
    if(this_win->wi_data->stop_original_data_load == FALSE){
        st_Progress_Bar_Add_Step(this_win->wi_progress_bar);
        st_Progress_Bar_Init(this_win->wi_progress_bar, (int8_t*)"WEBP DECODING");
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 15, (int8_t*)"Init");

        int8_t *data;
        uint32_t data_size;
        int ok;
        const char *in_file;

        // The first step is to declare the configuration and properties for webp decoding 
        VP8StatusCode status;
        WebPDecoderConfig config;
        WebPBitstreamFeatures* const input = &config.input;
        WebPDecBuffer* const output = &config.output;

        // The second step initializes webp decoding configuration information 
        if (!WebPInitDecoderConfig(&config)) {
        fprintf(stderr, " Library version mismatch!\n " );
            return;
        }
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

        // The third step is to get webp image data 
        status = WebPGetFeatures((uint8_t*)data, (size_t)data_size, &config.input);
        if (status != VP8_STATUS_OK) {
        fprintf(stderr, " VP8_STATUS NOK!\n " );
            return;
        }

        int16_t nb_components_32bits = 4;
        int16_t width = input->width;
        int16_t height = input->height;

        this_win->wi_data->img.scaled_pourcentage = 0;
        this_win->wi_data->img.rotate_degree = 0;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->resized = FALSE;
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        u_int32_t total_pixels = width * height;
        u_int8_t* temp_buffer = (u_int8_t*)mem_alloc((width * height) << 2);
        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_EXCLAM, alert_message);
        } else {
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, nb_components_32bits << 3);
            WebPDecodeARGBInto((u_int8_t*)data, data_size, temp_buffer, (MFDB_STRIDE(width) * height) << 2, MFDB_STRIDE(width) << 2);
            u_int32_t *dest_ptr = (u_int32_t*)destination_buffer;
            u_int32_t *src_ptr = (u_int32_t*)temp_buffer;
            u_int32_t index = 0;
            while(index < total_pixels){
                *dest_ptr++ = st_Blend_Pix(*dest_ptr, *src_ptr++);
                index++;
            }
            this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
            this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
            this_win->wi_data->stop_original_data_load = TRUE;
        }

        if(file_process == TRUE){
            mem_free(data);
        }
        st_Progress_Bar_Signal(this_win->wi_progress_bar, 100, (int8_t*)"Finished");
        st_Progress_Bar_Step_Done(this_win->wi_progress_bar);
        st_Progress_Bar_Finish(this_win->wi_progress_bar);
    }
}

void st_Write_WEBP(u_int8_t* src_buffer, int width, int height, const char* filename){
    int16_t quality = 100;
    int16_t nb_components24b = 3;
    u_int8_t *destination_buffer = NULL;

    int16_t ret_value = 0;

    st_Progress_Bar_Add_Step(global_progress_bar);
    st_Progress_Bar_Init(global_progress_bar, (int8_t*)"WEBP EXPORT START");
    st_Progress_Bar_Signal(global_progress_bar, 35, (int8_t*)"WebP image encoding");

    size_t output_size = WebPEncodeRGB(src_buffer, width, height, MFDB_STRIDE(width) * nb_components24b, quality, &destination_buffer);

    st_Progress_Bar_Signal(global_progress_bar, 75, (int8_t*)"Saving image file");
    FILE *fp;
	if((fp = fopen(filename,"wb")) == NULL){
		sprintf(alert_message, "Unable to open %s for writing", filename);
		st_form_alert(FORM_STOP, alert_message);
        ret_value = 1;
		goto clean;
	} else {
		int16_t err = fwrite(destination_buffer, output_size, 1, fp);
		if( err != 1) {
			sprintf(alert_message, "Write error\nerr %d\n%s", err, filename);
			st_form_alert(FORM_STOP, alert_message);
            ret_value = 1;			
		}
		fclose(fp);
	}
clean:        
    st_Progress_Bar_Signal(global_progress_bar, 100, (int8_t*)"Finished");
    st_Progress_Bar_Step_Done(global_progress_bar);
    st_Progress_Bar_Finish(global_progress_bar);        
    return ;
}

bool st_Detect_Webp_Animated(int16_t this_win_handle){
	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return false;
    }

    bool result = false;
    char buf[5] = {'\0'};

    FILE* fd = fopen(this_win->wi_data->path, "rb");
    fseek(fd, 12, SEEK_CUR);
    fread(buf, 1, 4, fd);
    if(!strcmp(buf, "VP8X")){
        fseek(fd, 4, SEEK_CUR);
        uint8_t myByte;
        fread(&myByte, 1, 1, fd);
        int a = myByte;
        result = ((a >> 1) & 1) ? true : false;
    }
    fclose(fd);
    return result;
}
#include "img_webp.h"
#include <webp/decode.h>
#include <webp/encode.h>
#include "../utils/utils.h"
#include "../img_handler.h"

void st_Win_Print_WEBP(int16_t this_win_handle);
void _st_Read_WEBP(int16_t this_win_handle, boolean file_process);

void st_Init_WEBP(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->needs_refresh = TRUE;
	this_win->refresh_win = st_Win_Print_WEBP;
    this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    /* Progress Bar Stuff */
    this_win->wi_progress_bar = global_progress_bar;
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
}

void st_Win_Print_WEBP(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->wi_original_modified == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_WEBP(this_win_handle, this_win->prefers_file_instead_mem);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_WEBP(int16_t this_win_handle, boolean file_process)
{
	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }
    if(this_win->wi_data->wi_original_modified == FALSE){
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
        this_win->wi_data->img.original_width = width;
        this_win->wi_data->img.original_height = height;

        uint8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, nb_components_32bits << 3);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * nb_components_32bits);
            st_form_alert(FORM_EXCLAM, alert_message);
        } else {
            mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, nb_components_32bits << 3);
            WebPDecodeARGBInto((uint8_t*)data, data_size, destination_buffer, (MFDB_STRIDE(width) * height) << 2, MFDB_STRIDE(width) << 2);
            this_win->total_length_w = this_win->wi_original_mfdb.fd_w;
            this_win->total_length_h = this_win->wi_original_mfdb.fd_h;
            this_win->wi_data->wi_original_modified = TRUE;
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

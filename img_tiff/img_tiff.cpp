#include "img_tiff.h"

#ifdef WITH_TIFF

#include <tiffio.h>

#include "../utils/utils.h"
#include "../img_handler.h"

#include "../utils_gfx/pix_convert.h"
#include "../thumbs/thumbs.h"

#include "../rsc_processing/progress_bar.h"

#define USE_LZW_COMPRESSION 1
#define PRIMARY_IMAGE_ID    -1

void st_Win_Print_TIFF(int16_t this_win_handle);
void _st_Read_TIFF(int16_t this_win_handle, boolean file_process, int16_t img_id );
u_int16_t _st_Count_TIFF_Directories(TIFF *tiff_handler);
void _st_Handle_Thumbs_TIFF(int16_t this_win_handle, boolean file_process);

void st_Init_TIFF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_TIFF;

    this_win->prefers_file_instead_mem = TRUE; /* If FALSE the original file will be copied to memory and available in this_win->wi_data->original_buffer */
    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }      
    /* thumbnails stuff */
    if(this_win->wi_thumb == NULL){
        _st_Handle_Thumbs_TIFF(this_win->wi_handle, this_win->prefers_file_instead_mem);
    }    
}

void st_Win_Print_TIFF(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_TIFF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);
    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void st_Write_TIFF(u_int8_t* src_buffer, int width, int height, const char* filename){

    struct_win_progress_bar* this_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(NIL, "TIFF WRITING", 10,  "TIFF image encoding");

    u_int32_t* buffer = (u_int32_t*)src_buffer;
    TIFF *image = TIFFOpen(filename, "wb");

    st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 20, "TIFF: Setting TAGs");

    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, width); 
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, height); 
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8); 
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 3); 
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, height);   
    TIFFSetField(image, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);

    #if USE_LZW_COMPRESSION
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    TIFFSetField(image, TIFFTAG_PREDICTOR, PREDICTOR_HORIZONTAL);
    #else
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    #endif

    st_Win_Progress_Bar_Update_Info_Line(this_progress_bar, 60, "TIFF: Writing scanlines");

    //Now writing image to the file one strip at a time
    u_int32_t i;
    for(i = 0; i < height; i++)
    {
        TIFFWriteScanline(image, &src_buffer[(i + 1) * 3 * MFDB_STRIDE(width) ], i, 0);
    }

    // TIFFWriteEncodedStrip(image, 0, buffer, width * height * 3);

    st_Win_Progress_Bar_Finish(this_progress_bar->win_form_handle);

    TIFFWriteDirectory(image);
    TIFFClose(image);
}

void _st_Read_TIFF(int16_t this_win_handle,  boolean file_process, int16_t img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);
    if(this_win->wi_data->stop_original_data_load == FALSE){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "TIFF READING", 15,  "Starting...");

        TIFF *tiff_handler;
        u_int32_t width, height;

        u_int16_t compression, bitpersample, samplesperpixel;
        int16_t x, y;   

        const char *file_name;

        file_name = this_win->wi_data->path;

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 35, "TIFF: Reading file");

        if( file_process == TRUE && ( tiff_handler = TIFFOpen( file_name, "r")) == NULL){
            form_alert(1, "[1][File could not be opened for reading][Okay]"); 
            return;
        }
        if(img_id > PRIMARY_IMAGE_ID){
            TIFFSetDirectory(tiff_handler, img_id);
        }
        
        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 55, "TIFF: Parsing header data");

        TIFFGetField( tiff_handler, TIFFTAG_IMAGEWIDTH, &width);
        TIFFGetField( tiff_handler, TIFFTAG_IMAGELENGTH, &height);
        TIFFGetField( tiff_handler, TIFFTAG_COMPRESSION, &compression);
        TIFFGetField( tiff_handler, TIFFTAG_BITSPERSAMPLE, &bitpersample);
        TIFFGetField( tiff_handler, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 65, "TIFF: Parsing pixels data");

        u_int32_t* raster = ( u_int32_t* )mem_alloc( ( width * height ) << 2);
        if(raster == NULL){
            TIFFClose(tiff_handler);
            return;	
        }
        TIFFReadRGBAImageOriented(tiff_handler, width, height, raster, ORIENTATION_TOPLEFT, 0);

        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", (width * height) << 2);
            st_form_alert(FORM_EXCLAM, alert_message);
            TIFFClose(tiff_handler);
            return;	
        } 

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 85, "TIFF: Building ARGB pixels");

        mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t*)destination_buffer, width, height, 32);
        st_MFDB_Fill(&this_win->wi_original_mfdb, 0X00FFFFFF);
        long i, j;

        for(y = 0; y < height; y++){
            for(x = 0; x < width; x++){
                i = (x + y * MFDB_STRIDE(width)) << 2;
                j = (x + y * width);
                u_int32_t *ptr_dest = (u_int32_t*)&destination_buffer[i];
                u_int32_t src = TIFFGetA(raster[j]) << 24 | TIFFGetR(raster[j]) << 16 | TIFFGetG(raster[j]) << 8 | TIFFGetB(raster[j]);
                *ptr_dest = st_Blend_Pix(*ptr_dest, src);                
                // destination_buffer[i++] = TIFFGetA(raster[j]);
                // destination_buffer[i++] = TIFFGetR(raster[j]);
                // destination_buffer[i++] = TIFFGetG(raster[j]);
                // destination_buffer[i++] = TIFFGetB(raster[j]);      
            }
        }

        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

        mem_free(raster);
        TIFFClose(tiff_handler);

        st_Win_Progress_Bar_Finish(this_win->wi_handle);
    }
}

u_int16_t _st_Count_TIFF_Directories(TIFF *tiff_handler){
	u_int16_t dircount = 0;
	do {
	    dircount++;
	} while ( TIFFReadDirectory(tiff_handler) );
	return dircount;
}

void _st_Handle_Thumbs_TIFF(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

	const char *file_name;
    TIFF *tiff_handler;
    u_int32_t original_width;
    u_int32_t original_height;
    u_int16_t idx = 0;

    if( file_process == TRUE ){
        file_name = this_win->wi_data->path;
        tiff_handler = TIFFOpen( file_name, "r");
    }

    this_win->wi_data->img.img_total = _st_Count_TIFF_Directories(tiff_handler);
    this_win->wi_data->img.img_id = idx;
    this_win->wi_data->img.img_index = idx + 1;
    if(this_win->wi_data->img.img_total > 1){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "TIFF Thumbs processing", 1,  "Starting...");

        this_win->wi_data->thumbnail_slave = true;
        this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, 4, 8, 120, 80);

        this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
        struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
        struct_st_thumbs_list* prev_thumb_ptr = NULL;

        this_win->wi_thumb->thumbs_area_w = 0;
        this_win->wi_thumb->thumbs_area_h = this_win->wi_thumb->pady;
        this_win->wi_thumb->thumbs_nb = this_win->wi_data->img.img_total;

        for (int16_t i = 0; i < this_win->wi_thumb->thumbs_nb; i++) {
        
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

            TIFFSetDirectory(tiff_handler, i);

            char progess_bar_indication[96];
            sprintf(progess_bar_indication, "TIFF Thumb id.%d/%d - Image id.%d", i, this_win->wi_thumb->thumbs_nb, thumb_ptr->thumb_id);

            st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), progess_bar_indication);

            TIFFGetField( tiff_handler, TIFFTAG_IMAGEWIDTH, &original_width);
            TIFFGetField( tiff_handler, TIFFTAG_IMAGELENGTH, &original_height);

            u_int32_t* raster = ( u_int32_t* )mem_alloc( ( original_width * original_height ) << 2);
            TIFFReadRGBAImageOriented(tiff_handler, original_width, original_height, raster, ORIENTATION_TOPLEFT, 0);

            u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(original_width, original_height, 32);
            MFDB* temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, original_width, original_height, 32);
            st_MFDB_Fill(temp_mfdb, 0x00FFFFFF);
            long ii, jj, x, y;

            for(y = 0; y < original_height; y++){
                for(x = 0; x < original_width; x++){
                    ii = (x + y * MFDB_STRIDE(original_width)) << 2;
                    jj = (x + y * original_width);
                    u_int32_t *ptr_dest = (u_int32_t*)&temp_buffer[ii];
                    u_int32_t src = TIFFGetA(raster[jj]) << 24 | TIFFGetR(raster[jj]) << 16 | TIFFGetG(raster[jj]) << 8 | TIFFGetB(raster[jj]);
                    *ptr_dest = st_Blend_Pix(*ptr_dest, src);
                    // temp_buffer[ii++] = TIFFGetA(raster[jj]);
                    // temp_buffer[ii++] = TIFFGetR(raster[jj]);
                    // temp_buffer[ii++] = TIFFGetG(raster[jj]);
                    // temp_buffer[ii++] = TIFFGetB(raster[jj]);        
                }
            }

            int16_t new_width;
            int16_t new_height;

            if (original_width  > this_win->wi_thumb->thumb_w_size || original_height > this_win->wi_thumb->thumb_h_size){
                float factor_h, factor_v;
                factor_h = original_width  / (float)this_win->wi_thumb->thumb_w_size;
                factor_v = original_height / (float)this_win->wi_thumb->thumb_h_size;

                if (factor_v > factor_h) {
                    new_height = this_win->wi_thumb->thumb_h_size;
                    new_width  = original_width / factor_v;
                } else {
                    new_height = original_height / factor_h;
                    new_width  = this_win->wi_thumb->thumb_w_size;
                }
            }

            u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(new_width, new_height, 32);
            MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, new_width, new_height, 32);

            st_Rescale_ARGB(temp_mfdb, thumb_original_mfdb, new_width, new_height);
            mfdb_free(temp_mfdb);

            mem_free(raster);
            if(screen_workstation_bits_per_pixel != 32){
                thumb_ptr->thumb_mfdb = this_win->render_win(thumb_original_mfdb);
                mfdb_free(thumb_original_mfdb);
            } else {
                thumb_ptr->thumb_mfdb = thumb_original_mfdb;
            }
            thumb_ptr->thumb_visible = true;
            thumb_ptr->thumb_selectable = true;
            this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + new_width, this_win->wi_thumb->thumbs_area_w);
            this_win->wi_thumb->thumbs_area_h += new_height + this_win->wi_thumb->pady;
            thumb_ptr->thumb_selected = FALSE;

            prev_thumb_ptr = thumb_ptr;
            thumb_ptr = NULL;
        }
        this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;

        st_Win_Progress_Bar_Finish(this_win->wi_handle);

    } else {
        this_win->wi_data->thumbnail_slave = false;
        this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
    }
    TIFFClose(tiff_handler);
}

#endif
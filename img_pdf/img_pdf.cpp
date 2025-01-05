#include "img_pdf.h"
#ifdef WITH_XPDF
#include "../img_handler.h"

#include "../utils/utils.h"

#include "../thumbs/thumbs.h"
#include "../utils_gfx/pix_convert.h"
#include "../utils_gfx/ttf.h"

#include "../rsc_processing/progress_bar.h"

#define PRIMARY_IMAGE_ID    -1

/* XPDF */
// typedef struct {
//     char *generic;
//     char *fontname;
//     char *fontpath;
// } struct_img_pdf_fonts_array;


#include <xpdf_mint/goo/GString.h>

#include <xpdf_mint/splash/SplashBitmap.h>
#include <xpdf_mint/splash/Splash.h>

#include <xpdf_mint/xpdf/GlobalParams.h>
#include <xpdf_mint/xpdf/Object.h>
#include <xpdf_mint/xpdf/PDFDoc.h>
#include <xpdf_mint/xpdf/SplashOutputDev.h>
#include <xpdf_mint/xpdf/Error.h>
#include <xpdf_mint/xpdf/config.h>

// static double resolution = 180;
static double resolution_w = 180;
static double resolution_h = 180;

static int rotate = 0;
static char ownerPassword[33] = "";
static char userPassword[33] = "";

// struct_img_pdf_fonts_array this_fonts_array[] = {
//     { "fontFile", "Times-Roman", "/fonts/n021003l.pfb"},
//     { "fontFile", "Times-Italic", "/fonts/n021023l.pfb"},
//     { "fontFile", "Times-Bold", "/fonts/n021004l.pfb"},
//     { "fontFile", "Times-BoldItalic", "/fonts/n021024l.pfb"},
//     { "fontFile", "Helvetica", "/fonts/n019003l.pfb"},
//     { "fontFile", "Helvetica-Oblique", "/fonts/n019023l.pfb"},
//     { "fontFile", "Helvetica-Bold", "/fonts/n019004l.pfb"},
//     { "fontFile", "Helvetica-BoldOblique", "/fonts/n019024l.pfb"},
//     { "fontFile", "Courier", "/fonts/n022003l.pfb"},
//     { "fontFile", "Courier-Oblique", "/fonts/n022023l.pfb"},
//     { "fontFile", "Courier-Bold", "/fonts/n022004l.pfb"},
//     { "fontFile", "Courier-BoldOblique", "/fonts/n022024l.pfb"},
//     { "fontFile", "Symbol", "/fonts/s050000l.pfb"},
//     { "fontFile", "ZapfDingbats", "/fonts/d050000l.pfb"},
// };
/* XPDF */
/*

*/
void st_Win_Print_PDF(int16_t this_win_handle);
void _st_Read_PDF(int16_t this_win_handle, boolean file_process, int16_t img_id);
void _st_Handle_Thumbs_PDF(int16_t this_win_handle, boolean file_process);
void _st_Handle_Thumbs_PDF_Generic(int16_t this_win_handle, boolean file_process);

void st_Init_PDF(struct_window *this_win){
    this_win->wi_data->image_media = TRUE;
    this_win->wi_data->doc_media = TRUE;
    this_win->wi_data->autoscale = TRUE;
    this_win->wi_data->window_size_limited = TRUE;
    this_win->wi_data->remap_displayed_mfdb = TRUE;
	this_win->refresh_win = st_Win_Print_PDF;

    this_win->prefers_file_instead_mem = TRUE;

    if(!st_Set_Renderer(this_win)){
        sprintf(alert_message, "screen_format: %d\nscreen_bits_per_pixel: %d", screen_workstation_format, screen_workstation_bits_per_pixel);
        st_form_alert(FORM_STOP, alert_message);
        return;
    }
    /* Pages stuff */
    if(this_win->wi_thumb == NULL){
       _st_Handle_Thumbs_PDF(this_win->wi_handle, this_win->prefers_file_instead_mem);
    }
}

void st_Win_Print_PDF(int16_t this_win_handle){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){
        this_win->wi_to_work_in_mfdb = &this_win->wi_original_mfdb;
    }

    _st_Read_PDF(this_win_handle, this_win->prefers_file_instead_mem, this_win->wi_data->img.img_id);

    if( st_Img32b_To_Window(this_win) == false ){
        st_form_alert(FORM_STOP, alert_message);
    }
}

void _st_Read_PDF(int16_t this_win_handle, boolean file_process, int16_t img_id){
    struct_window *this_win;
    this_win = detect_window(this_win_handle);

    if(this_win->wi_data->stop_original_data_load == FALSE){

        this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "PDF READING", 10,  "Starting...");
        
/* XPDF */
        GString *ownerPW = NULL;
        GString *userPW = NULL;
        PDFDoc *doc;
        int this_page = 1;

        if(img_id > PRIMARY_IMAGE_ID){
            this_page = img_id + 1;
        }

        const char *fileName = this_win->wi_data->path;
        
        SplashColor paperColor;
        SplashOutputDev *splashOut;

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 20, "PDF: Opening file");

        if(!(st_FileExistsAccess(fileName))){
            sprintf(alert_message, "File not found %s\n", fileName);
            st_form_alert(FORM_EXCLAM, alert_message);
        }

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 30, "PDF: Checking for fonts");

        /* Global Parameters */
        char conf_file[256] = {0};
        strcpy(conf_file,current_path);
        strcat(conf_file, "\\conf\\xpdfrc");
        if(st_FileExistsAccess(conf_file)){
            globalParams = new GlobalParams(conf_file);
        }else{
            sprintf(alert_message,"Conf file not found\n%s", conf_file);
            if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"Cancel", (char*)"Continue") == 1){
                return;
            }
        }

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 40, "PDF: Setting global parameters");

        globalParams->setupBaseFonts(current_path);

        globalParams->setEnableFreeType((char*)"yes");
        globalParams->setAntialias((char*)"yes");
        globalParams->setVectorAntialias((char*)"yes");

        globalParams->setPrintStatusInfo(gFalse);
        globalParams->setErrQuiet(gTrue);
        // globalParams->setPrintStatusInfo(gTrue);
        // globalParams->setErrQuiet(gFalse);

        // if (ownerPassword[0]) {
        //     ownerPW = new GString(ownerPassword);
        // }
        // if (userPassword[0]) {
        //     userPW = new GString(userPassword);
        // }

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 60, "PDF: Parsing data");

        doc = new PDFDoc((char*)fileName, ownerPW, userPW);
        // if(doc->isEncrypted()){
        //     sprintf(alert_message, "User/Password doc not supported yet!");
        //     st_form_alert(FORM_STOP, alert_message);
        //     return;
        // }
        // if (userPW) {
        //     delete userPW;
        // }
        // if (ownerPW) {
        //     delete ownerPW;
        // }
        if (!doc->isOk()) {
            sprintf(alert_message, "Error opening PDF document %s\n", fileName);
            st_form_alert(FORM_STOP, alert_message);
        }
        double hDPI, vDPI;
        double page_width = doc->getPageMediaWidth(this_page);
        double page_height = doc->getPageMediaHeight(this_page);

        sprintf(this_win->wi_name, "%s %d/%d", basename(this_win->wi_data->path), this_page, doc->getNumPages());
        wind_set_str(this_win->wi_handle, WF_NAME, this_win->wi_name);

        u_int16_t wanted_width = 480;
        u_int16_t wanted_height = 621;
        uint16_t this_win_height;
        uint16_t this_win_width;
        if(page_width < page_height){
            this_win_height = wanted_height;
            this_win_width = wanted_width;
        }else{
            this_win_height = wanted_width;
            this_win_width = wanted_height;
        }
        hDPI = MAX( (double)((72 * this_win_width) / page_width ) - 0.5, resolution_h );
        vDPI = MAX( (double)((72 * this_win_height) / page_height ) - 0.5, resolution_w);
       
/* Splash */

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 80, "PDF: Rendering document");

        paperColor[0] = paperColor[1] = paperColor[2] = 0xff;
        splashOut = new SplashOutputDev(splashModeRGB8, 1, gFalse, paperColor);
        splashOut->setNoComposite(gTrue);
        splashOut->startDoc(doc->getXRef());
        doc->displayPage(splashOut, this_page, hDPI, vDPI, 0, gFalse, gTrue, gFalse);
        SplashBitmap *bitmap = splashOut->getBitmap();
/* Splash */

/* XPDF */

		u_int16_t width = bitmap->getWidth();
		u_int16_t height = bitmap->getHeight();

        st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, 90, "PDF: Building ARGB pixels");

        void* old_ptr = this_win->wi_original_mfdb.fd_addr;
        u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
        if(destination_buffer == NULL){
            sprintf(alert_message, "Out Of Mem Error\nAsked for %doctets", width * height * 4);
            st_form_alert(FORM_EXCLAM, alert_message);
        }

        uint32_t *dst_ptr = (uint32_t*)destination_buffer;
        uint8_t *src_ptr = (uint8_t*)bitmap->getDataPtr();
        uint32_t i, j = 0, x, y;
     
        for(y = 0; y < height; y++){
            for(x = 0; x < width; x++){
                i = (x + y * MFDB_STRIDE(width));            
                dst_ptr[i++] = 0xFF << 24 | ( src_ptr[j++] & 0xFF ) << 16| ( src_ptr[j++] & 0xFF ) << 8 | ( src_ptr[j++] & 0xFF );
            }
        }

		mfdb_update_bpp(&this_win->wi_original_mfdb, (int8_t *)destination_buffer, width, height, 32);
        if(old_ptr != NULL){
            mem_free(old_ptr);
        }
        st_Win_Set_Ready(this_win, width, height);
        this_win->wi_data->stop_original_data_load = TRUE;

        st_Win_Progress_Bar_Finish(this_win->wi_handle);

    	delete splashOut;
        delete doc;
end_global_param:
        delete globalParams;    
           
	}
}

void _st_Handle_Thumbs_PDF(int16_t this_win_handle, boolean file_process){

	struct_window *this_win;
	this_win = detect_window(this_win_handle);
    if(this_win == NULL){
        return;
    }

/* XPDF */
    GString *ownerPW = NULL;
    GString *userPW = NULL;
    PDFDoc *doc;
    u_int16_t idx = 0;

    const char *fileName = this_win->wi_data->path;
    
    SplashColor paperColor;
    SplashOutputDev *splashOut;
    if(!(st_FileExistsAccess(fileName))){
        sprintf(alert_message, "File not found %s\n", fileName);
        st_form_alert(FORM_EXCLAM, alert_message);  
    }
    /* Global Parameters */
    char conf_file[256] = {0};
    strcpy(conf_file,current_path);
    strcat(conf_file, "\\conf\\xpdfrc");
    if(st_FileExistsAccess(conf_file)){
        globalParams = new GlobalParams(conf_file);
    }else{
        sprintf(alert_message,"Conf file not found\n%s", conf_file);
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"Cancel", (char*)"Continue") == 1){
            return;
        }
    }
    globalParams->setPrintStatusInfo(gFalse);
    globalParams->setErrQuiet(gTrue);
    if(ownerPassword[0]){ ownerPW = new GString(ownerPassword); }
    if(userPassword[0]){ userPW = new GString(userPassword); }
    doc = new PDFDoc((char*)fileName, ownerPW, userPW);
    if(userPW){delete userPW;}
    if(ownerPW){delete ownerPW;}
    if(!doc->isOk()){printf("Error opening PDF document %s\n", fileName);}

    this_win->wi_data->img.img_total = doc->getNumPages();
    this_win->wi_data->img.img_id = idx;
    this_win->wi_data->img.img_index = idx + 1;

    bool process_thumbs;
    if(cpu_type < 30){
        process_thumbs = FALSE;
    }else if(this_win->wi_data->img.img_total > 10){
        char this_alert[128] = {'\0'};
        sprintf(this_alert, "Process a preview for %d pages?", this_win->wi_data->img.img_total);
        if(st_form_alert_choice(FORM_QUESTION, this_alert, (char*)"Pages number", (char*)"Process previews") == 1){
            process_thumbs = FALSE;
        }else{
            process_thumbs = TRUE;
        }
    }else{
        process_thumbs = TRUE;
    }

    if(!process_thumbs){
        st_Thumb_List_Generic(this_win, "PDF Building pages index", "Page", 80, 20, 4, 4, FALSE);
    }else{
        if(this_win->wi_data->img.img_total > 1){

            this_win->wi_win_progress_bar = (struct_win_progress_bar*)st_Win_Progress_Init(this_win->wi_handle, "PDF Thumbs processing", 1,  "Starting...");

            paperColor[0] = paperColor[1] = paperColor[2] = 0xff;
            splashOut = new SplashOutputDev(splashModeRGB8, 1, gFalse, paperColor);
            splashOut->setNoComposite(gTrue);
            splashOut->startDoc(doc->getXRef());

            double page_width = doc->getPageMediaWidth(1);
            double page_height = doc->getPageMediaHeight(1);

            u_int16_t wanted_width = 100;
            u_int16_t wanted_height = 140;
            if(page_height < page_width){
                wanted_width = 140;
                wanted_height = 100;            
            }
            u_int16_t wanted_padx = 8;
            u_int16_t wanted_pady = 8;

            this_win->wi_data->thumbnail_slave = true;
            this_win->wi_thumb = st_Thumb_Alloc(this_win->wi_data->img.img_total, this_win_handle, wanted_padx, wanted_pady, wanted_width, wanted_height);

            this_win->wi_thumb->thumbs_list_array = (struct_st_thumbs_list*)mem_alloc(sizeof(struct_st_thumbs_list));
            struct_st_thumbs_list* thumb_ptr = this_win->wi_thumb->thumbs_list_array;
            struct_st_thumbs_list* prev_thumb_ptr = NULL;

            this_win->wi_thumb->thumbs_open_new_win = FALSE;
            
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

                char progess_bar_indication[96];
                sprintf(progess_bar_indication, "Thumbnail rendering for page %d/%d", i+1, this_win->wi_thumb->thumbs_nb, thumb_ptr->thumb_id);

                st_Win_Progress_Bar_Update_Info_Line(this_win->wi_win_progress_bar, (mul_100_fast(i) / this_win->wi_thumb->thumbs_nb), progess_bar_indication);

                page_width = doc->getPageMediaWidth(thumb_ptr->thumb_index);
                page_height = doc->getPageMediaHeight(thumb_ptr->thumb_index);

                u_int16_t this_win_height;
                u_int16_t this_win_width;
                if(page_width < page_height){
                    this_win_height = wanted_height;
                    this_win_width = wanted_width;
                }else{
                    this_win_height = wanted_width;
                    this_win_width = wanted_height;
                }

                // double hDPI = MAX( (double)((72 * this_win_width) / page_width ) - 0.5, 72 );
                // double vDPI = MAX( (double)((72 * this_win_height) / page_height ) - 0.5, 72);
                double hDPI = 72;
                double vDPI = 72;

                doc->displayPage(splashOut, thumb_ptr->thumb_index, hDPI, vDPI, 0, gFalse, gTrue, gFalse);
                SplashBitmap *bitmap = splashOut->getBitmap();

                int16_t old_width  = bitmap->getWidth();
                int16_t old_height = bitmap->getHeight();

                int16_t new_width = old_width;
                int16_t new_height = old_height;

                if (old_width  > this_win->wi_thumb->thumb_w_size || old_height > this_win->wi_thumb->thumb_h_size){
                    float factor_h, factor_v;
                    factor_h = old_width  / (float)this_win->wi_thumb->thumb_w_size;
                    factor_v = old_height / (float)this_win->wi_thumb->thumb_h_size;

                    if (factor_v > factor_h) {
                        new_height = this_win->wi_thumb->thumb_h_size;
                        new_width  = old_width / factor_v;
                    } else {
                        new_height = old_height / factor_h;
                        new_width  = this_win->wi_thumb->thumb_w_size;
                    }
                }

                u_int8_t* temp_buffer = st_ScreenBuffer_Alloc_bpp(old_width, old_height, 32);
                MFDB* temp_mfdb = mfdb_alloc_bpp((int8_t*)temp_buffer, old_width, old_height, 32);
                u_int32_t *dst_ptr = (u_int32_t *)temp_buffer;
                u_int8_t *src_ptr = (uint8_t*)bitmap->getDataPtr();
                u_int32_t ii, jj = 0, x, y;            
                for(y = 0; y < old_height; y++){
                    for(x = 0; x < old_width; x++){
                        ii = (x + y * MFDB_STRIDE(old_width));            
                        dst_ptr[ii++] = 0XFF << 24 | ( src_ptr[jj++] & 0xFF ) << 16| ( src_ptr[jj++] & 0xFF ) << 8 | ( src_ptr[jj++] & 0xFF );
                    }
                }

                u_int8_t* destination_buffer = st_ScreenBuffer_Alloc_bpp(new_width, new_height, 32);
                MFDB* thumb_original_mfdb = mfdb_alloc_bpp( (int8_t*)destination_buffer, new_width, new_height, 32);

                st_Rescale_ARGB(temp_mfdb, thumb_original_mfdb, new_width, new_height);
                MFDB* thumb_final_mfdb = thumb_original_mfdb;

                char thumb_txt[10] = {'\0'};
                char font_path[strlen(current_path) + strlen(TTF_DEFAULT_PATH) + 1] = {'\0'};
                strcpy(font_path, current_path);
                strcat(font_path, TTF_DEFAULT_PATH);            
                sprintf(thumb_txt,"%d", thumb_ptr->thumb_index );
                print_TTF((thumb_original_mfdb->fd_w >> 1) - 4, thumb_original_mfdb->fd_h - 4, thumb_original_mfdb, font_path, 14, thumb_txt);

                if(screen_workstation_bits_per_pixel != 32){
                    thumb_ptr->thumb_mfdb = this_win->render_win(thumb_final_mfdb);
                    mfdb_free(thumb_final_mfdb);
                } else {
                    thumb_ptr->thumb_mfdb = thumb_final_mfdb;
                }

                mfdb_free(temp_mfdb);

                thumb_ptr->thumb_visible = true;
                thumb_ptr->thumb_selectable = true;
                this_win->wi_thumb->thumbs_area_w = MAX( (this_win->wi_thumb->padx << 1) + wanted_width, this_win->wi_thumb->thumbs_area_w);
                this_win->wi_thumb->thumbs_area_h += wanted_height + this_win->wi_thumb->pady;
                thumb_ptr->thumb_selected = FALSE;

                prev_thumb_ptr = thumb_ptr;
                thumb_ptr = NULL;
            }

            st_Win_Progress_Bar_Finish(this_win->wi_handle);

            this_win->wi_thumb->thumbs_area_h += this_win->wi_thumb->pady;
            this_win->wi_thumb->thumbs_list_array->thumb_selected = TRUE;
            delete splashOut;
        } else {
            this_win->wi_data->thumbnail_slave = false;
            this_win->wi_data->img.img_id = PRIMARY_IMAGE_ID;
        }
    }
    delete doc;
    delete globalParams; 
}

#endif
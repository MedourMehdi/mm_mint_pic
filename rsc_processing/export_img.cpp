#include "export_img.h"
#include "rsc_def.h"
#include "rsc_common.h"

#include "../file.h"
#include "../utils/utils.h"
#include "../utils_gfx/pix_convert.h"

#include "../img_png/img_png.h"
#include "../img_heif/img_heif.h"
#include "../img_webp/img_webp.h"
#include "../img_jpeg/img_jpeg.h"
#include "../img_tiff/img_tiff.h"
#include "../img_bmp/img_bmp.h"
#include "../img_tga/img_tga.h"
#include "../img_pi/img_pi.h"
#include "../img_psd/img_psd.h"

typedef struct {
    char        export_path[256];
    char        export_dir[192];
    char        file_without_ext[64];
    char        export_extension[6];
    char        export_comments[64];
    int16_t     export_width;
    int16_t     export_height;
    int16_t     export_components;
    u_int8_t    *export_data;
    void        *(*export_function)(void*);
} struct_export;

struct_export this_export = {'\0'};

boolean st_Set_Export(void* (*export_function)(void*), const char* this_extention, OBJECT* this_ftext_to_uptdate);

boolean st_Set_Export(void* (*export_function)(void*), const char* this_extention, OBJECT* this_ftext_to_uptdate){
    if(this_export.file_without_ext[0] == '\0'){
        sprintf(alert_message, "%s", "Please, select destination");
        st_form_alert(FORM_EXCLAM,alert_message);
        return false;
    }else{
        this_export.export_function = export_function;
        strcpy(this_export.export_path, this_export.export_dir);
        strcpy(this_export.export_extension, this_extention);
        strcat(this_export.export_path, this_export.file_without_ext);
        strcat(this_export.export_path, this_export.export_extension);        
        shrink_char_obj(this_export.export_path, this_ftext_to_uptdate);
        return true;
    }
}

void st_Update_Comments(int16_t this_win_form_handle, void* p_param, OBJECT* this_ftext_to_uptdate, u_int16_t bpp, const char* format ){

    struct_export* my_export = (struct_export*)p_param;
    struct_window* this_win_form = detect_window(this_win_form_handle);
    OBJECT* tree = this_win_form->wi_data->rsc.tree;
    struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);

    char extended_info[12] = {0};
    if(bpp < 8){
        sprintf(extended_info, "Bitpl." );
    }else{
        sprintf(extended_info, "Bpp" );
    }
    int16_t width, height;

    if(!strcasecmp(format, "Degas")){
        switch (bpp)
        {
        case 4:
            width = 320; height = 200;
            break;
        case 1:
            width = 640; height = 400;
            break;   
        default:
            break;
        }
    } else {
        width = this_win_master->total_length_w; height = this_win_master->total_length_h;
    }

    sprintf( my_export->export_comments, "%s/%d%s %dpx/%dpx", 
        format, bpp, extended_info, width, height);

    shrink_char_obj(this_export.export_comments, this_ftext_to_uptdate);
    
    st_Refresh_Object(this_ftext_to_uptdate, this_win_form);
}

void* st_Image_Export_To_PSD(void* p_param){
    #ifdef WITH_PSD
    struct_export* my_export = (struct_export*)p_param;

    u_int8_t* raw_data = my_export->export_data;

    switch (my_export->export_components)
    {
    case 4: /* 32 bits per pixels */
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", my_export->export_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;    
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }
    st_Write_PSD(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export PSD done][Okay]");
    #else
    form_alert(1, "[1][Built without PSD support][Okay]");
    #endif
    return NULL;
}

void* st_Image_Export_To_Degas(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    u_int16_t width;
    u_int16_t height;
    int16_t bpp;
    int16_t nb_colors;

    if(strcasecmp( my_export->export_extension, ".pi1") == 0){
        width = 320;
        height = 200;
        bpp = 4;
        nb_colors = 16;
    }else if(strcasecmp( my_export->export_extension, ".pi5") == 0){
        width = 320;
        height = 200;
        bpp = 4;
        nb_colors = 16;
    }else{
        width = 640;
        height = 400;
        bpp = 1;
        nb_colors = 2;
    }

    u_int8_t* raw_data;
    MFDB* MFDB32_src;
    MFDB* MFDB32_dst;
    u_int8_t* destination_buffer;

    switch (my_export->export_components)
    {
    case 4: /* 32 bits per pixels */
            MFDB32_src = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, 32);
            destination_buffer = st_ScreenBuffer_Alloc_bpp(width, height, 32);
            MFDB32_dst = mfdb_alloc_bpp( (int8_t*)destination_buffer, width, height, 32);

            st_Rescale_ARGB(MFDB32_src, MFDB32_dst, width, height);

            raw_data = (u_int8_t*)MFDB32_dst->fd_addr;
                       
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", my_export->export_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;    
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
            if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
                return NULL;
            }
    }
    st_Write_Degas(raw_data, width, height, my_export->export_path, palette_ori, nb_colors);

    mfdb_free(MFDB32_dst);
    mem_free(MFDB32_src);
    form_alert(1, "[1][Export Degas done][Okay]");
    return NULL;
}

void* st_Image_Export_To_HEIF(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    u_int8_t* raw_data = my_export->export_data;

    switch (my_export->export_components)
    {
    case 4: /* 32 bits per pixels */
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", my_export->export_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;    
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }
    st_Write_HEIF(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export HEIF done][Okay]");
    return NULL;
}

void* st_Image_Export_To_BMP(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    u_int8_t* raw_data = my_export->export_data;

    switch (my_export->export_components)
    {
    case 4: /* 32 bits per pixels */
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", my_export->export_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }

    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }
    st_Write_BMP(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export BMP done][Okay]");
    return NULL;
}

void* st_Image_Export_To_TGA(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    int16_t nb_components = my_export->export_components, nb_components_32bits = 4;

    u_int8_t* raw_data = my_export->export_data;
    u_int8_t* dst_buffer = NULL;

    MFDB* rgb888_mfdb = NULL;
    MFDB* rgb8888_mfdb = NULL;

    switch (nb_components)
    {
    case 4: /* 32 bits per pixels */
        rgb8888_mfdb = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, nb_components_32bits << 3);
        rgb888_mfdb = st_MFDB32_To_MFDB24(rgb8888_mfdb);
        rgb8888_mfdb->fd_addr = NULL;
        raw_data = (u_int8_t*)rgb888_mfdb->fd_addr;
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", nb_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }
    st_Write_TGA(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export TGA done][Okay]");
    if(rgb8888_mfdb != NULL){
        mfdb_free(rgb8888_mfdb);
    }
    if(rgb888_mfdb != NULL){
        mfdb_free(rgb888_mfdb);
    }
    return NULL;
}

void* st_Image_Export_To_PNG(void* p_param){
    struct_export* my_export = (struct_export*)p_param;
    int16_t nb_components = my_export->export_components, nb_components_24bits = 3, channel;
    int16_t bits_per_component;
    int16_t png_color_type, png_color_filter;

    u_int8_t* raw_data = my_export->export_data;

    channel = nb_components;

    switch (nb_components)
    {
    case 4: /* 32 bits per pixels */
        bits_per_component = 8;
        png_color_type = PNG_COLOR_TYPE_RGB_ALPHA;
        png_color_filter = PNG_TRANSFORM_SWAP_ALPHA; /* Aranym FVdi 32bpp is ARGB */
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", nb_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }    
    st_Save_PNG(my_export->export_path,my_export->export_width, my_export->export_height, bits_per_component, png_color_type, raw_data, ( channel * MFDB_STRIDE(my_export->export_width) ), png_color_filter);

    form_alert(1, "[1][Export PNG done][Okay]");
    return NULL;
}

void* st_Image_Export_To_TIFF(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    int16_t nb_components = my_export->export_components, nb_components_32bits = 4;

    u_int8_t* raw_data = my_export->export_data;
    u_int8_t* dst_buffer = NULL;

    MFDB* rgb888_mfdb = NULL;
    MFDB* rgb8888_mfdb = NULL;

    switch (nb_components)
    {
    case 4: /* 32 bits per pixels */
        rgb8888_mfdb = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, nb_components_32bits << 3);
        rgb888_mfdb = st_MFDB32_To_MFDB24(rgb8888_mfdb);
        rgb8888_mfdb->fd_addr = NULL;
        raw_data = (u_int8_t*)rgb888_mfdb->fd_addr;
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", nb_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }
    st_Write_TIFF(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export TIFF done][Okay]");
    if(rgb8888_mfdb != NULL){
        mfdb_free(rgb8888_mfdb);
    }
    if(rgb888_mfdb != NULL){
        mfdb_free(rgb888_mfdb);
    }
    return NULL;
}

void* st_Image_Export_To_WEBP(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    int16_t nb_components = my_export->export_components, nb_components_32bits = 4;

    u_int8_t* raw_data = my_export->export_data;
    u_int8_t* dst_buffer = NULL;

    MFDB* rgb888_mfdb = NULL;
    MFDB* rgb8888_mfdb = NULL;

    switch (nb_components)
    {
    case 4: /* 32 bits per pixels */
        rgb8888_mfdb = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, nb_components_32bits << 3);
        rgb888_mfdb = st_MFDB32_To_MFDB24(rgb8888_mfdb);
        rgb8888_mfdb->fd_addr = NULL; /* To be safe when we use mfdb_free */
        raw_data = (u_int8_t*)rgb888_mfdb->fd_addr;
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", nb_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }    
    st_Write_WEBP(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export WEBP done][Okay]");
    if(rgb8888_mfdb != NULL){
        mfdb_free(rgb8888_mfdb);
    }
    if(rgb888_mfdb != NULL){
        mfdb_free(rgb888_mfdb);
    }
    return NULL;
}

void* st_Image_Export_To_MFD(void* p_param){
    struct_export* my_export = (struct_export*)p_param;
    u_int8_t* raw_data;
    MFDB* dst_mfdb;
    MFDB* this_mfdb = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, my_export->export_components << 3);

    if(my_export->export_components == 4){
        switch (screen_workstation_bits_per_pixel)
        {
        case 32: /* 32 bits per pixel */
            dst_mfdb = this_mfdb;
            raw_data = my_export->export_data;
            break;
        case 24: /* 24 bits per pixel */
            dst_mfdb = st_MFDB32_To_MFDB24(this_mfdb);
            raw_data = (uint8_t*)dst_mfdb->fd_addr;
            break;
        case 16: /* 16 bits per pixel */
            dst_mfdb = st_MFDB32_To_MFDB16(this_mfdb);
            raw_data = (uint8_t*)dst_mfdb->fd_addr;
            break;
        case 8: /* 8 bits per pixel */
            dst_mfdb = st_MFDB32_To_MFDB8bpp(this_mfdb);
            raw_data = (uint8_t*)dst_mfdb->fd_addr;
            break;
        case 4: /* 4 bits per pixel */
            dst_mfdb = st_MFDB32_To_MFDB4bpp(this_mfdb);
            raw_data = (uint8_t*)dst_mfdb->fd_addr;
            break;            
        case 1: /* 1 bit per pixel */
            dst_mfdb = st_MFDB32_To_MFDB1bpp(this_mfdb);
            raw_data = (uint8_t*)dst_mfdb->fd_addr;
            break;                   
        default:
            sprintf(alert_message, "Hw bits per px %d\nFormat not supported", screen_workstation_bits_per_pixel);
            st_form_alert(FORM_STOP, alert_message);
            break;
        }
    } else {
            sprintf(alert_message, "Hw bits per px %d\nChannel %d\nFormat not supported", screen_workstation_bits_per_pixel, my_export->export_components);
            st_form_alert(FORM_STOP, alert_message);
    }

    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }

    if(mfdb_to_file(dst_mfdb, my_export->export_path) == true){
        sprintf(alert_message, "Export done\nNew width %dpx\nNew height %dpx\nBits per px %d", dst_mfdb->fd_w, dst_mfdb->fd_h, dst_mfdb->fd_nplanes);
    } else{
        sprintf(alert_message, "Data wasn't exported");
    }

    mem_free(this_mfdb); /* Do not mfdb_free as we must keep fd_addr */
    mfdb_free(dst_mfdb);
    st_form_alert(FORM_EXCLAM, alert_message);

    return NULL;
}

void* st_Image_Export_To_JPEG(void* p_param){
    struct_export* my_export = (struct_export*)p_param;

    int16_t nb_components = my_export->export_components, nb_components_32bits = 4;

    u_int8_t* raw_data = my_export->export_data;

    MFDB* rgb888_mfdb = NULL;
    MFDB* rgb8888_mfdb = NULL;

    switch (nb_components)
    {
    case 4: /* 32 bits per pixels */
        rgb8888_mfdb = mfdb_alloc_bpp((int8_t*)my_export->export_data, my_export->export_width, my_export->export_height, nb_components_32bits << 3);
        rgb888_mfdb = st_MFDB32_To_MFDB24(rgb8888_mfdb);
        rgb8888_mfdb->fd_addr = NULL;
        raw_data = (u_int8_t*)rgb888_mfdb->fd_addr;
        break;
    default:
        sprintf(alert_message,"Error\nnb_components are %d", nb_components);
        st_form_alert(FORM_EXCLAM, alert_message);
        return NULL;
        break;
    }
    if(st_FileExistsAccess(my_export->export_path) == 1){
        sprintf(alert_message,"File exist\nDo you want to erase it?");
        if(st_form_alert_choice(FORM_STOP, alert_message, (char*)"No", (char*)"Yes") == 1){
            return NULL;
        }
    }    
    st_Write_JPEG(raw_data, my_export->export_width, my_export->export_height, my_export->export_path);
    form_alert(1, "[1][Export JPEG done][Okay]");
    if(rgb8888_mfdb != NULL){
        mfdb_free(rgb8888_mfdb);
    }
    if(rgb888_mfdb != NULL){
        mfdb_free(rgb888_mfdb);
    }
    return NULL;
}

void process_diag_export(int16_t this_win_form_handle){
    char default_extension[5] = ".png";
    // char* env_aesdir = getenv("AESDIR");
    char file[64] = {'\0'}, final_path[256] = {'\0'};
    char ext[5] = {'\0'};

    struct_window* this_win_form = detect_window(this_win_form_handle);
    struct_window* this_win_master = detect_window(this_win_form->wi_data->rsc.win_master_handle);

    OBJECT* tree = this_win_form->wi_data->rsc.tree;
    OBJECT obj_gui_ftext_info = tree[DiagExport_TextInfo1];

    int16_t obj_pxy_info[4];
    obj_pxy_info[0] = obj_gui_ftext_info.ob_x + this_win_form->work_area.g_x;
    obj_pxy_info[1] = obj_gui_ftext_info.ob_y + this_win_form->work_area.g_y;
    obj_pxy_info[2] = obj_gui_ftext_info.ob_width + 3;
    obj_pxy_info[3] = obj_gui_ftext_info.ob_height + 3;

    OBJECT obj_gui_ftext_filepath = tree[DiagExport_FTEXTDest];
    int16_t obj_pxy_filepath[4];
    obj_pxy_filepath[0] = obj_gui_ftext_filepath.ob_x + this_win_form->work_area.g_x;
    obj_pxy_filepath[1] = obj_gui_ftext_filepath.ob_y + this_win_form->work_area.g_y;
    obj_pxy_filepath[2] = obj_gui_ftext_filepath.ob_width + 3;
    obj_pxy_filepath[3] = obj_gui_ftext_filepath.ob_height + 3;

    int16_t focus_object = this_win_form->wi_data->rsc.current_object;

    int16_t i, j, k;
    int16_t var;

    boolean original_extension_removed = FALSE;

    switch (focus_object){
        case DiagExport_ButtonBrowse:
            st_clear_char_array(this_export.export_path);
            st_clear_char_array(this_export.export_comments);
            st_clear_char_array(this_export.export_dir);
            st_clear_char_array(this_export.file_without_ext);
            st_clear_char_array(file);
            st_clear_char_array(final_path);
    
            // strcpy(file, this_win_master->wi_name);
            strcpy(file, basename(this_win_master->wi_name));
            /* Get the filename without extention */
            for(i = strlen(file); i > 0; i--){
                if(file[i] == '.'){
                    strcpy(&file[i], default_extension);
                    original_extension_removed = TRUE;
                    break;
                }
            }
            if(original_extension_removed == FALSE){
                strcat(file, default_extension);
            }
            

            file_selector(final_path, (char*)"Select a destination path", file);
        
            j = strlen(final_path);
            k = strlen(this_win_master->wi_data->path);

            /* if we got a directory for destination we take the original filename */
            if(st_DirectoryExists(final_path)){
                strncpy(this_export.export_dir, final_path, j);
                for(i = strlen(this_win_master->wi_data->path); i > 0; i--){
                    if(this_win_master->wi_data->path[i] == '\\' || this_win_master->wi_data->path[i] == '/'){
                        strncpy(file, &this_win_master->wi_data->path[i + 1], k - i);
                        break;
                    }
                }
            } else {
                /* Get dir & basename */
                for(i = strlen(file); i > 0; i--){
                    if(file[i] == '\\' || file[i] == '/'){
                        strncpy(file, (const char*)&final_path[ j - i ], i);
                        break;
                    }
                }
                strncpy(&this_export.export_dir[0], &final_path[0], j - strlen(file));
            }
            /* Get the filename without extention */
            for(i = strlen(file); i > 0; i--){
                if(file[i] == '.' || file[i] == '/' || file[i] == '\\'){
                    strncpy(this_export.file_without_ext, file, i);
                    if(file[i] == '.'){
                        st_clear_char_array(this_export.export_extension);
                        strncpy(this_export.export_extension, &file[i], strlen(file));
                    }
                    break;
                }
            }
/*
fo_btree	Address of the object tree in memory
fo_bobject	Object to be processed
fo_bclicks	Number of mouse clicks
fo_bnxtobj	New current object, or 0 if the next object has the status HIDDEN or DISABLED, or is not editable
*/

            sprintf(this_export.export_path, "%s%s", this_export.export_dir, this_export.file_without_ext );

            if(this_export.export_extension[0] == '\0'){
                strcat(this_export.export_path, default_extension);
            } else {
                strcat(this_export.export_path, this_export.export_extension);
            }

            shrink_char_obj(this_export.export_path, &obj_gui_ftext_filepath);
            st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            // objc_draw( tree, 0, MAX_DEPTH, obj_pxy_filepath[0] , obj_pxy_filepath[1], obj_pxy_filepath[2], obj_pxy_filepath[3] );

            if(strcasecmp(this_export.export_extension, ".png") == 0){
                form_button(tree, DiagExport_chk_png, 1, 0);
                st_Set_Export(&st_Image_Export_To_PNG, ".png", &obj_gui_ftext_filepath);

                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 32, "PNG");

            } else if(strcasecmp(this_export.export_extension, ".heif") == 0 || strcasecmp(this_export.export_extension, ".hei") == 0){
                form_button(tree, DiagExport_chk_heif, 1, 0);
                st_Set_Export(&st_Image_Export_To_HEIF, ".heif", &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "HEIF");
            } else if(strcasecmp(this_export.export_extension, ".webp") == 0 || strcasecmp(this_export.export_extension, ".web") == 0){
                form_button(tree, DiagExport_chk_webp, 1, 0);
                st_Set_Export(&st_Image_Export_To_WEBP, ".webp", &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "WEBP");
            } else if(strcasecmp(this_export.export_extension, ".jpg") == 0 || strcasecmp(this_export.export_extension, ".jpeg") == 0 || strcasecmp(this_export.export_extension, ".jpe") == 0){
                form_button(tree, DiagExport_chk_jpeg, 1, 0);
                st_Set_Export(&st_Image_Export_To_JPEG, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "JPEG");                         
            } else if(strcasecmp(this_export.export_extension, ".tif") == 0 || strcasecmp(this_export.export_extension, ".tiff") == 0){
                form_button(tree, DiagExport_chk_tiff, 1, 0);
                st_Set_Export(&st_Image_Export_To_TIFF, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "TIFF");                          
            } else if(strcasecmp(this_export.export_extension, ".mfd") == 0){
                form_button(tree, DiagExport_chk_mfd, 1, 0);
                st_Set_Export(&st_Image_Export_To_MFD, ".mfd", &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, screen_workstation_bits_per_pixel, "MFD");
            } else if(strcasecmp(this_export.export_extension, ".bmp") == 0 || strcasecmp(this_export.export_extension, ".BMP") == 0){
                form_button(tree, DiagExport_chk_bmp, 1, 0);
                st_Set_Export(&st_Image_Export_To_BMP, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "BMP");                          
            } else if(strcasecmp(this_export.export_extension, ".tga") == 0 || strcasecmp(this_export.export_extension, ".TGA") == 0){
                form_button(tree, DiagExport_chk_tga, 1, 0);
                st_Set_Export(&st_Image_Export_To_TGA, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "TGA");                          
            } else if(strcasecmp(this_export.export_extension, ".pi1") == 0 || strcasecmp(this_export.export_extension, ".PI1") == 0){
                form_button(tree, DiagExport_chk_pi1, 1, 0);
                st_Set_Export(&st_Image_Export_To_Degas, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 4, "DEGAS");                          
            } else if(strcasecmp(this_export.export_extension, ".pi3") == 0 || strcasecmp(this_export.export_extension, ".PI3") == 0){
                form_button(tree, DiagExport_chk_pi3, 1, 0);
                st_Set_Export(&st_Image_Export_To_Degas, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 1, "DEGAS");                          
            } else if(strcasecmp(this_export.export_extension, ".psd") == 0 || strcasecmp(this_export.export_extension, ".PSD") == 0){
                form_button(tree, DiagExport_chk_psd, 1, 0);
                st_Set_Export(&st_Image_Export_To_PSD, this_export.export_extension, &obj_gui_ftext_filepath);
                st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "PHOTOSHOP");                          
            } else {
                sprintf(alert_message, "Unknown %s extension", this_export.export_extension);
                st_form_alert(FORM_STOP, alert_message);
            }
            break;
        case DiagExport_chk_png:
            if(st_Set_Export(&st_Image_Export_To_PNG, ".png", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }

            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 32, "PNG");
 
            break;
        case DiagExport_chk_jpeg:
            if(st_Set_Export(&st_Image_Export_To_JPEG, ".jpg", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "JPEG");
            break;
        case DiagExport_chk_webp:
            if(st_Set_Export(&st_Image_Export_To_WEBP, ".webp", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "WEBP");
            break;
        case DiagExport_chk_tiff:
            if(st_Set_Export(&st_Image_Export_To_TIFF, ".tiff", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "TIFF");
            break;
        case DiagExport_chk_bmp:
            if(st_Set_Export(&st_Image_Export_To_BMP, ".bmp", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "BMP");
            break;
        case DiagExport_chk_tga:
            if(st_Set_Export(&st_Image_Export_To_TGA, ".tga", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "TGA");
            break;
        case DiagExport_chk_pi1:
            if(st_Set_Export(&st_Image_Export_To_Degas, ".pi1", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 4, "DEGAS");
            break;
        case DiagExport_chk_pi3:
            if(st_Set_Export(&st_Image_Export_To_Degas, ".pi3", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 1, "DEGAS");
            break;
        case DiagExport_chk_psd:
            if(st_Set_Export(&st_Image_Export_To_Degas, ".psd", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 1, "PHOTOSHOP");
            break;                          
        case DiagExport_chk_mfd:
            if(st_Set_Export(&st_Image_Export_To_MFD, ".mfd", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, screen_workstation_bits_per_pixel, "MFD");
            break;
        case DiagExport_chk_heif:
            if(st_Set_Export(&st_Image_Export_To_HEIF, ".heif", &obj_gui_ftext_filepath)){
                st_Refresh_Object(&obj_gui_ftext_filepath, this_win_form);
            }
            st_Update_Comments(this_win_form_handle, (void*)&this_export, &obj_gui_ftext_info, 24, "HEIC"); 
            break;
        case DiagExport_OkBtnEx:
            /* Setting destination image values */
            this_export.export_width = this_win_master->wi_to_work_in_mfdb->fd_w;
            this_export.export_height = this_win_master->wi_to_work_in_mfdb->fd_h;
            this_export.export_data = (u_int8_t*)this_win_master->wi_to_work_in_mfdb->fd_addr;
            this_export.export_components = this_win_master->wi_to_work_in_mfdb->fd_nplanes >> 3;

            if(this_export.export_function != NULL){
                this_export.export_function((void*)&this_export);
            }
            break;
        default:
            break;
    } 
}
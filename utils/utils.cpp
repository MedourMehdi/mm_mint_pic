#include "utils.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

u_int16_t palette_256[256] = {
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,
0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,
0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,
0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,
0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,
0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,
0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,
0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,
0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,
0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,0x0FFF,0x0F00,0x00F0,0x0FF0,
0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,0x033F,0x0F3F,0x03FF,0x0000,
0x0FFF,0x0F00,0x00F0,0x0FF0,0x000F,0x0F0F,0x00FF,0x0555,0x0333,0x0F33,0x03F3,0x0FF3,
0x033F,0x0F3F,0x03FF,0x0000
};

int16_t get_color(int16_t colorIndex);

void stringtoupper(char *ext);
void stringtolower(char *ext);

/*
*
*	Image tools
*
*/

/* saves the current palette into a buffer */
void save_palette(int16_t *paletteBuffer)
{
    int i;
    for(i=0; i < 16; i++)
    {
        paletteBuffer[i] = get_color(i);
    }
}

/* gets color value in the current palette */
int16_t get_color(int16_t colorIndex)
{
    return Setcolor(colorIndex, -1);
}

/*
*
*	To avoid vscode warnings about m68 registers
*
*/

int16_t st_Getrez(){
	return Getrez();
}

/*
*
*	Supexec routines
*
*/

void reset(void)
{
	(* (void (**)(void)) 0x4)();
}

long get200hz(void)
{
	return *((long*)0x4ba);
}

long st_Supexec(long atari_hardware_addr()){
	return Supexec(atari_hardware_addr);
}

/*
*
*	Others fancy functions
*
*/

char* replace_char(char* str, char find, char replace){
    char *current_pos = strchr(str,find);
    while (current_pos) {
        *current_pos = replace;
        current_pos = strchr(current_pos,find);
    }
    return str;
}

boolean check_ext(const char *ext1, const char *ext2){
	u_int16_t i;
	boolean lower_detected;
	lower_detected = FALSE;

	for (i = 0; ext1[i] != '\0'; i++) {
		if (ext1[i] >= 'a' && ext1[i] <= 'z'){
			lower_detected = TRUE;
		break;
		}
	}
	if(lower_detected){
		stringtolower((char *)ext2);
	} else {
		stringtoupper((char *)ext2);
	}
	if(strcmp(ext1, ext2) == 0){
		return TRUE;
	} else {
		return FALSE;
	}
}

void stringtolower(char *ext){
	u_int8_t i;
	u_int8_t s_len;

	s_len = strlen(ext);
    for (i = 0; i < s_len; i++)
    { 
        ext[i] = tolower(ext[i]); 
    }
}

void stringtoupper(char *ext){
	u_int8_t i;
	u_int8_t s_len;

	s_len = strlen(ext);
    for (i = 0; i < s_len; i++)
    { 
        ext[i] = toupper(ext[i]); 
    }
}

/*
*
*	Memory routines
*
*/

void *mem_alloc(long size){
	void *mem_ptr = NULL;
	mem_ptr = (void*)Mxalloc(size,3);
	return mem_ptr;
}

void *mem_calloc(size_t elementCount, size_t size){
	void *mem_ptr = NULL;
	mem_ptr = (void*)Mxalloc(elementCount * size,3);
    memset(mem_ptr, 0, elementCount * size);
	return mem_ptr;
}

void *mem_free(void *ptr){
    if(ptr != NULL){
        Mfree(ptr);
        ptr = NULL;
    }
	// free(ptr);
	return NULL;
}

/*
*
*	Disk routines
*
*/

int16_t st_Dgetpath(char* current_path, int16_t drive_num){
	/*
		int16_t drive_num
		0	Current drive
		1	Drive A:
		2	Drive B:
		3	Drive C: (etc.)
	*/
	int16_t rez = Dgetpath(current_path, drive_num);
	return rez;
}

int16_t st_Dgetdrv(){
	int16_t rez = Dgetdrv();
	return rez;
}

/**
 * Function to check whether a directory exists or not.
 * It returns 1 if given path is directory and  exists 
 * otherwise returns 0.
 */
int16_t st_DirectoryExists(const char *path){
    struct stat stats;

    stat(path, &stats);

    // Check for file existence
    if (S_ISDIR(stats.st_mode))
        return 1;

    return 0;
}

/**
 * Function to check whether a file exists or not using 
 * access() function. It returns 1 if file exists at 
 * given path otherwise returns 0.
 */
int16_t st_FileExistsAccess(const char *path)
{
    // Check for file existence
    if (access(path, F_OK) == -1)
        return 0;

    return 1;
}

void shrink_char_obj(const char* src_string, OBJECT* dst_string){

	int16_t src_len = strlen(src_string), max_len = dst_string->ob_spec.tedinfo->te_txtlen;
	if(src_len > max_len){
		snprintf(dst_string->ob_spec.tedinfo->te_ptext, (max_len >> 1) - 2, "%ls", src_string);
		strcat(dst_string->ob_spec.tedinfo->te_ptext, (const char*)"...");
		strcat(dst_string->ob_spec.tedinfo->te_ptext, &src_string[src_len - ((max_len >> 1) - 1)]);
	} else {
		sprintf(dst_string->ob_spec.tedinfo->te_ptext, "%ls", src_string);
	}

}

void st_clear_char_array(char* carray){
    memset(carray, '\0', strlen(carray));
}

char *basename(char const *path)
{
        char *s = strrchr(path, '\\');
        if(s==NULL) {
                return strdup(path);
        } else {
                return strdup(s + 1);
        }
}

void st_form_alert(int16_t this_form_icon, char* this_form_message){
    char _form_message[96];
    replace_char(this_form_message, 0XA, '|');
    sprintf(_form_message, "[%d][%s][Okay]", this_form_icon, this_form_message);
    form_alert(1, _form_message);
}

int16_t st_form_alert_choice_2(int16_t this_form_icon, char* this_form_message, char* but0, char* but1, char* but2){
    char _form_message[96];
    replace_char(this_form_message, 0XA, '|');
    sprintf(_form_message, "[%d][%s][%s|%s|%s]", this_form_icon, this_form_message, but0, but1, but2);
    return form_alert(1, _form_message);
}

int16_t st_form_alert_choice(int16_t this_form_icon, char* this_form_message, char* but0, char* but1){
    char _form_message[96];
    replace_char(this_form_message, 0XA, '|');
    sprintf(_form_message, "[%d][%s][%s|%s]", this_form_icon, this_form_message, but0, but1);
    return form_alert(1, _form_message);
}

/* MFDB Utils */

MFDB* mfdb_alloc_bpp( int8_t* buffer, int16_t width, int16_t height, int16_t bpp){
    MFDB* new_mfdb = (MFDB*)mem_alloc(sizeof(MFDB));
	int16_t width_stride = MFDB_STRIDE(width) - width;
    new_mfdb->fd_addr = buffer;
    new_mfdb->fd_nplanes = bpp;
    new_mfdb->fd_w = width;
    new_mfdb->fd_h = height;
    new_mfdb->fd_wdwidth = MFDB_STRIDE(width) >> 4;
	new_mfdb->fd_stand = 0;
    new_mfdb->fd_r2 = 0;
    new_mfdb->fd_r3 = 0;
    return new_mfdb;
}

/* warning: mfdb_update will not clear fd_addr */
int16_t mfdb_update_bpp( MFDB* new_mfdb, int8_t* buffer, int16_t width, int16_t height, int16_t bpp ){
    int16_t width_stride = MFDB_STRIDE(width) - width;
    new_mfdb->fd_addr = buffer;
    new_mfdb->fd_nplanes = bpp;
    new_mfdb->fd_w = width;
    new_mfdb->fd_h = height;
    new_mfdb->fd_wdwidth = MFDB_STRIDE(width) >> 4;
	new_mfdb->fd_stand = 0;
    new_mfdb->fd_r2 = 0;    
    new_mfdb->fd_r3 = 0;
	return width_stride;
}

// float ReverseFloat( const float inFloat )
// {
//    float retVal;
//    char *floatToConvert = ( char* ) & inFloat;
//    char *returnFloat = ( char* ) & retVal;

//    // swap the bytes into a temporary buffer
//    returnFloat[0] = floatToConvert[3];
//    returnFloat[1] = floatToConvert[2];
//    returnFloat[2] = floatToConvert[1];
//    returnFloat[3] = floatToConvert[0];

//    return retVal;
// }

void mfdb_duplicate(MFDB *src_mfdb, MFDB *dst_mfdb){
    /* We duplicate the original MFDB so we can work on it */
    if(dst_mfdb->fd_addr != NULL){
        mem_free(dst_mfdb->fd_addr);
    }

    u_int32_t src_buffer_size;
    u_int8_t *dst_buffer = st_ScreenBuffer_Alloc_bpp(src_mfdb->fd_w, src_mfdb->fd_h, src_mfdb->fd_nplanes);

    if(src_mfdb->fd_nplanes < 8){
        src_buffer_size = (MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h) / (8 / src_mfdb->fd_nplanes);
    } else {
        src_buffer_size = MFDB_STRIDE(src_mfdb->fd_w) * src_mfdb->fd_h * (src_mfdb->fd_nplanes >> 3);
    }
    memcpy(dst_buffer, src_mfdb->fd_addr, src_buffer_size);
    mfdb_update_bpp(dst_mfdb, (int8_t*)dst_buffer, src_mfdb->fd_w, src_mfdb->fd_h, src_mfdb->fd_nplanes);
}

void st_MFDB_Fill(MFDB* this_mfdb, u_int32_t background_color){
    u_int32_t numPixels = MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h;
    int16_t nb_components = this_mfdb->fd_nplanes >> 3;
    for (u_int32_t i = 0; i < numPixels; ++i){
        memcpy((u_int32_t*)this_mfdb->fd_addr + i, &background_color, nb_components);
    }
    return;
}

void st_MFDB_Fill_bpp(MFDB* this_mfdb, u_int32_t background_color, int16_t bpp){
    u_int32_t numPixels = MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h;
    u_int32_t *ptr_32 = (u_int32_t*)this_mfdb->fd_addr;
    u_int16_t  *ptr_16 = (u_int16_t*)this_mfdb->fd_addr;
    u_int8_t  *ptr_8 = (u_int8_t*)this_mfdb->fd_addr;
    u_int8_t  *color = (u_int8_t*)&background_color;
    int16_t nb_components;
    u_int8_t a = color[0];
    u_int8_t r = color[1];
    u_int8_t g = color[2];
    u_int8_t b = color[3];
    if(bpp < 8){
        nb_components = 1;
        numPixels = (MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h) / (8 / bpp);
    }else{
        nb_components = bpp >> 3;
    }

    switch (bpp)
    {
    case 32:
        for (u_int32_t i = 0; i < numPixels; ++i){
            memcpy(ptr_32 + i, &background_color, 1);
        }
        break;
    case 24:
        for (u_int32_t i = 0; i < numPixels; ++i){
            *ptr_8++ = r;
            *ptr_8++ = g;
            *ptr_8++ = b;
        }
        break;
    case 16:
        for (u_int32_t i = 0; i < numPixels; ++i){
            *ptr_16++ = (g << 8) | b;
        }
        break;
    case 8:
        for (u_int32_t i = 0; i < numPixels; ++i){
            *ptr_8++ = b;
        }
        break;
    default:
        break;
    }

    return;
}

void st_MFDB_Fill_8bits(MFDB* this_mfdb, u_int32_t background_color){
    u_int32_t numPixels = MFDB_STRIDE(this_mfdb->fd_w) * this_mfdb->fd_h;
    int16_t nb_components = this_mfdb->fd_nplanes >> 3;
    u_int8_t eight_bits_color = ((  (background_color >> 16 & 0xFF) >> 5) << 5)
    + ( (  (background_color >> 8 & 0xFF) >> 5) << 2) 
    + ((background_color & 0xFF) >> 6);

    u_int8_t *ptr = (u_int8_t *)this_mfdb->fd_addr;

    for (u_int32_t i = 0; i < (numPixels * 4); i++){
        ptr[i] = eight_bits_color;
    }
    return;
}

u_int8_t* st_ScreenBuffer_Alloc_bpp(u_int16_t width, u_int16_t height, int16_t bpp){
    int16_t width_stride;
	width_stride = MFDB_STRIDE(width) - width;
    u_int32_t destination_size_in_bytes;
    if(bpp < 8){
        destination_size_in_bytes = (MFDB_STRIDE(width) * height) / (8 / bpp);
    } else {
        destination_size_in_bytes = MFDB_STRIDE(width) * height * (bpp >> 3);
    }
    u_int8_t* destination_buffer;
	destination_buffer = (u_int8_t*)mem_alloc(destination_size_in_bytes);
    if(destination_buffer == NULL){
        sprintf(alert_message, "Can't allocate %dbytes", destination_size_in_bytes);
        st_form_alert(FORM_STOP, alert_message);
    }
    if(screen_workstation_format < 2){
        memset(destination_buffer, 0x00, destination_size_in_bytes);
    } else {
        memset(destination_buffer, 0xFF, destination_size_in_bytes);
    }
    
    return destination_buffer;
}

void mfdb_free(MFDB* my_mfdb){
    if(my_mfdb->fd_addr != NULL){
        mem_free(my_mfdb->fd_addr);
    }
    mem_free(my_mfdb);
}

/* saves the current palette into a buffer */
void st_Save_Pal(int16_t* paletteBuffer, int16_t max_colors)
{
    int i;
    if(screen_workstation_bits_per_pixel > 8){
        memcpy(paletteBuffer, palette_256, 512);
    }else{
        for(i = 0; i < max_colors; i++) {
            paletteBuffer[i] = Setcolor(i, -1);
        }
    }

}

// void st_Save_Pal_to_File(int16_t* paletteBuffer, int16_t max_colors)
// {
//     FILE *stream;
//     stream = fopen("palette", "wb");
//     int i;
//     fprintf(stream, "u_int16_t palette_fix[%d] = \n{\n", max_colors);
//     for(i = 0; i < MAX(max_colors, 256); i++)
//     {
//         paletteBuffer[i] = Setcolor(i, -1);
//         fprintf(stream, "0x%04X,\n", paletteBuffer[i]);
//     }
//     fprintf(stream, "}\n");
//     fclose (stream);
// }

void st_Load_Pal(int16_t* paletteBuffer)
{
    Setpalette(paletteBuffer);
}


void st_VDI_SavePalette_RGB(int16_t (*_vdi_palette)[3])
{
	int16_t rgb[3];
	u_int16_t i;  
	for(i = 0; i < MIN(256, (1 << screen_workstation_bits_per_pixel)); i++) {
		vq_color(st_vdi_handle, i, 0, rgb);
		_vdi_palette[i][0] = rgb[0];
		_vdi_palette[i][1] = rgb[1];
		_vdi_palette[i][2] = rgb[2];
	}
}

void dbg_printf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void st_VDI_LoadPalette_RGB(int16_t (*_vdi_palette)[3])
{

	u_int16_t i;  
	for(i = 0; i < (1 << screen_workstation_bits_per_pixel); i++) {

		vs_color(st_vdi_handle, i, _vdi_palette[i]);
	}
}

void remove_quotes(char* s1, char* s2){
    char *dst = s1;
    char *src = s2;
    char c;

    while ((c = *src++) != '\0')
    {
        if (c == '\\')
        {
            *dst++ = c;
            if ((c = *src++) == '\0')
                break;
            *dst++ = c;
        }
        else if (c != '"')
            *dst++ = c;
    }
    *dst = '\0';
}

unsigned char reverse(unsigned char b) {
   b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
   b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
   b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
   return b;
}


char* GetNextVaStartFileName(char* start_pos, char* filename) {
/*
MIT License
Copyright (c) 2021 Jean Lusetti (vision.support@free.fr)
*/    
    char* aposd ;
    char* new_pos = NULL ;

    filename[0] = 0 ;
    aposd = strchr( start_pos, '\'' ) ;
    if ( aposd ) {
        char* aposf = strchr( 1 + aposd, '\'' ) ;
        if ( aposf ) {
            *aposf = 0 ;
            strcpy( filename, 1 + aposd ) ;
            *aposf = '\'' ;
            new_pos = 1 + aposf ;
        }
    } else {
        char* space = strchr( start_pos, ' ' ) ;
        if ( space ) {
            *space = 0 ;
            strcpy( filename, start_pos ) ;
            *space = ' ' ;
            new_pos = 1 + space ;
        } else{
            strcpy( filename, start_pos ) ;
        }
    }
    return new_pos ;
}

void st_Get_Current_Dir(char* dst_char){
    if(strlen(dst_char) < 1){
        int16_t this_current_drive;
        char this_current_path[256] = {'\0'};

        this_current_drive = Dgetdrv();
        Dgetpath(this_current_path,0);
        dst_char[0] = this_current_drive + 65;
        dst_char[1] = ':';
        strcat(dst_char,this_current_path);
    }
}

const wchar_t *st_Char_to_WChar(const char *c) {
    const size_t cSize = strlen(c)+1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs (wc, c, cSize);
    return wc;
}

void st_Path_to_Linux(const char* st_path){
    char* lnx_path = (char*)mem_calloc(1, strlen(st_path));
    char str_tmp[] = "\\/";
    if( strncmp(&st_path[1], ":", 1) == 0){
        if( strncmp(&st_path[0], "U", 1) == 0 || strncmp(&st_path[0], "u", 1) == 0){
            strcpy(lnx_path, "/");
            strcat(&lnx_path[1], &st_path[3]);
        }else{
            strcpy(lnx_path, "/");
            strncpy(&lnx_path[1], st_path, 1);
            strcat(&lnx_path[2], &st_path[2]);
        }
        replace_char(lnx_path, str_tmp[0], str_tmp[1]);
        // printf("--> New path is %s", lnx_path);
        strcpy((char*)st_path, lnx_path);
    }
}
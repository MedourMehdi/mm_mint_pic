#include "utils.h"
#include <math.h>
// void get_colors(const void *p, u_int16_t *colors, int16_t cnt);
// u_int16_t get_word(const u_int8_t *p);

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

int32_t get200hz(void)
{
	return *((int32_t*)0x4ba);
}

int32_t st_Supexec(int32_t atari_hardware_addr()){
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
		// printf("Match TRUE %s %s", ext1, ext2);
		return TRUE;
	} else {
		// printf("Match FALSE %s %s", ext1, ext2);
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

void *mem_alloc(int32_t size){
	void *mem_ptr = NULL;
	mem_ptr = (void*)Mxalloc(size,3);
	return mem_ptr;
}

void *mem_free(void *ptr){
	Mfree(ptr);
    ptr = NULL;
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

int16_t st_form_alert_choice(int16_t this_form_icon, char* this_form_message){
    char _form_message[96];
    replace_char(this_form_message, 0XA, '|');
    sprintf(_form_message, "[%d][%s][Cancel|Continue]", this_form_icon, this_form_message);
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
    new_mfdb->fd_wdwidth = MFDB_STRIDE(width + width_stride) >> 4;
	new_mfdb->fd_stand = 0;
    return new_mfdb;
}

/* warning: mfdb_update will not clear fd_addr */
int16_t mfdb_update_bpp( MFDB* new_mfdb, int8_t* buffer, int16_t width, int16_t height, int16_t bpp ){
    int16_t width_stride = MFDB_STRIDE(width) - width;
    new_mfdb->fd_addr = buffer;
    new_mfdb->fd_nplanes = bpp;
    new_mfdb->fd_w = width;
    new_mfdb->fd_h = height;
    new_mfdb->fd_wdwidth = MFDB_STRIDE(width + width_stride) >> 4;
	new_mfdb->fd_stand = 0;
	return width_stride;
}

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
    u_int32_t numPixels = MFDB_STRIDE(this_mfdb->fd_w)* this_mfdb->fd_h;
    int16_t nb_components = this_mfdb->fd_nplanes >> 3;
    for (u_int32_t i = 0; i < numPixels; ++i){
        memcpy((u_int32_t*)this_mfdb->fd_addr + i, &background_color, nb_components);
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

u_int8_t* st_ScreenBuffer_Alloc_bpp(int16_t width, int16_t height, int16_t bpp){
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
    for(i = 0; i < max_colors; i++)
    {
        paletteBuffer[i] = Setcolor(i, -1);
        // printf("col %d -> 0x%04X\n", i, paletteBuffer[i]);
    }
}

void st_Load_Pal(int16_t* paletteBuffer)
{
    Setpalette(paletteBuffer);
}


void st_VDI_SavePalette_RGB(int16_t (*_vdi_palette)[3])
{
	int16_t rgb[3];
	u_int16_t i;  
	for(i = 0; i < (1 << screen_workstation_bits_per_pixel); i++) {
		vq_color(st_vdi_handle, i, 0, rgb);
		_vdi_palette[i][0] = rgb[0];
		_vdi_palette[i][1] = rgb[1];
		_vdi_palette[i][2] = rgb[2];
	}
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
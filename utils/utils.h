#include "../headers.h"

#ifndef UTILS_HEADERS
#define UTILS_HEADERS
void save_palette(int16_t *paletteBuffer);

int16_t st_Getrez(void);

void reset(void);
long get200hz(void);
long st_Supexec(long atari_hardware_addr());

int16_t st_Dgetpath(char* current_path, int16_t drive_num);
int16_t st_Dgetdrv();
boolean st_DirectoryExists(const char *path);
boolean st_FileExistsAccess(const char *path);

const char *get_filename_ext(const char *filename);
boolean check_ext(const char *ext1, const char *ext2);
char* replace_char(char* str, char find, char replace);
void st_Path_to_Linux(const char* st_path);
bool st_is_lnx_path(const char* this_path);
void st_lnx_path_st(const char* lnx_path);
void st_Path_Parser(const char* this_path, bool lnx);
void shrink_char_obj(const char* src_string, OBJECT* dst_string);
void st_form_alert(int16_t this_form_icon, char* form_message);
int16_t st_form_alert_choice(int16_t this_form_icon, char* this_form_message, char* but0, char* but1);
int16_t st_form_alert_choice_2(int16_t this_form_icon, char* this_form_message, char* but0, char* but1, char* but2);
void st_clear_char_array(char* carray);
char *basename(char const *path);

void *mem_alloc(long size);
void *mem_calloc(size_t elementCount, size_t size);
void *mem_free(void *ptr);

MFDB* mfdb_alloc_bpp( int8_t* buffer, int16_t width, int16_t height, int16_t bpp);
int16_t mfdb_update_bpp( MFDB* new_mfdb, int8_t* buffer, int16_t width, int16_t height, int16_t bpp );
void mfdb_duplicate(MFDB *src_mfdb, MFDB *dst_mfdb);
void st_MFDB_Fill(MFDB* this_mfdb, u_int32_t background_color);
void st_MFDB_Fill_bpp(MFDB* this_mfdb, u_int32_t background_color, int16_t bpp);
void st_MFDB_Fill_8bits(MFDB* this_mfdb, u_int32_t background_color);
u_int8_t* st_ScreenBuffer_Alloc_bpp(u_int16_t width, u_int16_t height, int16_t bpp);
void mfdb_free(MFDB* my_mfdb);

void st_Save_Pal(int16_t* paletteBuffer, int16_t max_colors);
void st_Load_Pal(int16_t* paletteBuffer);

void st_VDI_SavePalette_RGB(int16_t (*vdi_palette)[3]);
void st_VDI_LoadPalette_RGB(int16_t (*_vdi_palette)[3]);

void remove_quotes(char* s1, char* s2);

unsigned char reverse(unsigned char b);

const wchar_t *st_Char_to_WChar(const char *c);

void dbg_printf(const char *fmt, ...);

char* GetNextVaStartFileName(char* start_pos, char* filename);

void st_Get_Current_Dir(char* dst_char);
void st_Get_App_Dir(char* dst_char, char* src_path);
#ifdef WITH_URL
bool st_Is_URL(char* path);
bool st_Get_Tmp_Dir(char *tmp_dir);
void st_Gen_Random_Char(char* dst_char, u_int16_t char_len);
#endif
// float ReverseFloat( const float inFloat );

#endif
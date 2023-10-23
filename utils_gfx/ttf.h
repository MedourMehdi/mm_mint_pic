#include "../headers.h"

#ifndef ST_TTF
#define ST_TTF

// typedef struct {
//     char* font_path;
//     int16_t font_size;
//     int char_boxx;
//     int char_boxy;
// } struct_ft;

#ifndef TTF_DEFAULT_PATH
#define TTF_DEFAULT_PATH "\\fonts\\arial.ttf"
#endif

void print_ft_simple(int init_x, int init_y, MFDB* this_mfdb, char* ttf_file, int font_size, char* this_string);
#endif
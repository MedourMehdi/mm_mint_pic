#include "ttf.h"
#include "../utils/utils.h"

#include <ft2build.h>
#include <freetype/freetype.h>

#ifndef UTF8
typedef unsigned char UTF8;
#endif
#ifndef UTF32
typedef long UTF32;
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

boolean init_ft ( const char *ttf_file, FT_Face *face, FT_Library *ft, FT_UInt req_size );
void done_ft (FT_Library ft);
int face_get_line_spacing(FT_Face face);
void st_TTF_Draw_char(FT_Face face, MFDB *fb, int c, int *x, int y);
void st_TTF_Draw_string(FT_Face face, MFDB *fb, const UTF32 *s, int *x, int y);
void face_get_char_extent(const FT_Face face, int c, int *x, int *y);
void face_get_string_extent(const FT_Face face, const UTF32 *s, int *x, int *y);
UTF32 *utf8_to_utf32(const UTF8 *word);
void st_TTF_Set_Pixel (MFDB *self, int x, int y, BYTE r, BYTE g, BYTE b);

boolean init_ft( const char *ttf_file, FT_Face *face, FT_Library *ft, FT_UInt req_size ){
    boolean ret = FALSE;
    if (FT_Init_FreeType (ft) == 0){
        if (FT_New_Face(*ft, ttf_file, 0, face) == 0){
            if (FT_Set_Pixel_Sizes(*face, 0, req_size) == 0){
                ret = TRUE;
            } else {
                printf("Can't set font size to %d", req_size);
            }
        } else {
            printf("Can't load TTF file %s", ttf_file);
        }
    } else {
        printf("Can't initialize FreeType library"); 
    }
    return ret;
}

void done_ft(FT_Library ft){
    FT_Done_FreeType (ft);
}

int face_get_line_spacing(FT_Face face){
    return face->size->metrics.height >> 6;
}

void st_TTF_Draw_char(FT_Face face, MFDB *fb, int c, int *x, int y){
    FT_UInt gi = FT_Get_Char_Index (face, c);
    FT_Load_Glyph (face, gi, FT_LOAD_DEFAULT);
    int bbox_ymax = face->bbox.yMax >> 6;
    int BearingY = (face->glyph->metrics.horiBearingY >> 6);
    int y_off = bbox_ymax - BearingY;
    int glyph_width = face->glyph->metrics.width >> 6;
    int advance = face->glyph->metrics.horiAdvance >> 6;
    int x_off = (advance - glyph_width) / 2;
    FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
    // FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
    // printf("y_off %d - y %d - face->glyph->metrics.horiBearingY %d\n", y_off, y, face->glyph->metrics.horiBearingY >> 6);
    for (int i = 0; i < (int)face->glyph->bitmap.rows; i++){
        // int row_offset = y + i + y_off;
        int row_offset = y + i - BearingY;
        for (int j = 0; j < (int)face->glyph->bitmap.width; j++){
            unsigned char p = face->glyph->bitmap.buffer [i * face->glyph->bitmap.pitch + j];
            // put pix to mfdb here
            if(p){
                p = 255 - p;
                // p = 0x7D;
                st_TTF_Set_Pixel (fb, *x + j + x_off, row_offset, p, p, p);
            }
        }
    }
    *x += advance;
}

void st_TTF_Draw_string(FT_Face face, MFDB *fb, const UTF32 *s, int *x, int y) {
    while (*s) {
        st_TTF_Draw_char(face, fb, *s, x, y);
        s++;
    }
}

void face_get_char_extent(const FT_Face face, int c, int *x, int *y) {
    FT_UInt gi = FT_Get_Char_Index(face, c);
    FT_Load_Glyph (face, gi, FT_LOAD_NO_BITMAP);
    *y = face_get_line_spacing (face);
    *x = face->glyph->metrics.horiAdvance >> 6;
}

void face_get_string_extent(const FT_Face face, const UTF32 *s, int *x, int *y) {
    *x = 0;
    int y_extent = 0;
    while (*s) {
        int x_extent;
        face_get_char_extent (face, *s, &x_extent, &y_extent);
        *x += x_extent;
        s++;
    }
    *y = y_extent;
}

UTF32 *utf8_to_utf32(const UTF8 *word) {
    int l = strlen ((char *)word);
    UTF32 *ret = (UTF32*)mem_alloc((l + 1) * sizeof (UTF32));
    for (int i = 0; i < l; i++){
        ret[i] = (UTF32) word[i];
    }
    ret[l] = 0;
    return ret;
}

void st_TTF_Set_Pixel(MFDB *this_mfdb, int x, int y, BYTE r, BYTE g, BYTE b){
    u_int8_t* ptr = (u_int8_t*)this_mfdb->fd_addr;
    int index;
    if (x > 0 && x < this_mfdb->fd_w && y > 0 && y < this_mfdb->fd_h) {
        index = (y * MFDB_STRIDE(this_mfdb->fd_w) + x) << 2;
        ptr[index++] = 0xFF;
        ptr[index++] = r;
        ptr[index++] = g;
        ptr[index++] = b;
    }
}

void print_ft_simple(int init_x, int init_y, MFDB* this_mfdb, char* ttf_file, int font_size, char* this_string){

    static const UTF32 utf32_space[2] = {' ', 0};

    int width = this_mfdb->fd_w;
    int height = this_mfdb->fd_h;

    FT_Face face;
    FT_Library ft;

    if (init_ft (ttf_file, &face, &ft, font_size)){

        int space_y;
        int space_x; // Pixel width of a space
        face_get_string_extent(face, utf32_space, &space_x, &space_y);
        int x = init_x;
        int y = init_y;
        int line_spacing = face_get_line_spacing(face);

        const char *word = this_string;
        UTF32 *word32 = utf8_to_utf32 ((UTF8 *)word);
        int x_extent, y_extent;
        face_get_string_extent(face, word32, &x_extent, &y_extent);
        int x_advance = x_extent + space_x;
        if(x + x_advance > width){
            x = init_x; 
            y += line_spacing;
        }
        if (y + line_spacing < init_y + height){
            st_TTF_Draw_string (face, this_mfdb, word32, &x, y);
            // st_TTF_Draw_string (face, this_mfdb, utf32_space, &x, y);
        }
        mem_free(word32);
        done_ft (ft);
    }
}
#include "../headers.h"

#ifndef PNGICO
#define PNGICO

#ifndef PNG_HEADER_LOADED
#define PNG_HEADER_LOADED
#include <png.h>
// #ifndef png_const_bytep
// typedef png_bytep png_const_bytep;
// #endif
#endif

// #define MFDB_STRIDE(w)  (((w) + 15) & -16)
// #define CONTROLBAR_H	40 /* 32px ico */
#define CONTROLBAR_H	32
// #define CONTROLBAR_H	22 /* Use this value if ico size is 16px */
// #ifndef _MFDB_STRIDE_
// #define _MFDB_STRIDE_
// #define MFDB_STRIDE(w)  (((w & 15) != 0) ? (w | 15)+1 : w)
// #endif
#define BYTES_TO_CHECK	8

#ifndef COMPUTE_TRANSPARENCY
#define COMPUTE_TRANSPARENCY(adjustedColor, opacity, foregroundColor, backgroundColor) { adjustedColor = opacity * foregroundColor + (1 - opacity) * backgroundColor; }
#endif

#ifndef HAVE_BOOLEAN
typedef int boolean;
#endif
#ifndef FALSE			/* in case these macros already exist */
#define FALSE	0		/* values of boolean */
#endif
#ifndef TRUE
#define TRUE	1
#endif

typedef struct {
	const char* filename;
	u_int16_t x;
	u_int16_t y;
	u_int16_t x2;
	u_int16_t y2;
	MFDB st_ico_mfdb;
} struct_st_ico_png;

typedef struct {
    int16_t		icon_id;
	const char	*main_icon_path;
	const char	*mask_icon_path;
    struct_st_ico_png	*main_icon;
    struct_st_ico_png	*mask_icon;
    void*		(*main_func)(void* param);
	int16_t	pos_x;
	int16_t pos_y;
	int	icon_status;
} struct_st_ico_png_list;

typedef struct {
	struct_st_ico_png_list*	control_bar_list;
	u_int16_t				control_bar_h;
	int16_t					last_ico_padding_right;
	int16_t					pxy_control_bar[4];
	int8_t*					st_control_bar_buffer;
	GRECT					rect_control_bar;
	MFDB					st_control_bar_mfdb;
	MFDB*					background_mfdb;
	MFDB*					virtual_screen_mfdb;
	boolean 				need_to_reload_control_bar;
	boolean 				force_unhide;	
	boolean 				transparency;
	u_int32_t 				transparency_color;
	VdiHdl*					vdi_handle;
} struct_st_control_bar;

bool st_Ico_PNG_Init(struct_st_ico_png_list *ico_list_array);

void st_Ico_PNG_Release(struct_st_ico_png_list *ico_list_array);

void st_Control_Bar_PNG_Handle(int16_t mouse_x, int16_t mouse_y, int16_t mouse_button, struct_st_control_bar *st_control_bar, void* param);

void st_Control_Bar_PXY_Update(struct_st_control_bar *this_control_bar, GRECT *win_work_area);
void st_Control_Bar_Buffer_to_Screen(struct_st_control_bar* control_bar, GRECT* raster_dest);
void st_Control_Bar_Redraw(struct_st_control_bar* control_bar, int16_t my_win_handle);
void st_Control_Bar_Refresh_MFDB(struct_st_control_bar *control_bar,  MFDB *background_mfdb, int16_t elevator_posx, int16_t elevator_posy, int16_t win_work_area_width, int16_t win_work_area_heigh);

void st_Control_Bar_Refresh_Classic(struct_st_control_bar *control_bar, int16_t win_work_area_width, int16_t bpp);
#endif
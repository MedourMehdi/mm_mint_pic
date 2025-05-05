#ifndef INITHEADERS
#define INITHEADERS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef MINT_HEADERS
#define MINT_HEADERS
#include <mint/osbind.h>
#include <mint/mintbind.h>
// #include <mint/cookie.h>
#endif

#include <unistd.h>
#include <sys/stat.h> /* type defined here */

#ifndef GEM_HEADERS
#define GEM_HEADERS
#include <gem.h>
#include <gemx.h>
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

#ifndef SELECTED
#define SELECTED    0x01
#endif

#define MIN(a,b) (a<=b?a:b)
#define MAX(a,b) (a>=b?a:b)

#define DO_WE_USE_FILE TRUE

#ifndef MFDB_STRIDE
#define MFDB_STRIDE(w) (((w) + 15) & -16)
#endif

#define div_255_fast(x) (((x) + (((x) + 257) >> 8)) >> 8)

#define mul_3_fast(x) ((x << 1) + x)
#define mul_5_fast(x) ((x << 2) + x)
#define mul_6_fast(x) ((x << 2) + (x << 1))
#define mul_7_fast(x) ((x << 3) - x)
#define mul_10_fast(x) ((x << 3) + (x << 1))
#define mul_100_fast(x) ((mul_10_fast(x) << 3) + (mul_10_fast(x) << 1))
#define mul_1000_fast(x) ((mul_100_fast(x) << 3) + (mul_100_fast(x) << 1))
#define mul_255_fast(x) ((x << 8) - x)
#define div_1000_fast(x) ((x >> 10) + (x >> 15) - (x >> 17) + (x >> 21) + (x >> 24) + (x >> 26) - (x >> 29));

/*
*
*   External variables
*
*/

extern int16_t xdesk, ydesk, wdesk, hdesk, wrez, hrez;

extern int16_t screen_workstation_format;
extern int16_t screen_workstation_bits_per_pixel;
extern bool reverse_bits;
extern int16_t st_vdi_handle;
extern int16_t msg_buffer[8];
extern int16_t wbox, hbox;
extern int16_t  butdown; /* button state tested for UP/DOWN */
extern GRECT r1, r2;
extern int16_t  r1_flags;
extern int16_t  r2_flags;
extern int16_t  mx, my; /* Mouse Position */
extern int16_t  mb, mc; /* Mouse button - clicks */
extern int16_t  ks, kc; /* Key state/code */
extern int16_t  events; /* What events are valid ? */

extern MFDB screen_mfdb;

extern char alert_message[96];

// extern int16_t wchar, hchar, wbox, hbox;

extern int16_t vdi_palette[256][3];
extern int16_t pix_palette[256];
extern int16_t palette_ori[256];

extern long event_timer_default;
extern long event_timer_video;
extern long event_timer_used; 

/*
COOKIE related section
*/
extern u_int16_t mint_version;
extern u_int8_t computer_type;
extern u_int8_t cpu_type;
extern u_int16_t tos_version;
extern bool edDi_present;
extern bool emutos_rom;
// extern bool gsxb_present;
extern bool milanblaster_present;
/* App path */
extern char current_path[256];

extern bool path_to_lnx;
extern bool use_cached_icons;

#define GREY_COLOR 0x7AC0C0C0

#define FORM_EXCLAM     1
#define FORM_QUESTION   2
#define FORM_STOP       3

#define THIS_APP_NAME    "MMPIC"
// #define USE_LNX_PATH

#define MAIN_RSC_PATH    "\\rsc\\mm_pic.rsc"

// #define DEBUG   1

#ifdef DEBUG
#define TRACE(x) do { if (DEBUG) dbg_printf x; } while (0);
#else
#define TRACE(x)
#endif

#define USE_CIRCULAR_BUFFER 1
// #define WITH_CACHE 1
// #define PRINT_REAL_HZ 1

// #define WITH_URL 1

// #define WITH_WAVLIB 1
// #define WITH_XPDF 1
// #define WITH_PSD 1

// #define WITH_FFMPEG 1
// #define WITH_FFMPEG_SOUND 1
// #define WITH_MP3LIB 1
// #define WITH_LUNASVG 1
extern void *event_loop( void *result);
#endif
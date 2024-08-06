#include "headers.h"

#include "png_ico/png_ico.h"
#include "utils_rsc/progress.h"

#include <pthread.h>

#ifndef WINDOWSTRUCT_HEADERS
#define WINDOWSTRUCT_HEADERS

#define MIN_WINDOWS_HSIZE 160
#define MIN_WINDOWS_WSIZE 320
#define MAX_WINDOWS	10
#define WINDOW_TITLE_MAXLEN 128

#define NUM_THREADS	20

#define WIN_STYLE_IMG (NAME|CLOSER|ICONIFIER|FULLER|MOVER|SIZER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE)
// #define WIN_STYLE_IMG (NAME|CLOSER|ICONIFIER|FULLER|MOVER|SIZER|VSLIDE|HSLIDE)

#define WIN_STYLE_AUTOSCALED (NAME|CLOSER|ICONIFIER|FULLER|MOVER|SIZER)

#define WIN_STYLE_2 (0x0000)

#define WIN_STYLE_FORM (NAME|CLOSER|MOVER)

#define WIN_STYLE_THUMBS (NAME|CLOSER|ICONIFIER|MOVER|UPARROW|DNARROW|VSLIDE|LFARROW|RTARROW|HSLIDE|SIZER)

#define WIN_STYLE_VID (NAME|CLOSER|MOVER)

#define	CLIPPING_OFF 0
#define	CLIPPING_ON 1

#define HORIZONTAL_MOVE 0
#define VERTICAL_MOVE 1

#define MAX_THUMBNAIL_SIZE 120
#define GRAY_LIGHT 	0x7AD9D9D9
#define GRAY_DARK	0x7A2596BE
#define BLUE_MIDNIGHT	0x7A191970
#define BLUE_DODGER		0x7A1E90FF
#define BLUE_SKY		0x7A00BFFF

typedef struct {
	boolean	crop_requested;
	int16_t master_win_handle;
	void*	(*crop_func)(void* param);
	MFDB*	wi_crop_rendered;
	MFDB*	wi_crop_original;
	GRECT	rect_crop_array;
	int16_t	pxy_crop_array[4];
} struct_crop;

typedef struct
{
    u_int16_t prescale;
    u_int16_t original_channels;
    u_int16_t effective_channels;
    u_int16_t original_samplerate;
    u_int16_t effective_samplerate;
	u_int16_t wanted_samplerate;
    u_int32_t bufferSize;
	u_int32_t processedSize;
    u_int16_t original_sampleformat;
    u_int16_t effective_sampleformat;	
	u_int16_t effective_bytes_per_samples;
	float duration_s;
    int8_t* pBuffer;
    int8_t* pPhysical;
    int8_t* pLogical;
    void* (*sound_feed)(void*);
    int16_t win_handle;
	void* user_data;
	int16_t left_lvl;
	int16_t right_lvl;
	bool use_clk_ext;
	bool play;
	bool flip_play_action;
    u_int8_t* surplus_buffer;
    u_int32_t surplus_buffer_size;	
} struct_snd;

typedef struct struct_st_thumbs_list{
    u_int32_t thumb_id;
	u_int16_t thumb_index;
    char*   thumb_filename;
    boolean thumb_selected;
	boolean thumb_selectable;	
	boolean thumb_visible;
	struct_st_thumbs_list* next;
	struct_st_thumbs_list* prev;
    int16_t thumb_win_pxy[4];
    int16_t thumb_desk_pxy[4];
    MFDB*   thumb_mfdb;
    void*	(*thumb_func)(void* param);
} struct_st_thumbs_list;

typedef struct {
	int16_t		master_win_handle;
	int16_t		slave_win_handle;
	int16_t		thumbs_nb;
    int16_t		padx;
    int16_t 	pady;
	int16_t		thumb_w_size;
	int16_t		thumb_h_size;
	int16_t		thumb_w_Item;
	int16_t		thumb_h_Item;
	int16_t		thumbs_rows;
	int16_t		thumbs_cols;
    int16_t		thumbs_area_w;
    int16_t		thumbs_area_h;
	int16_t		thumbs_max_area_w;
    int16_t		thumbs_max_area_h;
	int16_t		thumbs_selected_nb;
    boolean		thumbs_area_refresh;
	boolean		thumbs_open_new_win;
	MFDB*		thumb_selected_mfdb;
    u_int32_t   thumb_selected_color;
	MFDB*		thumb_background_mfdb;
    u_int32_t   thumb_background_color;
    MFDB*		wi_original_thumbs_mfdb;
	struct_st_thumbs_list* thumbs_list_array;
	bool	(*open_win_func)(const char* path);
	void*	(*thumb_clean_func)(void* param);
} struct_thumbs;

typedef struct 
{
	const char *rsc_file;
	OBJECT	*tree;
	int16_t *first_edit;
    int16_t edit_object;
    int16_t next_object, hotspot_object;
	int16_t current_object;
    int16_t we_continue;
    int16_t char_pos;
	int16_t startob;
	u_int16_t winform_padding;
	int16_t win_master_handle;
	void (*process_function)(int16_t);
} struct_rsc_metadata;

typedef struct {
	int16_t original_width;
	int16_t original_height;
	int16_t scaled_pourcentage;
	int16_t scaled_width;
	int16_t scaled_height;
	int16_t rotate_degree;
	int16_t export_width;
	int16_t export_height;
	int8_t	bit_per_pixel;
	boolean needs_resize;
	boolean needs_dither;
	boolean needs_grayscale;
	int8_t	dither_size;
	long img_id;
	int16_t img_index;
	u_int32_t img_total;
} struct_image_metadata;

typedef struct {
	u_int32_t frame_delay;
} struct_video_metadata;

typedef struct {
	void *sws_ctx;
	void *swr_ctx;
	void *pFormatCtx;
	void *pCodecVideoCtx;
	void *pCodecAudioCtx;
	void *pCodecVideoParam;
	void *pCodecAudioParam;
	void *pCodecVideo;
	void *pCodecAudio;
	void *pFrameVideo;
	void *pFrameAudio;	
	void *pFrameRGBVideo;
	void *pPacketVideo;
	void *pPacketAudio;
	int16_t videoStream;
	int16_t audioStream;
	double fps;
	double delay;
	double time_start;
	double time_end;
	double total_duration;
	double duration;
} struct_ffmpeg;

typedef struct {
	char *path;
	char *extension;    
	int8_t *original_buffer;
	boolean crop_requested;
	boolean fx_on;
	boolean fx_requested;
	boolean play_on;
	boolean thumbnail_master;
	boolean thumbnail_slave;
	boolean control_bar_media;
	boolean video_media;
	boolean sound_media;	
	boolean image_media;
	boolean doc_media;
	boolean rsc_media;
	boolean autoscale;
	u_int16_t xbrz_scale;
	boolean resized;
	boolean window_size_limited;
	boolean wi_buffer_modified;
	boolean stop_original_data_load;
	boolean remap_displayed_mfdb;
	FILE *file_lock;
	pthread_t *wi_pth;
	struct_image_metadata img;
	struct_rsc_metadata rsc;
	struct stat STAT_FILE;
} struct_metadata;

#ifndef WINDOWSTRUCT
#define WINDOWSTRUCT
typedef struct {
	int16_t wi_handle;
	char *wi_name;
	char *wi_info;
    int16_t wi_style;

    int16_t work_pxy[4];
	int16_t ext_pxy[4];

	boolean prefers_file_instead_mem;

    boolean win_is_topped;

	int16_t current_pos_x; /* x Slide Position */
	int16_t current_pos_y; /* y Slide Position */

	int16_t x_unit;
	int16_t y_unit;

	int16_t total_length_w;
	int16_t total_length_h;

	int16_t stride;

	u_int32_t rendering_time;

    GRECT work_area;
	GRECT ext_area;
	GRECT previous_work_area;	
	GRECT previous_ext_area;
	GRECT full_ext_area;

	MFDB wi_buffer_mfdb;
	MFDB wi_rendered_mfdb;
	MFDB wi_original_mfdb;

	MFDB *wi_original_bitdepth_mfdb;
	MFDB *wi_rendered_bitdepth_mfdb;
	MFDB *wi_to_work_in_mfdb;
	MFDB *wi_to_display_mfdb;

	struct_ffmpeg			(*wi_ffmpeg);
	struct_snd				(*wi_snd);
	struct_video_metadata	(*wi_video);
	struct_crop				(*wi_crop);
	struct_thumbs			(*wi_thumb);
	struct_rsc_metadata		(*wi_form);
    struct_metadata 		(*wi_data);
	struct_st_control_bar	(*wi_control_bar);
	struct_progress_bar		(*wi_progress_bar);
	void (*refresh_win)(int16_t);
	MFDB* (*render_win)(MFDB*);
} struct_window;

#endif

#define THREAD_STACK_SIZE 256000
extern pthread_attr_t *tattr;
extern pthread_t threads[NUM_THREADS];
extern int *taskids[NUM_THREADS];
extern int16_t total_thread;

extern struct_window win_struct_array[MAX_WINDOWS];
extern int16_t number_of_opened_windows;
extern boolean clip_status;
extern boolean mouse_status;
extern boolean exit_call;

void st_Set_Mouse(boolean status);
void send_message(int16_t my_win_handle, int16_t my_message);
void update_struct_window(struct_window *this_win);
struct_window *detect_window(int16_t my_win_handle);
boolean rsc_already_loaded(const char* rsc_file_name);
void open_window(struct_window *this_win);
void wipe_pxy_area(int16_t *pxy);
void st_Set_Clipping(int16_t flag, int16_t *pxy_array);
void reorder_struct_window(void);
int16_t close_window(int16_t this_win_handle);
void win_refresh_from_buffer(struct_window *this_win);

void win_slider_size(int16_t my_win_handle, int16_t length_seen, int16_t direction);
void do_hslide(int16_t my_win_handle, int16_t new_hposition);
void do_vslide(int16_t my_win_handle, int16_t new_vposition);
void do_arrow(int16_t my_win_handle, int16_t arrow_msg);

void redraw_window(int16_t my_win_handle);

void full_size_window(int16_t my_win_handle);

void buffer_to_screen(int16_t my_win_handle, GRECT *raster_dest);

void st_Init_Default_Win(struct_window *this_win);

void st_Start_Window_Process(struct_window *this_win);
void st_End_Window_Process(struct_window *this_win);

void st_Limit_Work_Area(struct_window *this_win);

struct_window* get_win_thumb_slave_by_image_id(int16_t master_win_handle, u_int32_t img_id);
struct_window* get_win_thumb_master_by_file(const char* this_path);

int st_Open_Thread(void* func(void*), void* th_param);
void st_Wait_For_Threads();
void st_Win_Close_All(void);

void st_Win_Set_Ready(struct_window* this_win, u_int16_t width, u_int16_t height);

bool win_is_playing_media(void);

#endif
// #include "../headers.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MINT_HEADERS
#define MINT_HEADERS
#include <mint/osbind.h>
#include <mint/mintbind.h>
#endif

#ifndef GEM_HEADERS
#define GEM_HEADERS
#include <gem.h>
#include <gemx.h>
#endif

#ifndef MAX
#define MAX(a,b) (a>=b?a:b)
#endif

#ifndef PROGRESSBAR_HEADERS
#define PROGRESSBAR_HEADERS

typedef struct {
	bool progress_bar_enabled;
	bool progress_bar_in_use;
	bool progress_bar_locked;
	int16_t current_value;
	int16_t max_value;
	int16_t initial_nb_functions; /* Total functions is in this progress bar */
	int16_t current_nb_functions; /* wich function is in this progress bar */
	GRECT form_rect;
} struct_progress_bar;

#define PROGRESS_BAR_MAX_VALUE  100

void st_Progress_Bar_Init(struct_progress_bar* progress_bar, int8_t *title);
void st_Progress_Bar_Signal(struct_progress_bar* progress_bar, int16_t current_value, int8_t *progress_txt);
void st_Progress_Bar_Finish(struct_progress_bar* progress_bar);
void st_Progress_Bar_Add_Step(struct_progress_bar* progress_bar);
void st_Progress_Bar_Step_Done(struct_progress_bar* progress_bar);

struct_progress_bar* st_Progress_Bar_Alloc_Enable();
void st_Progress_Bar_Destroy(struct_progress_bar* progress_bar);

extern struct_progress_bar *global_progress_bar;

#endif
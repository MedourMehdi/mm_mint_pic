#include "progress.h"
#include "../utils/utils.h"

/* Indices de fichier ressource pour PROGRESS. */

#define PROGRESS         0   /* Form/Boite de dialogue */
#define PROTITLE         1   /* TEXT dans l'arbre PROGRESS */
#define PLINE            2   /* TEXT dans l'arbre PROGRESS */
#define PROBOX           3   /* BOX dans l'arbre PROGRESS */
#define PROBAR           4   /* BOX dans l'arbre PROGRESS */

void beg_prog(GRECT	*rect);
void end_prog(GRECT	*rect);
void set_prog(int16_t value, int16_t MAXc);
void set_text(int16_t obj, int8_t *str);
void disp_obj(OBJECT *tree, int16_t obj);

char rsc_progress_file_name[] = "\\rsc\\progress.rsc";
char rsc_path[256] = {'\0'};

#ifndef _OBJC_XYWH_
#define _OBJC_XYWH_
void objc_xywh(OBJECT *tree, int16_t obj, GRECT *p);
#endif

struct_progress_bar* st_Progress_Bar_Alloc_Enable(){
    struct_progress_bar* progress_bar = (struct_progress_bar*)mem_alloc(sizeof(struct_progress_bar));
	progress_bar->progress_bar_enabled = true;
    progress_bar->progress_bar_in_use = false;
    progress_bar->progress_bar_locked = false;
    progress_bar->initial_nb_functions = 0;
    progress_bar->current_nb_functions = 0;    
    return progress_bar;
}

void st_Progress_Bar_Destroy(struct_progress_bar* progress_bar){
    mem_free(progress_bar);
}

void st_Progress_Bar_Add_Step(struct_progress_bar* progress_bar){
    if(progress_bar != NULL){
        if(progress_bar->progress_bar_enabled == true){
            progress_bar->progress_bar_locked = true;
            progress_bar->initial_nb_functions += 1;
            progress_bar->current_nb_functions += 1;
        }
    }
}

void st_Progress_Bar_Step_Done(struct_progress_bar* progress_bar){
    if(progress_bar != NULL){
        if(progress_bar->progress_bar_enabled == true){
            progress_bar->initial_nb_functions -= 1;
            progress_bar->current_nb_functions -= 1;
            if(!progress_bar->current_nb_functions){
                progress_bar->progress_bar_locked = false;
            }
        }
    }
}

void st_Progress_Bar_Init(struct_progress_bar* progress_bar, int8_t *title){
    if(progress_bar != NULL){
        if(progress_bar->progress_bar_enabled && !progress_bar->progress_bar_in_use){
            if(strlen(rsc_path) < 1){
                strcpy(rsc_path,current_path);
                strcat(rsc_path,rsc_progress_file_name);
            }
            if(!rsrc_load((const char*)&rsc_path)){
                form_alert(1, "[1][st_Progress_Bar_Init -> RSC Error][Okay]");
            }   
            progress_bar->progress_bar_in_use = true;
            progress_bar->current_value = 0;
            progress_bar->max_value = 100;
            beg_prog(&progress_bar->form_rect);
        }
        if(progress_bar->progress_bar_enabled && progress_bar->current_nb_functions == 1){
            set_prog(progress_bar->current_value, progress_bar->max_value);
            set_text(PROTITLE, title);
        }
    }
}

void st_Progress_Bar_Signal(struct_progress_bar* progress_bar, int16_t current_value, int8_t *progress_txt){
    if(progress_bar != NULL){
        if(progress_bar->progress_bar_in_use && progress_bar->progress_bar_enabled ){
            
            // if( progress_bar->initial_nb_functions ){
            //     progress_bar->current_value += ( (progress_bar->max_value - progress_bar->current_value) * current_value ) / 100;
            // }
            progress_bar->current_value = current_value;
            set_text(PLINE, progress_txt);
            set_prog(progress_bar->current_value, progress_bar->max_value);
        }
    }
}

void st_Progress_Bar_Finish(struct_progress_bar* progress_bar){
    if(progress_bar != NULL){
        if(progress_bar->progress_bar_enabled && !progress_bar->progress_bar_locked){
            progress_bar->current_value = 1;
            end_prog(&progress_bar->form_rect);
            progress_bar->progress_bar_in_use = false;
            rsrc_free();
        }
    }
}

void beg_prog(GRECT	*rect) {
	OBJECT	*tree;
	int16_t	xdial, ydial, wdial, hdial;
	rsrc_gaddr(R_TREE, PROGRESS, &tree);
	form_center(tree, &rect->g_x, &rect->g_y, &rect->g_w, &rect->g_h);
	form_dial(FMD_START, 0, 0, 0, 0, rect->g_x, rect->g_y, rect->g_w, rect->g_h);
	objc_draw(tree, ROOT, MAX_DEPTH, rect->g_x, rect->g_y, rect->g_w, rect->g_h);
}

void end_prog(GRECT	*rect) {
	form_dial(FMD_FINISH, 0, 0, 0, 0, rect->g_x, rect->g_y, rect->g_w, rect->g_h);
}

void set_prog(int16_t value, int16_t MAXc) {
    int16_t	width_new, width_old;
    OBJECT	*tree;
    GRECT	box;

    rsrc_gaddr(R_TREE, PROGRESS, &tree);
    width_old = tree[PROBOX].ob_width - 1;	/* Take border into account */
    width_new = width_old + 1;
    if (MAXc){
        width_new = MAX(1, ((long)value * (long)width_new) / MAXc); 
    }
    tree[PROBAR].ob_width = width_new;
    if (value){
        objc_xywh(tree, PROBAR, &box);
        box.g_x += width_old; box.g_w -= width_old;
        objc_draw(tree, ROOT, MAX_DEPTH, box.g_x, box.g_y, box.g_w, box.g_h);
    }
    disp_obj(tree, PROGRESS);
    disp_obj(tree, PROBOX);
    disp_obj(tree, PROBAR);
}

void set_text(int16_t obj, int8_t *str) {
    OBJECT	*tree;
    TEDINFO	*obspec;

    rsrc_gaddr(R_TREE, PROGRESS, &tree);
    obspec = tree[obj].ob_spec.tedinfo;	/* Get TEDINFO address  */
    obspec->te_ptext = (char *)str;			/* Set new text pointer */
    obspec->te_txtlen = strlen((const char *)str);	/* Set new length	*/
    disp_obj(tree, obj);
}

void disp_obj(OBJECT *tree, int16_t obj){
    GRECT	box;

    objc_xywh(tree, obj, &box);
    objc_draw(tree, ROOT, MAX_DEPTH, box.g_x, box.g_y, box.g_w, box.g_h);
}

/* get x,y,w,h for specified object	*/
void objc_xywh(OBJECT *tree, int16_t obj, GRECT *p)		
{
    objc_offset(tree, obj, &p->g_x, &p->g_y);
    p->g_w = tree[obj].ob_width;
    p->g_h = tree[obj].ob_height;
}
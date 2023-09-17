#include "../headers.h"

int16_t new_form_do(OBJECT *tree, int16_t *start_fld);
void new_objc_xywh(OBJECT *tree, int16_t obj, GRECT *p);
int16_t new_form_button(OBJECT *tree, int16_t obj, int16_t clicks, int16_t *next_object, int16_t *hotspot_object);
void objc_toggle(OBJECT *tree, int16_t obj);
int16_t form_hotspot(OBJECT *tree, int16_t hotspot_object, int16_t mouse_x, int16_t mouse_y, GRECT *rect, int16_t *mode);
int16_t new_form_keybd(OBJECT *tree, int16_t edit_object, int16_t next_object, int16_t key_scancode, int16_t *out_object, int16_t *okey_scancode);
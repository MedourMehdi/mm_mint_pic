#include "../headers.h"
#include "../windows.h"
#ifdef WITH_RECOIL
#ifndef IMG_Recoil
#define IMG_Recoil
#include "../external/recoil/recoil.h"
void st_Init_Recoil(struct_window *);
#ifdef USE_CUSTOM_RECOIL_CHECK
bool st_Check_Recoil_Ext(const char* this_ext);
#endif
#endif
#endif
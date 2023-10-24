#include "headers.h"
#include "windows.h"

#ifndef CUSTOM_CBAR_HEADERS
#define CUSTOM_CBAR_HEADERS

/* st_Init_WinImage_Control_Bar() - Init the originals values for your control bar in an Init function */

void st_Init_WinVideo_Control_Bar(void* p_param);
void st_Init_WinImage_Control_Bar(void* p_param);
void st_Init_WinDoc_Control_Bar(void* p_param);
void st_Init_WinStart_Control_Bar(void* p_param);

/*	
*	st_Control_Bar_Refresh_MFDB() rebuild the MFDB control bar with transprency if it was enabled
*		-	control bar structure declared previously & assiociated to your icons + funtions
*		-	window MFDB, containing the original image buffer
*		-	window elevator x & y values
*		-	window working area width & heigh
*
*	st_Control_Bar_Buffer_to_Screen() render the control bar in the window.
*/

void st_Reload_Control_Bar(struct_window *this_win, struct_st_control_bar* control_bar);

bool st_Ico_PNG_Init_Video(void);
bool st_Ico_PNG_Init_Document(void);
bool st_Ico_PNG_Init_Image(void);
bool st_Ico_PNG_Init_Main(void);

void st_Ico_PNG_Release_Image(void);
void st_Ico_PNG_Release_Video(void);
void st_Ico_PNG_Release_Document(void);
void st_Ico_PNG_Release_Main(void);

#endif
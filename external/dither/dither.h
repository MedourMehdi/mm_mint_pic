#include "../../headers.h"
#ifndef DITHER_HEADERS
#define DITHER_HEADERS

void	makeDitherSierra( u_int8_t* pixels, long width, long height );
void    makeDitherSierraLiteRgbNbpp( u_int8_t* pixels, long width, long height, long ncolors );
void    makeDitherSierraRgbNbpp( u_int8_t* pixels, long width, long height, long ncolors );

void	makeDitherFS( u_int8_t* pixels, long width, long height );
void	makeDitherFSRgb3bpp( u_int8_t* pixels, long width, long height );
void	makeDitherFSRgb6bpp( u_int8_t* pixels, long width, long height );

#endif
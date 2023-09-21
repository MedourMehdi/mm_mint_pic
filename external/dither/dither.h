#include "../../headers.h"
#ifndef DITHER_HEADERS
#define DITHER_HEADERS

void	makeDitherSierra( u_int8_t* pixels, int32_t width, int32_t height );
void    makeDitherSierraLiteRgbNbpp( u_int8_t* pixels, int32_t width, int32_t height, int32_t ncolors );
void    makeDitherSierraRgbNbpp( u_int8_t* pixels, int32_t width, int32_t height, int32_t ncolors );

void	makeDitherFS( u_int8_t* pixels, int32_t width, int32_t height );
void	makeDitherFSRgb3bpp( u_int8_t* pixels, int32_t width, int32_t height );
void	makeDitherFSRgb6bpp( u_int8_t* pixels, int32_t width, int32_t height );

#endif
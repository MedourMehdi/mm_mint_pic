#include "../headers.h"
#ifndef PLANAR_HEADERS
#define PLANAR_HEADERS
MFDB* st_Chunky_to_Planar_8bits(MFDB* source_mfdb);
MFDB* st_Planar_to_Chunky_8bits(MFDB* source_mfdb);
MFDB* st_Chunky8bpp_to_Planar_4bpp(MFDB* source_mfdb);
MFDB *st_Planar4bpp_to_chunky32bpp(MFDB* MFDB4bpp, u_int16_t *palette);
#endif
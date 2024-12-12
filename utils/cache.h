#include "../headers.h"

#ifndef UTILS_CACHE
#define UTILS_CACHE
#ifdef WITH_CACHE
bool st_Build_MFDB_Cache(char* filepath, MFDB *data);
bool st_Load_MFDB_Cache(char* filepath, MFDB* mfdb_dst);
bool st_Check_Cached(char* filepath);
#endif
#endif
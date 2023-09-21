#include "headers.h"
#include "utils/utils.h"
#include "windows.h"

#ifndef FILE_HEADERS
#define FILE_HEADERS

#define READ_ONLY "r"
#define READ_ONLY_BINARY "rb"
#define WRITE_BINARY "wb"

boolean open_file(struct_window *this_win, const char *my_path);
boolean file_to_memory(struct_window *this_win);
boolean mfdb_to_file(MFDB *this_mfdb, char* final_path);
int16_t file_selector(char *final_path, char* title, char *file_selected);

#endif
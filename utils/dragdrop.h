#include "../headers.h"

#ifndef DD_OK
#define DD_OK		0
#endif
#define DD_NAK		1
#define DD_NAMEMAX	128
#define DD_EXTSIZE	32

int16_t ddopen(int16_t ddnam, int8_t ddmsg);
int16_t ddrtry(int16_t fd, char *name, char *file, char *whichext, int32_t *size);
int16_t ddsexts(int16_t fd, char *exts);
int16_t ddreply(int16_t fd, int8_t ack);
void ddclose(int16_t fd);
char *strnset ( char * string, int val, size_t count );
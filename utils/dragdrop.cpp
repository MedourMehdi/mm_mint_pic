#include "dragdrop.h"
#include <mintbind.h>
#include <signal.h>
#ifndef SIGPIPE
#define SIGPIPE		13		/* broken pipe */
#endif
#ifndef SIG_IGN
#define       SIG_IGN	((__sighandler_t) 1L)
#endif

void ddsetsig(int32_t oldsig);
static int8_t pipename[] = "U:\\PIPE\\DRAGDROP.AA";
static int32_t pipesig;


void ddclose(int16_t fd)
{
	/* Signalhandler restaurieren */
	
	ddsetsig(pipesig);
	
	
	Fclose(fd);
}

void ddsetsig(int32_t oldsig)
{
	if (oldsig != -32L){
		Psignal(SIGPIPE, (void *) oldsig);
    }
}

void ddgetsig(int32_t *oldsig)
{
	*oldsig = (int32_t) Psignal(SIGPIPE, (void *) SIG_IGN);
}

int16_t ddopen(int16_t ddnam, int8_t ddmsg)
{
	int32_t fd;

	pipename[17] = (ddnam & 0xff00) >> 8;
	pipename[18] = ddnam & 0x00ff;

	fd = Fopen(pipename, 2);

	if (fd < 0L)
		return(-1);


	/* Signalhandler konfigurieren */
	
	ddgetsig(&pipesig);
	
	
	if (Fwrite((int16_t) fd, 1L, &ddmsg) != 1L)
	{
		ddclose((int16_t) fd);
		return(-1);
	}

	return((int16_t) fd);
}

char *strnset ( char * string, int val, size_t count ) {
    char *start = string;
    while (count-- && *string){
        *string++ = (char)val;
    }
    return(start);
}

int16_t ddrtry(int16_t fd, char *name, char *file, char *whichext, int32_t *size)
{
	char buf[DD_NAMEMAX * 2];
	int16_t hdrlen, i, len;

	if (Fread(fd, 2L, &hdrlen) != 2L)
		return(-1);
	

	if (hdrlen < 9)	/* il reste au minimum 11 - 2 = 9 octets a lire */
	{
		/* sollte eigentlich nie passieren */

		return(-1);	/* erreur taille incorrecte */
	}
	
	if (Fread(fd, 4L, whichext) != 4L)	/* lecture de l'extension */
		return(-1);
	
	if (Fread(fd, 4L, size) != 4L)		/* lecture de la longueurs des donnes */
		return(-1);
	
	hdrlen -= 8;	/* on a lu 8 octets */
	
	if (hdrlen > DD_NAMEMAX*2)
		i = DD_NAMEMAX*2;
	else
		i = hdrlen;

	len = i;
	
	if (Fread(fd, (int32_t) i, buf) != (int32_t) i)
		return(-1);
	
	hdrlen -= i;
	
	strncpy(name, buf, DD_NAMEMAX);
	
	i = (int16_t) strlen(name) + 1;
	
	if (len - i > 0)
		strncpy(file, buf + i, DD_NAMEMAX);
	else
		file[0] = '\0';


	/* weitere Bytes im Header in den Mll */
	
	while (hdrlen > DD_NAMEMAX*2)
	{
		if (Fread(fd, DD_NAMEMAX*2, buf) != DD_NAMEMAX*2)
			return(-1);
		
		hdrlen -= DD_NAMEMAX*2;
	}
	
	if (hdrlen > 0)
	{
		if (Fread(fd, (int32_t) hdrlen, buf) != (int32_t) hdrlen)
			return(-1);
	}

	return(1);
}

int16_t ddsexts(int16_t fd, char *exts)
{
	if (Fwrite(fd, DD_EXTSIZE, exts) != DD_EXTSIZE)
	{
		ddclose(fd);
		return(-1);
	}

	return(1);
}

int16_t ddreply(int16_t fd, int8_t ack)
{
	if (Fwrite(fd, 1L, &ack) != 1L)
	{
		ddclose(fd);
		return(-1);
	}
	
	return(1);
}


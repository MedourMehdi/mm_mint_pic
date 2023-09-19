#ifndef _BMP_H_
#define _BMP_H_


/**************************************************************

	QDBMP - Quick n' Dirty BMP

	v1.0.0 - 2007-04-07
	http://qdbmp.sourceforge.net


	The library supports the following BMP variants:
	1. Uncompressed 32 BPP (alpha values are ignored)
	2. Uncompressed 24 BPP
	3. Uncompressed 8 BPP (indexed color)

	QDBMP is free and open source software, distributed
	under the MIT licence.

	Copyright (c) 2007 Chai Braudo (braudo@users.sourceforge.net)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

**************************************************************/

#include <stdio.h>

#ifdef __MINT__

#ifndef _WORD
#define _WORD short int
#endif
#ifndef _UWORD
#define _UWORD unsigned short int
#endif
#ifndef _LONGWORD
#define _LONGWORD long
#endif
#ifndef _ULONGWORD
#define _ULONGWORD unsigned long
#endif

#ifndef _INT8
#define _INT8 char
#endif
#ifndef _UINT8
#define _UINT8 unsigned char
#endif

#ifndef _INT16
#define _INT16 _WORD
#endif
#ifndef _UINT16
#define _UINT16 _UWORD
#endif

#ifndef _UINT32
#define _UINT32 _ULONGWORD
#endif
#ifndef _INT32
#define _INT32 _LONGWORD
#endif

#ifndef _FP64
#define _FP64 double
#endif


#endif

/* Type definitions */
#ifndef _UINT32
	#define _UINT32	unsigned long int
#endif

#ifndef _UINT16
	#define _UINT16	unsigned short
#endif

#ifndef _UINT8
	#define _UINT8	unsigned char
#endif


/* Version */
#define QDBMP_VERSION_MAJOR		1
#define QDBMP_VERSION_MINOR		0
#define QDBMP_VERSION_PATCH		1


/* Error codes */
typedef enum
{
	BMP_OK = 0,				/* No error */
	BMP_ERROR,				/* General error */
	BMP_OUT_OF_MEMORY,		/* Could not allocate enough memory to complete the operation */
	BMP_IO_ERROR,			/* General input/output error */
	BMP_FILE_NOT_FOUND,		/* File not found */
	BMP_FILE_NOT_SUPPORTED,	/* File is not a supported BMP variant */
	BMP_FILE_INVALID,		/* File is not a BMP image or is an invalid BMP */
	BMP_INVALID_ARGUMENT,	/* An argument is invalid or out of range */
	BMP_TYPE_MISMATCH,		/* The requested action is not compatible with the BMP's type */
	BMP_ERROR_NUM
} BMP_STATUS;


/* Bitmap image */
/* Bitmap header */
typedef struct _BMP_Header
{
	_UINT16		Magic;				/* Magic identifier: "BM" */
	_UINT32		FileSize;			/* Size of the BMP file in bytes */
	_UINT16		Reserved1;			/* Reserved */
	_UINT16		Reserved2;			/* Reserved */
	_UINT32		DataOffset;			/* Offset of image data relative to the file's start */
	_UINT32		HeaderSize;			/* Size of the header in bytes */
	_UINT32		Width;				/* Bitmap's width */
	_UINT32		Height;				/* Bitmap's height */
	_UINT16		Planes;				/* Number of color planes in the bitmap */
	_UINT16		BitsPerPixel;		/* Number of bits per pixel */
	_UINT32		CompressionType;	/* Compression type */
	_UINT32		ImageDataSize;		/* Size of uncompressed image's data */
	_UINT32		HPixelsPerMeter;	/* Horizontal resolution (pixels per meter) */
	_UINT32		VPixelsPerMeter;	/* Vertical resolution (pixels per meter) */
	_UINT32		ColorsUsed;			/* Number of color indexes in the color table that are actually used by the bitmap */
	_UINT32		ColorsRequired;		/* Number of color indexes that are required for displaying the bitmap */
} BMP_Header;


/* Private data structure */
typedef struct _BMP
{
	BMP_Header	Header;
	_UINT8*		Palette;
	_UINT8*		Data;
} BMP;




/*********************************** Public methods **********************************/


/* Construction/destruction */
BMP*			BMP_Create					( _UINT32 width, _UINT32 height, _UINT16 depth );
void			BMP_Free					( BMP* bmp );


/* I/O */
BMP*			BMP_ReadFile				( const char* filename );
BMP* 			BMP_ReadBuffer				( _INT8* buffer );
void			BMP_WriteFile				( BMP* bmp, const char* filename );


/* Meta info */
_UINT32			BMP_GetWidth				( BMP* bmp );
_UINT32			BMP_GetHeight				( BMP* bmp );
_UINT16			BMP_GetDepth				( BMP* bmp );


/* Pixel access */
void			BMP_GetPixelRGB				( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8* r, _UINT8* g, _UINT8* b );
void			BMP_SetPixelRGB				( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8 r, _UINT8 g, _UINT8 b );
void			BMP_GetPixelIndex			( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8* val );
void			BMP_SetPixelIndex			( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8 val );


/* Palette handling */
void			BMP_GetPaletteColor			( BMP* bmp, _UINT8 index, _UINT8* r, _UINT8* g, _UINT8* b );
void			BMP_SetPaletteColor			( BMP* bmp, _UINT8 index, _UINT8 r, _UINT8 g, _UINT8 b );


/* Error handling */
BMP_STATUS		BMP_GetError				();
const char*		BMP_GetErrorDescription		();


/* Useful macro that may be used after each BMP operation to check for an error */
#define BMP_CHECK_ERROR( output_file, return_value ) \
	if ( BMP_GetError() != BMP_OK )													\
	{																				\
		fprintf( ( output_file ), "BMP error: %s\n", BMP_GetErrorDescription() );	\
		return( return_value );														\
	}																				\

#endif

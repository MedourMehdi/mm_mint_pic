#include "QDBMP.H"
#include <stdlib.h>
#include <string.h>
#define MFDB_STRIDE(w) (((w) + 15) & -16)

#define UINT16_SWAP_LE_BE_CONSTANT(val)		\
  ((unsigned int)					\
   (						\
    (unsigned int) ((unsigned int) (val) >> 8) |	\
    (unsigned int) ((unsigned int) (val) << 8)))

#define UINT32_SWAP_LE_BE_CONSTANT(val)			  \
  ((unsigned long)						  \
   (							  \
    (((unsigned long) (val) & (unsigned long) 0x000000ffU) << 24) | \
    (((unsigned long) (val) & (unsigned long) 0x0000ff00U) <<  8) | \
    (((unsigned long) (val) & (unsigned long) 0x00ff0000U) >>  8) | \
    (((unsigned long) (val) & (unsigned long) 0xff000000U) >> 24)))

/* Holds the last error code */
static BMP_STATUS BMP_LAST_ERROR_CODE = (BMP_STATUS)0;


/* Error description strings */
static const char* BMP_ERROR_STRING[] =
{
	"",
	"General error",
	"Could not allocate enough memory to complete the operation",
	"File input/output error",
	"File not found",
	"File is not a supported BMP variant (must be uncompressed 8, 24 or 32 BPP)",
	"File is not a valid BMP image",
	"An argument is invalid or out of range",
	"The requested action is not compatible with the BMP's type"
};

/* Size of the palette data for 8 BPP bitmaps */
#define BMP_PALETTE_SIZE_8bpp ( 256 * 4 )

/* Size of the palette data for 4 BPP bitmaps */
#define BMP_PALETTE_SIZE_4bpp ( 16 * 4 )


/*********************************** Forward declarations **********************************/
int		ReadHeader	( BMP* bmp, FILE* f );
int		ReadBufferHeader	( BMP* bmp, _INT8 *f );
int		WriteHeader	( BMP* bmp, FILE* f );

int		Read_UINT32	( _UINT32* x, FILE* f );
int		Read_UINT16	( _UINT16 *x, FILE* f );

int		ReadBuffer_UINT32	( _UINT32* x, _INT8 *f , _INT16 i);
int		ReadBuffer_UINT16	( _UINT16 *x, _INT8 *f , _INT16 i);

int		Write_UINT32	( _UINT32 x, FILE* f );
int		Write_UINT16	( _UINT16 x, FILE* f );

/*********************************** Public methods **********************************/


/**************************************************************
	Creates a blank BMP image with the specified dimensions
	and bit depth.
**************************************************************/
BMP* BMP_Create( _UINT32 width, _UINT32 height, _UINT16 depth )
{
	BMP*	bmp;
	_INT16	bytes_per_pixel = depth >> 3;
	_UINT32	bytes_per_row, bits_per_row;
	_UINT32	palette_size;

	switch (depth)
	{
	case 4:
		palette_size = BMP_PALETTE_SIZE_4bpp;
		break;
	case 8:
		palette_size = BMP_PALETTE_SIZE_8bpp;
		break;
	default:
		palette_size = 0;
		break;
	}

	// printf("\npalette size\n %lu", palette_size);

	if ( height <= 0 || width <= 0 )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return NULL;
	}

	if ( depth != 4 && depth != 8 && depth != 24 && depth != 32 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_SUPPORTED;
		return NULL;
	}


	/* Allocate the bitmap data structure */
	bmp = (BMP *)calloc( 1, sizeof( BMP ) );
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		return NULL;
	}


	/* Set header' default values */
	bmp->Header.Magic				= 0x4D42;
	bmp->Header.Reserved1			= 0;
	bmp->Header.Reserved2			= 0;
	bmp->Header.HeaderSize			= 40;
	bmp->Header.Planes				= 1;
	bmp->Header.CompressionType		= 0;
	bmp->Header.HPixelsPerMeter		= 0;
	bmp->Header.VPixelsPerMeter		= 0;
	bmp->Header.ColorsUsed			= 0;
	bmp->Header.ColorsRequired		= 0;


	/* Calculate the number of bytes used to store a single image row. This is always
	rounded up to the next multiple of 4. */
	bytes_per_row = width * bytes_per_pixel;
	bytes_per_row += ( bytes_per_row % 4 ? 4 - bytes_per_row % 4 : 0 );

	bits_per_row = width * depth;
	bits_per_row += ( bits_per_row % 32 ? 32 - bits_per_row % 32 : 0 );

	/* Set header's image specific values */
	bmp->Header.Width				= width;
	bmp->Header.Height				= height;
	bmp->Header.BitsPerPixel		= depth;
	bmp->Header.ImageDataSize		= depth < 8 ? (bytes_per_row * height) : (height * ( bits_per_row >> 3 )) ;
	bmp->Header.FileSize			= bmp->Header.ImageDataSize + 54 + ( depth == 8 ? BMP_PALETTE_SIZE_8bpp : 0 ) + ( depth == 4 ? BMP_PALETTE_SIZE_4bpp : 0 );
	bmp->Header.DataOffset			= 54 + ( depth == 8 ? BMP_PALETTE_SIZE_8bpp : 0 ) + ( depth == 4 ? BMP_PALETTE_SIZE_4bpp : 0 );


	/* Allocate palette */
	if ( bmp->Header.BitsPerPixel == 8 || bmp->Header.BitsPerPixel == 4 )
	{
		bmp->Palette = (_UINT8*) calloc( palette_size, sizeof( _UINT8 ) );
		if ( bmp->Palette == NULL )
		{
			BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
			free( bmp );
			return NULL;
		}
	}
	else
	{
		bmp->Palette = NULL;
	}


	/* Allocate pixels */
	bmp->Data = (_UINT8*) calloc( bmp->Header.ImageDataSize, sizeof( _UINT8 ) );
	if ( bmp->Data == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		free( bmp->Palette );
		free( bmp );
		return NULL;
	}


	BMP_LAST_ERROR_CODE = BMP_OK;

	return bmp;
}


/**************************************************************
	Frees all the memory used by the specified BMP image.
**************************************************************/
void BMP_Free( BMP* bmp )
{
	if ( bmp == NULL )
	{
		return;
	}

	if ( bmp->Palette != NULL )
	{
		free( bmp->Palette );
	}

	if ( bmp->Data != NULL )
	{
		free( bmp->Data );
	}

	free( bmp );

	BMP_LAST_ERROR_CODE = BMP_OK;
}

/**************************************************************
	Reads the specified BMP image from buffer.
**************************************************************/
BMP* BMP_ReadBuffer( _INT8* buffer )
{
	BMP*	bmp;
	_INT16	i;
	_UINT32	palette_size;

	if ( buffer == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		printf("BMP_INVALID_ARGUMENT");
		return NULL;
	}


	/* Allocate */
	bmp = (BMP *)calloc( 1, sizeof( BMP ) );
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		printf("BMP_OUT_OF_MEMORY");
		return NULL;
	}


	/* Read header */
	i = ReadBufferHeader( bmp, buffer );
	if ( bmp->Header.Magic != 0x4D42 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		free( bmp );
		printf("\nBMP_FILE_INVALID => 0x%04x\n",bmp->Header.Magic);
		return NULL;
	}

	switch (bmp->Header.BitsPerPixel)
	{
	case 4:
		palette_size = BMP_PALETTE_SIZE_4bpp;
		break;
	case 8:
		palette_size = BMP_PALETTE_SIZE_8bpp;
		break;
	default:
		palette_size = 0;
		break;
	}

	// printf("\npalette size %lu\n", palette_size);

	/* Verify that the bitmap variant is supported */
	if ( ( bmp->Header.BitsPerPixel != 32 && bmp->Header.BitsPerPixel != 24 && bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4)
		|| bmp->Header.CompressionType != 0 || bmp->Header.HeaderSize != 40 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_SUPPORTED;
		free( bmp );
		printf("BMP_FILE_NOT_SUPPORTED");
		return NULL;
	}


	/* Allocate and read palette */
	if ( bmp->Header.BitsPerPixel == 8 || bmp->Header.BitsPerPixel == 4)
	{
		bmp->Palette = (_UINT8*) malloc( palette_size * sizeof( _UINT8 ) );
		if ( bmp->Palette == NULL )
		{
			BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
			free( bmp );
			printf("BMP_OUT_OF_MEMORY");
			return NULL;
		}
		memcpy( bmp->Palette, &buffer[i] , palette_size );
		if( bmp->Palette == NULL )
		{
			BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
			free( bmp->Palette );
			free( bmp );
			printf("BMP_FILE_INVALID bmp->Palette");
			return NULL;
		}
		i += palette_size;
	}
	else	/* Not an indexed image */
	{
		bmp->Palette = NULL;
	}


	/* Allocate memory for image data */
	bmp->Data = (_UINT8*) malloc( bmp->Header.ImageDataSize );
	if ( bmp->Data == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		free( bmp->Palette );
		free( bmp );
		printf("BMP_OUT_OF_MEMORY");
		return NULL;
	}


	/* Read image data */
	// printf("bmp->Header.ImageDataSize %lu - Offset %lu\n", bmp->Header.ImageDataSize, bmp->Header.DataOffset);
	memcpy( bmp->Data, &buffer[i] , bmp->Header.ImageDataSize );
	if ( bmp->Data == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		free( bmp->Data );
		free( bmp->Palette );
		free( bmp );
		printf("BMP_FILE_INVALID : bmp->Data is NULL ");
		return NULL;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return bmp;
}


/**************************************************************
	Reads the specified BMP image file.
**************************************************************/
BMP* BMP_ReadFile( const char* filename )
{
	BMP*	bmp;
	FILE*	f;
	_UINT32	palette_size;

	if ( filename == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return NULL;
	}


	/* Allocate */
	bmp = (BMP *)calloc( 1, sizeof( BMP ) );
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		return NULL;
	}


	/* Open file */
	f = fopen( filename, "rb" );
	if ( f == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_FOUND;
		free( bmp );
		return NULL;
	}


	/* Read header */
	if ( ReadHeader( bmp, f ) != BMP_OK || bmp->Header.Magic != 0x4D42 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		fclose( f );
		free( bmp );
		return NULL;
	}

	switch (bmp->Header.BitsPerPixel)
	{
	case 4:
		palette_size = BMP_PALETTE_SIZE_4bpp;
		break;
	case 8:
		palette_size = BMP_PALETTE_SIZE_8bpp;
		break;
	default:
		palette_size = 0;
		break;
	}

	/* Verify that the bitmap variant is supported */
	if ( ( bmp->Header.BitsPerPixel != 32 && bmp->Header.BitsPerPixel != 24 && bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4 )
		|| bmp->Header.CompressionType != 0 || bmp->Header.HeaderSize != 40 )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_SUPPORTED;
		fclose( f );
		free( bmp );
		return NULL;
	}


	/* Allocate and read palette */
	printf("bmp->Header.BitsPerPixel %d\n", bmp->Header.BitsPerPixel);
	if ( bmp->Header.BitsPerPixel == 8 || bmp->Header.BitsPerPixel == 4 )
	{
		bmp->Palette = (_UINT8*) malloc( palette_size * sizeof( _UINT8 ) );
		if ( bmp->Palette == NULL )
		{
			BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
			fclose( f );
			free( bmp );
			return NULL;
		}

		if ( fread( bmp->Palette, sizeof( _UINT8 ), palette_size, f ) != palette_size )
		{
			BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
			fclose( f );
			free( bmp->Palette );
			free( bmp );
			return NULL;
		}
	}
	else	/* Not an indexed image */
	{
		bmp->Palette = NULL;
	}

/* M.Medour 2023/09 - Fix ImageDataSize */
if(!bmp->Header.ImageDataSize){
	bmp->Header.ImageDataSize = bmp->Header.FileSize - bmp->Header.DataOffset;
}
	/* Allocate memory for image data */
	bmp->Data = (_UINT8*) malloc( bmp->Header.ImageDataSize );
	if ( bmp->Data == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_OUT_OF_MEMORY;
		fclose( f );
		free( bmp->Palette );
		free( bmp );
		return NULL;
	}


	/* Read image data */
	// printf("bmp->Header.ImageDataSize %lu\n", bmp->Header.ImageDataSize);
	if ( fread( bmp->Data, sizeof( char ), bmp->Header.ImageDataSize, f ) != bmp->Header.ImageDataSize )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_INVALID;
		fclose( f );
		free( bmp->Data );
		free( bmp->Palette );
		free( bmp );
		return NULL;
	}


	fclose( f );

	BMP_LAST_ERROR_CODE = BMP_OK;

	return bmp;
}


/**************************************************************
	Writes the BMP image to the specified file.
**************************************************************/
void BMP_WriteFile( BMP* bmp, const char* filename )
{
	FILE*	f;
	_UINT32	palette_size;

	switch (bmp->Header.BitsPerPixel)
	{
	case 4:
		palette_size = BMP_PALETTE_SIZE_4bpp;
		break;
	case 8:
		palette_size = BMP_PALETTE_SIZE_8bpp;
		break;
	default:
		palette_size = 0;
		break;
	}

	if ( filename == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return;
	}


	/* Open file */
	f = fopen( filename, "wb" );
	if ( f == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_FILE_NOT_FOUND;
		return;
	}


	/* Write header */
	if ( WriteHeader( bmp, f ) != BMP_OK )
	{
		BMP_LAST_ERROR_CODE = BMP_IO_ERROR;
		fclose( f );
		return;
	}


	/* Write palette */
	if ( bmp->Palette )
	{
		if ( fwrite( bmp->Palette, sizeof( _UINT8 ), palette_size, f ) != palette_size )
		{
			BMP_LAST_ERROR_CODE = BMP_IO_ERROR;
			fclose( f );
			return;
		}
	}


	/* Write data */
	if ( fwrite( bmp->Data, sizeof( _UINT8 ), bmp->Header.ImageDataSize, f ) != bmp->Header.ImageDataSize )
	{
		BMP_LAST_ERROR_CODE = BMP_IO_ERROR;
		fclose( f );
		return;
	}


	BMP_LAST_ERROR_CODE = BMP_OK;
	fclose( f );
}


/**************************************************************
	Returns the image's width.
**************************************************************/
_UINT32 BMP_GetWidth( BMP* bmp )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return -1;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return ( bmp->Header.Width );
}


/**************************************************************
	Returns the image's height.
**************************************************************/
_UINT32 BMP_GetHeight( BMP* bmp )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return -1;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return ( bmp->Header.Height );
}


/**************************************************************
	Returns the image's color depth (bits per pixel).
**************************************************************/
_UINT16 BMP_GetDepth( BMP* bmp )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
		return -1;
	}

	BMP_LAST_ERROR_CODE = BMP_OK;

	return ( bmp->Header.BitsPerPixel );
}


/**************************************************************
	Populates the arguments with the specified pixel's RGB
	values.
**************************************************************/
void BMP_GetPixelRGB( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8* r, _UINT8* g, _UINT8* b )
{
	_UINT8*	pixel;
	_UINT32	bytes_per_row;
	_UINT8	bytes_per_pixel;

	if ( bmp == NULL || x < 0 || x >= bmp->Header.Width || y < 0 || y >= bmp->Header.Height )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}
	else
	{
		BMP_LAST_ERROR_CODE = BMP_OK;

		bytes_per_pixel = bmp->Header.BitsPerPixel >> 3;

		/* Row's size is rounded up to the next multiple of 4 bytes */
		bytes_per_row = bmp->Header.ImageDataSize / bmp->Header.Height;

		/* Calculate the location of the relevant pixel (rows are flipped) */
		pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + x * bytes_per_pixel );


		/* In indexed color mode the pixel's value is an index within the palette */
		if ( bmp->Header.BitsPerPixel == 8 || bmp->Header.BitsPerPixel == 4)
		{
			/*
			Todo for 4bpp
			*/
			pixel = bmp->Palette + *pixel * 4;
		}

		/* Note: colors are stored in BGR order */
		if ( r )	*r = *( pixel + 2 );
		if ( g )	*g = *( pixel + 1 );
		if ( b )	*b = *( pixel + 0 );
	}
}


/**************************************************************
	Sets the specified pixel's RGB values.
**************************************************************/
void BMP_SetPixelRGB( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8 r, _UINT8 g, _UINT8 b )
{
	_UINT8*	pixel;
	_UINT32	bytes_per_row;
	_UINT8	bytes_per_pixel;

	if ( bmp == NULL || x < 0 || x >= bmp->Header.Width || y < 0 || y >= bmp->Header.Height )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}

	else if ( bmp->Header.BitsPerPixel != 24 && bmp->Header.BitsPerPixel != 32 )
	{
		BMP_LAST_ERROR_CODE = BMP_TYPE_MISMATCH;
	}

	else
	{
		BMP_LAST_ERROR_CODE = BMP_OK;

		bytes_per_pixel = bmp->Header.BitsPerPixel >> 3;

		/* Row's size is rounded up to the next multiple of 4 bytes */
		bytes_per_row = bmp->Header.ImageDataSize / bmp->Header.Height;

		/* Calculate the location of the relevant pixel (rows are flipped) */
		pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + x * bytes_per_pixel );

		/* Note: colors are stored in BGR order */
		*( pixel + 2 ) = r;
		*( pixel + 1 ) = g;
		*( pixel + 0 ) = b;
	}
}


/**************************************************************
	Gets the specified pixel's color index.
**************************************************************/
void BMP_GetPixelIndex( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8* val )
{
	_UINT8*	pixel;
	_UINT32	bytes_per_row;
	_UINT32	tx;

	if ( bmp == NULL || x < 0 || x >= bmp->Header.Width || y < 0 || y >= bmp->Header.Height )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}

	// else if ( bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4 )
	else if ( bmp->Palette == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_TYPE_MISMATCH;
	}

	else
	{
		BMP_LAST_ERROR_CODE = BMP_OK;

		/* Row's size is rounded up to the next multiple of 4 bytes */
		bytes_per_row = bmp->Header.ImageDataSize / bmp->Header.Height;

		if( bmp->Header.BitsPerPixel == 4){

		/* Calculate the location of the relevant pixel for 4bpp*/
			tx = (x % 2 == 0 ? x / 2 : (x - 1) / 2); //Divide x by two and round to floor
			pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + tx );

			if( x % 2 == 1 )
				*val = *pixel & 0x0F;
			else 
				*val = (*pixel & 0xF0) / 0x10;

		} else {
		/* Calculate the location of the relevant pixel */
		pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + x );


		if ( val )	*val = *pixel;
		}
	}
}


/**************************************************************
	Sets the specified pixel's color index.
**************************************************************/
void BMP_SetPixelIndex( BMP* bmp, _UINT32 x, _UINT32 y, _UINT8 val )
{
	_UINT8*	pixel;
	_UINT32	bytes_per_row;
	_UINT32	tx;

	if ( bmp == NULL || x < 0 || x >= bmp->Header.Width || y < 0 || y >= bmp->Header.Height )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}

	// else if ( bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4 )
	else if ( bmp->Palette == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_TYPE_MISMATCH;
	}

	else
	{
		BMP_LAST_ERROR_CODE = BMP_OK;

		/* Row's size is rounded up to the next multiple of 4 bytes */
		bytes_per_row = bmp->Header.ImageDataSize / bmp->Header.Height;

		if( bmp->Header.BitsPerPixel == 4){
			tx = (x % 2 == 0 ? x / 2 : (x - 1) / 2); //Divide x by two and round to floor
			pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + tx );
			//Put the 4bit value in *pixel
			if(x % 2 == 0)
			{
				*pixel &= 0x0F;
				*pixel |= val * 0x10; 
			}
			else
			{
				*pixel &= 0xF0;
				*pixel |= val; 
			}
		} else {
			/* Calculate the location of the relevant pixel */
			pixel = bmp->Data + ( ( bmp->Header.Height - y - 1 ) * bytes_per_row + x );

			*pixel = val;
		}


	}
}


/**************************************************************
	Gets the color value for the specified palette index.
**************************************************************/
void BMP_GetPaletteColor( BMP* bmp, _UINT8 index, _UINT8* r, _UINT8* g, _UINT8* b )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}

	// else if ( bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4 )
	else if ( bmp->Palette == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_TYPE_MISMATCH;
	}

	else
	{
		if ( r )	*r = *( bmp->Palette + index * 4 + 2 );
		if ( g )	*g = *( bmp->Palette + index * 4 + 1 );
		if ( b )	*b = *( bmp->Palette + index * 4 + 0 );

		BMP_LAST_ERROR_CODE = BMP_OK;
	}
}


/**************************************************************
	Sets the color value for the specified palette index.
**************************************************************/
void BMP_SetPaletteColor( BMP* bmp, _UINT8 index, _UINT8 r, _UINT8 g, _UINT8 b )
{
	if ( bmp == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_INVALID_ARGUMENT;
	}

	// else if ( bmp->Header.BitsPerPixel != 8 && bmp->Header.BitsPerPixel != 4 )
	else if ( bmp->Palette == NULL )
	{
		BMP_LAST_ERROR_CODE = BMP_TYPE_MISMATCH;
	}

	else
	{
		*( bmp->Palette + index * 4 + 2 ) = r;
		*( bmp->Palette + index * 4 + 1 ) = g;
		*( bmp->Palette + index * 4 + 0 ) = b;

		BMP_LAST_ERROR_CODE = BMP_OK;
	}
}


/**************************************************************
	Returns the last error code.
**************************************************************/
BMP_STATUS BMP_GetError()
{
	return BMP_LAST_ERROR_CODE;
}


/**************************************************************
	Returns a description of the last error code.
**************************************************************/
const char* BMP_GetErrorDescription()
{
	if ( BMP_LAST_ERROR_CODE > 0 && BMP_LAST_ERROR_CODE < BMP_ERROR_NUM )
	{
		return BMP_ERROR_STRING[ BMP_LAST_ERROR_CODE ];
	}
	else
	{
		return NULL;
	}
}





/*********************************** Private methods **********************************/


/**************************************************************
	Reads the BMP file's header into the data structure.
	Returns BMP_OK on success.
**************************************************************/
int	ReadHeader( BMP* bmp, FILE* f )
{
	if ( bmp == NULL || f == NULL )
	{
		return BMP_INVALID_ARGUMENT;
	}

	/* The header's fields are read one by one, and converted from the format's
	little endian to the system's native representation. */
	if ( !Read_UINT16( &( bmp->Header.Magic ), f ) )			return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.FileSize ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT16( &( bmp->Header.Reserved1 ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT16( &( bmp->Header.Reserved2 ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.DataOffset ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.HeaderSize ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.Width ), f ) )			return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.Height ), f ) )			return BMP_IO_ERROR;
	if ( !Read_UINT16( &( bmp->Header.Planes ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT16( &( bmp->Header.BitsPerPixel ), f ) )	return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.CompressionType ), f ) )	return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.ImageDataSize ), f ) )	return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.HPixelsPerMeter ), f ) )	return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.VPixelsPerMeter ), f ) )	return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.ColorsUsed ), f ) )		return BMP_IO_ERROR;
	if ( !Read_UINT32( &( bmp->Header.ColorsRequired ), f ) )	return BMP_IO_ERROR;

	return BMP_OK;
}

int ReadBufferHeader( BMP* bmp, _INT8 *buffer ){
	if ( bmp == NULL || buffer == NULL )
	{
		return BMP_INVALID_ARGUMENT;
	}
	_INT16 i = 0;
	/* The header's fields are read one by one, and converted from the format's
	little endian to the system's native representation. */
	if ( !ReadBuffer_UINT16( &( bmp->Header.Magic ), buffer, i ) )			return BMP_IO_ERROR;
	i += sizeof(_UINT16);
	if ( !ReadBuffer_UINT32( &( bmp->Header.FileSize ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT16( &( bmp->Header.Reserved1 ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT16);
	if ( !ReadBuffer_UINT16( &( bmp->Header.Reserved2 ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT16);
	if ( !ReadBuffer_UINT32( &( bmp->Header.DataOffset ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.HeaderSize ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &bmp->Header.Width, buffer, i ) )			return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.Height ), buffer, i ) )			return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT16( &( bmp->Header.Planes ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT16);
	if ( !ReadBuffer_UINT16( &( bmp->Header.BitsPerPixel ), buffer, i ) )	return BMP_IO_ERROR;
	i += sizeof(_UINT16);
	if ( !ReadBuffer_UINT32( &( bmp->Header.CompressionType ), buffer, i ) )	return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.ImageDataSize ), buffer, i ) )	return BMP_IO_ERROR;
	if (bmp->Header.ImageDataSize == 0)
	{
		bmp->Header.ImageDataSize = bmp->Header.FileSize - bmp->Header.DataOffset;
	}
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.HPixelsPerMeter ), buffer, i ) )	return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.VPixelsPerMeter ), buffer, i ) )	return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.ColorsUsed ), buffer, i ) )		return BMP_IO_ERROR;
	i += sizeof(_UINT32);
	if ( !ReadBuffer_UINT32( &( bmp->Header.ColorsRequired ), buffer, i ) )	return BMP_IO_ERROR;
	i += sizeof(_UINT32);

	// printf("width %d\n", bmp->Header.Width);

	// printf("height %d\n", bmp->Header.Height);
	// printf("planes %d vs bitsPerPixel %d\n", bmp->Header.Planes, bmp->Header.BitsPerPixel);
	// printf("image size %d\n", bmp->Header.ImageDataSize);

	// printf("offset %d\n", bmp->Header.DataOffset);
	// printf("compression method %d\n", bmp->Header.CompressionType);

	return i;
}
/**************************************************************
	Writes the BMP file's header into the data structure.
	Returns BMP_OK on success.
**************************************************************/
int	WriteHeader( BMP* bmp, FILE* f )
{
	if ( bmp == NULL || f == NULL )
	{
		return BMP_INVALID_ARGUMENT;
	}

	/* The header's fields are written one by one, and converted to the format's
	little endian representation. */
	if ( !Write_UINT16( bmp->Header.Magic, f ) )			return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.FileSize, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT16( bmp->Header.Reserved1, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT16( bmp->Header.Reserved2, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.DataOffset, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.HeaderSize, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.Width, f ) )			return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.Height, f ) )			return BMP_IO_ERROR;
	if ( !Write_UINT16( bmp->Header.Planes, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT16( bmp->Header.BitsPerPixel, f ) )	return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.CompressionType, f ) )	return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.ImageDataSize, f ) )	return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.HPixelsPerMeter, f ) )	return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.VPixelsPerMeter, f ) )	return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.ColorsUsed, f ) )		return BMP_IO_ERROR;
	if ( !Write_UINT32( bmp->Header.ColorsRequired, f ) )	return BMP_IO_ERROR;

	return BMP_OK;
}


/**************************************************************
	Reads a little-endian unsigned int from the file.
	Returns non-zero on success.
**************************************************************/
int	Read_UINT32( _UINT32* x, FILE* f )
{
	_UINT8 little[ 4 ];	/* BMPs use 32 bit ints */

	if ( x == NULL || f == NULL )
	{
		return 0;
	}

	if ( fread( little, 4, 1, f ) != 1 )
	{
		return 0;
	}

	*x = ( little[ 3 ] << 24 | little[ 2 ] << 16 | little[ 1 ] << 8 | little[ 0 ] );

	return 1;
}

/**************************************************************
	Reads a little-endian unsigned int from the buffer.
	Returns non-zero on success.
**************************************************************/
int	ReadBuffer_UINT32( _UINT32* x, _INT8* f , _INT16 i)
{
	_UINT8 little[ 4 ];	/* BMPs use 32 bit ints */

	if ( x == NULL || f == NULL )
	{
		return 0;
	}
	// printf("i = %d\n", i);
	memcpy(little, &f[i], 4);

	*x = ( little[ 3 ] << 24 | little[ 2 ] << 16 | little[ 1 ] << 8 | little[ 0 ] );
	// printf("X = 0X%08x\n", *x);
	// printf("X = %d\n", *x);
	return 1;
}

/**************************************************************
	Reads a little-endian unsigned short int from the file.
	Returns non-zero on success.
**************************************************************/
int	Read_UINT16( _UINT16 *x, FILE* f )
{
	_UINT8 little[ 2 ];	/* BMPs use 16 bit shorts */

	if ( x == NULL || f == NULL )
	{
		return 0;
	}

	if ( fread( little, 2, 1, f ) != 1 )
	{
		return 0;
	}

	*x = ( little[ 1 ] << 8 | little[ 0 ] );
	
	return 1;
}

/**************************************************************
	Reads a little-endian unsigned short int from the buffer.
	Returns non-zero on success.
**************************************************************/
int	ReadBuffer_UINT16( _UINT16 *x, _INT8* f , _INT16 i)
{
	_UINT8 little[ 2 ];	/* BMPs use 16 bit shorts */

	if ( x == NULL || f == NULL )
	{
		return 0;
	}
	// printf("i = %d\n", i);
	memcpy(little, &f[i], 2);

	*x = ( little[ 1 ] << 8 | little[ 0 ] );
	// printf("X = 0X%04x\n", *x);
	// printf("X = %0d\n", *x);
	
	return 1;
}


/**************************************************************
	Writes a little-endian unsigned int to the file.
	Returns non-zero on success.
**************************************************************/
int	Write_UINT32( _UINT32 x, FILE* f )
{
	_UINT8 little[ 4 ];	/* BMPs use 32 bit ints */

	little[ 3 ] = (_UINT8)( ( x & 0xff000000 ) >> 24 );
	little[ 2 ] = (_UINT8)( ( x & 0x00ff0000 ) >> 16 );
	little[ 1 ] = (_UINT8)( ( x & 0x0000ff00 ) >> 8 );
	little[ 0 ] = (_UINT8)( ( x & 0x000000ff ) >> 0 );

	return ( f && fwrite( little, 4, 1, f ) == 1 );
}


/**************************************************************
	Writes a little-endian unsigned short int to the file.
	Returns non-zero on success.
**************************************************************/
int	Write_UINT16( _UINT16 x, FILE* f )
{
	_UINT8 little[ 2 ];	/* BMPs use 16 bit shorts */

	little[ 1 ] = (_UINT8)( ( x & 0xff00 ) >> 8 );
	little[ 0 ] = (_UINT8)( ( x & 0x00ff ) >> 0 );

	return ( f && fwrite( little, 2, 1, f ) == 1 );
}


#include "dither.h"

typedef	u_int32_t pixel;

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef mul_3_fast
#define mul_3_fast(x) ((x << 1) + x)
#endif

#define	CLAMPED( x, xmin, xmax )	MAX( (xmin), MIN( (xmax), (x) ) )
#define	SIERRA_LITE_COEF( v, err )	((( (err) * ((v) << 8)) >> 2) >> 8)
#define	SIERRA_COEF( v, err )	((( (err) * ((v) << 8)) >> 5) >> 8)

#define		CLAMP( x, xmin, xmax )		(x)	= MAX( (xmin), (x) );	\
										(x)	= MIN( (xmax), (x) )

const u_int8_t	VALUES_6BPP[]	= {	0,  85, 170, 255	};

#define	f7_16	112		//const long	f7	= (7 << 8) / 16;
#define	f5_16	 80		//const long	f5	= (5 << 8) / 16;
#define	f3_16	 48		//const long	f3	= (3 << 8) / 16;
#define	f1_16	 16		//const long	f1	= (1 << 8) / 16;

/////////////////////////////////////////////////////////////////////////////

//	Back-white Floyd-Steinberg dither
void makeDitherFS( u_int8_t* pixels, long width, long height ) {
	const long	size	= width * height;

	long*	error	= (long*)Mxalloc( size * sizeof(long), 3 );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( long x = 0; x < width; x++,i++ )
		{
			const long	blue	= prow[mul_3_fast(x) + 0];
            const long	green	= prow[mul_3_fast(x) + 1];
            const long	red		= prow[mul_3_fast(x) + 2];

			//	Get the pixel gray value.
			// long	newVal	= (red+green+blue)/3 + (error[i] >> 8);	//	PixelGray + error correction
			long	newVal	= (red + green + blue) / 3 + (error[i] >> 8);
			long	newc	= (newVal < 128 ? 0 : 255);
            prow[mul_3_fast(x) + 0]	= newc;	//	blue
            prow[mul_3_fast(x) + 1]	= newc;	//	green
            prow[mul_3_fast(x) + 2]	= newc;	//	red

			//	Correction - the new error
			const long	cerror	= newVal - newc;

			long idx = i+1;
			if( x+1 < width )
				error[idx] += (cerror * f7_16);

			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
				error[idx] += (cerror * f3_16);

			idx++;
			if( y+1 < height )
				error[idx] += (cerror * f5_16);

			idx++;
			if( x+1 < width && y+1 < height )
				error[idx] += (cerror * f1_16);
		}
	}

	Mfree( error );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 3 bits per pixel (1 bit per color plane)
void	makeDitherFSRgb3bpp( u_int8_t* pixels, long width, long height )	
{
	const long	size	= width * height;

	long*	errorB	= (long*)malloc( size * sizeof(long) );
	long*	errorG	= (long*)malloc( size * sizeof(long) );
	long*	errorR	= (long*)malloc( size * sizeof(long) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(long) );
	memset( errorG, 0, size * sizeof(long) );
	memset( errorR, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( long x = 0; x < width; x++,i++ )
		{
			const long	blue	= prow[mul_3_fast(x)];
            const long	green	= prow[mul_3_fast(x) + 1];
            const long	red		= prow[mul_3_fast(x) + 2];

			long	newValB	= blue  + (errorB[i] >> 8);	//	PixelRed   + error correctionB
			long	newValG	= green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			long	newValR	= red   + (errorR[i] >> 8);	//	PixelBlue  + error correctionR

			long	newcb	= (newValB < 128 ? 0 : 255);
			long	newcg	= (newValG < 128 ? 0 : 255);
			long	newcr	= (newValR < 128 ? 0 : 255);

            prow[mul_3_fast(x)]	= newcb;
            prow[mul_3_fast(x) + 1]	= newcg;
            prow[mul_3_fast(x) + 2]	= newcr;

			//	Correction - the new error
			long	cerrorR	= newValR - newcr;
			long	cerrorG	= newValG - newcg;
			long	cerrorB	= newValB - newcb;

			long	idx	= i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}

			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}

			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				// errorR[idx] += (cerrorR * f1_16);
				// errorG[idx] += (cerrorG * f1_16);
				// errorB[idx] += (cerrorB * f1_16);
				errorR[idx] += (cerrorR << 4);
				errorG[idx] += (cerrorG << 4);
				errorB[idx] += (cerrorB << 4);				
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Color Floyd-Steinberg dither using 6 bits per pixel (2 bit per color plane)
void	makeDitherFSRgb6bpp( u_int8_t* pixels, long width, long height )	
{
	const long	size	= width * height;

	long*	errorB	= (long*)malloc( size * sizeof(long) );
	long*	errorG	= (long*)malloc( size * sizeof(long) );
	long*	errorR	= (long*)malloc( size * sizeof(long) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(long) );
	memset( errorG, 0, size * sizeof(long) );
	memset( errorR, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( long x = 0; x < width; x++,i++ )
		{
			const long	blue	= prow[mul_3_fast(x) + 0];
            const long	green	= prow[mul_3_fast(x) + 1];
            const long	red		= prow[mul_3_fast(x) + 2];

			long	newValB	= (long)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			long	newValG	= (long)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			long	newValR	= (long)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			long	idxR	= CLAMPED( newValR, 0, 255 );
			long	idxG	= CLAMPED( newValG, 0, 255 );
			long	idxB	= CLAMPED( newValB, 0, 255 );

			long	newcR	= VALUES_6BPP[idxR >> 6];	//	x >> 6 is the same as x / 64
			long	newcG	= VALUES_6BPP[idxG >> 6];	//	x >> 6 is the same as x / 64
			long	newcB	= VALUES_6BPP[idxB >> 6];	//	x >> 6 is the same as x / 64

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			long cerrorB	= newValB - newcB;
			long cerrorG	= newValG - newcG;
			long cerrorR	= newValR - newcR;

			long	idx = i+1;
			if( x+1 < width )
			{
				errorR[idx] += (cerrorR * f7_16);
				errorG[idx] += (cerrorG * f7_16);
				errorB[idx] += (cerrorB * f7_16);
			}
			
			idx += width - 2;
			if( x-1 > 0 && y+1 < height )
			{
				errorR[idx] += (cerrorR * f3_16);
				errorG[idx] += (cerrorG * f3_16);
				errorB[idx] += (cerrorB * f3_16);
			}
			
			idx++;
			if( y+1 < height )
			{
				errorR[idx] += (cerrorR * f5_16);
				errorG[idx] += (cerrorG * f5_16);
				errorB[idx] += (cerrorB * f5_16);
			}
			
			idx++;
			if( x+1 < width && y+1 < height )
			{
				errorR[idx] += (cerrorR * f1_16);
				errorG[idx] += (cerrorG * f1_16);
				errorB[idx] += (cerrorB * f1_16);
			}
		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

//	Black-white Sierra Lite dithering (variation of Floyd-Steinberg with less computational cost)
void	makeDitherSierra( u_int8_t* pixels, long width, long height )	
{
	//	To avoid real number calculations, I will raise the level of INT arythmetics by shifting with 8 bits to the left ( << 8 )
	//	Later, when it is necessary will return to te normal level by shifting back 8 bits to the right ( >> 8 )
	//	       X   5   3
    //	2   4  5   4   2
    //	    2  3   2
    //	    (1/32)

	//~~~~~~~~

	const long	size	= width * height;

	long*	error	= (long*)malloc( size * sizeof(long) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		for( long x = 0; x < width; x++,i++ )
		{
			const pixel	blue	= pixels[mul_3_fast(x)];
            const pixel	green	= pixels[mul_3_fast(x) + 1];
            const pixel	red		= pixels[mul_3_fast(x) + 2];

			//	Get the pixel gray value.
			long	newVal	= (red + green + blue) / 3 + error[i];		//	PixelGray + error correction
			// long	newVal	= GRAY(red , green , blue) + error[i];		//	PixelGray + error correction
			long	newc	= (newVal < 128 ? 0 : 255);

			pixels[mul_3_fast(x)]	= newc;
			pixels[mul_3_fast(x) + 1]	= newc;
			pixels[mul_3_fast(x) + 2]	= newc;

			//	Correction - the new error
			const long	cerror	= newVal - newc;

			long idx = i;
			if( x + 1 < width )
				error[idx+1] += SIERRA_COEF( 5, cerror );

			if( x + 2 < width )
				error[idx+2] += SIERRA_COEF( 3, cerror );

			if( y + 1 < height )
			{
				idx += width;
				if( x-2 >= 0 )
					error[idx-2] += SIERRA_COEF( 2, cerror );
				
				if( x-1 >= 0 )
					error[idx-1] += SIERRA_COEF( 4, cerror );

				error[idx] += SIERRA_COEF( 5, cerror );

				if( x+1 < width )
					error[idx+1] += SIERRA_COEF( 4, cerror );

				if( x+2 < width )
					error[idx+2] += SIERRA_COEF( 2, cerror );
			}

			if( y + 2 < height )
			{
				idx	+= width;
				if( x-1 >= 0 )
					error[idx-1] += SIERRA_COEF( 2, cerror );

				error[idx] += SIERRA_COEF( 3, cerror );

				if( x+1 < width )
					error[idx+1] += SIERRA_COEF( 2, cerror );
			}
		}
		
		pixels	+= width*3;
	}

	free( error );
}
/////////////////////////////////////////////////////////////////////////////

void makeDitherSierraLiteRgbNbpp( u_int8_t* pixels, long width, long height, long ncolors ) {
	long	divider	= 256 / ncolors;

	const long	size	= width * height;

	long*	errorB	= (long*)Mxalloc( size * sizeof(long) , 3);
	long*	errorG	= (long*)Mxalloc( size * sizeof(long) , 3);
	long*	errorR	= (long*)Mxalloc( size * sizeof(long) , 3);

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(long) );
	memset( errorG, 0, size * sizeof(long) );
	memset( errorR, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( long x = 0; x < width; x++,i++ )
		{
			const long	blue	= prow[mul_3_fast(x)];
            const long	green	= prow[mul_3_fast(x) + 1];
            const long	red		= prow[mul_3_fast(x) + 2];

			const long	newValB	= blue	+ errorB[i];
			const long	newValG	= green + errorG[i];
			const long	newValR	= red	+ errorR[i];

			long	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			long	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			long	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const long	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const long	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const long	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			const long cerrorB	= (newValB - newcB);
			const long cerrorG	= (newValG - newcG);
			const long cerrorR	= (newValR - newcR);

			long	idx = i;
			if( x+1 < width )
			{
				errorR[idx+1] += SIERRA_LITE_COEF( 2, cerrorR );
				errorG[idx+1] += SIERRA_LITE_COEF( 2, cerrorG );
				errorB[idx+1] += SIERRA_LITE_COEF( 2, cerrorB );
			}
			
			idx += width;
			if( y + 1 < height )
			{
				if( x-1 > 0 && y+1 < height )
				{
					errorR[idx-1] += SIERRA_LITE_COEF( 1, cerrorR );
					errorG[idx-1] += SIERRA_LITE_COEF( 1, cerrorG );
					errorB[idx-1] += SIERRA_LITE_COEF( 1, cerrorB );
				}

				errorR[idx] += SIERRA_LITE_COEF( 1, cerrorR );
				errorG[idx] += SIERRA_LITE_COEF( 1, cerrorG );
				errorB[idx] += SIERRA_LITE_COEF( 1, cerrorB );
			}
		}
	}

	Mfree( errorB );
	Mfree( errorG );
	Mfree( errorR );
}
/////////////////////////////////////////////////////////////////////////////

void makeDitherSierraRgbNbpp( u_int8_t* pixels, long width, long height, long ncolors ) {
	long	divider	= 256 / ncolors;

	const long	size	= width * height;

	long*	errorB	= (long*)malloc( size * sizeof(long) );
	long*	errorG	= (long*)malloc( size * sizeof(long) );
	long*	errorR	= (long*)malloc( size * sizeof(long) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(long) );
	memset( errorG, 0, size * sizeof(long) );
	memset( errorR, 0, size * sizeof(long) );

	//~~~~~~~~

	long	i	= 0;

	for( long y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( long x = 0; x < width; x++,i++ )
		{
			const long	blue	= prow[mul_3_fast(x) + 0];
            const long	green	= prow[mul_3_fast(x) + 1];
            const long	red		= prow[mul_3_fast(x) + 2];

			const long	newValB	= blue	+ errorB[i];
			const long	newValG	= green + errorG[i];
			const long	newValR	= red	+ errorR[i];

			long	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			long	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			long	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const long	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const long	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const long	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			const long cerrorB	= (newValB - newcB);
			const long cerrorG	= (newValG - newcG);
			const long cerrorR	= (newValR - newcR);

			long idx = i;
			if( x + 1 < width )
			{
				errorR[idx+1] += SIERRA_COEF( 5, cerrorR );
				errorG[idx+1] += SIERRA_COEF( 5, cerrorG );
				errorB[idx+1] += SIERRA_COEF( 5, cerrorB );
			}

			if( x + 2 < width )
			{
				errorR[idx+2] += SIERRA_COEF( 3, cerrorR );
				errorG[idx+2] += SIERRA_COEF( 3, cerrorG );
				errorB[idx+2] += SIERRA_COEF( 3, cerrorB );
			}

			if( y + 1 < height )
			{
				idx += width;
				if( x-2 >= 0 )
				{
					errorR[idx-2] += SIERRA_COEF( 2, cerrorR );
					errorG[idx-2] += SIERRA_COEF( 2, cerrorG );
					errorB[idx-2] += SIERRA_COEF( 2, cerrorB );
				}


				if( x-1 >= 0 )
				{
					errorR[idx-1] += SIERRA_COEF( 4, cerrorR );
					errorG[idx-1] += SIERRA_COEF( 4, cerrorG );
					errorB[idx-1] += SIERRA_COEF( 4, cerrorB );
				}

				errorR[idx] += SIERRA_COEF( 5, cerrorR );
				errorG[idx] += SIERRA_COEF( 5, cerrorG );
				errorB[idx] += SIERRA_COEF( 5, cerrorB );

				if( x+1 < width )
				{
					errorR[idx+1] += SIERRA_COEF( 4, cerrorR );
					errorG[idx+1] += SIERRA_COEF( 4, cerrorG );
					errorB[idx+1] += SIERRA_COEF( 4, cerrorB );
				}

				if( x+2 < width )
				{
					errorR[idx+2] += SIERRA_COEF( 2, cerrorR );
					errorG[idx+2] += SIERRA_COEF( 2, cerrorG );
					errorB[idx+2] += SIERRA_COEF( 2, cerrorB );
				}
			}

			if( y + 2 < height )
			{
				idx	+= width;
				if( x-1 >= 0 )
				{
					errorR[idx-1] += SIERRA_COEF( 2, cerrorR );
					errorG[idx-1] += SIERRA_COEF( 2, cerrorG );
					errorB[idx-1] += SIERRA_COEF( 2, cerrorB );
				}

				errorR[idx] += SIERRA_COEF( 3, cerrorR );
				errorG[idx] += SIERRA_COEF( 3, cerrorG );
				errorB[idx] += SIERRA_COEF( 3, cerrorB );

				if( x+1 < width )
				{
					errorR[idx+1] += SIERRA_COEF( 2, cerrorR );
					errorG[idx+1] += SIERRA_COEF( 2, cerrorG );
					errorB[idx+1] += SIERRA_COEF( 2, cerrorB );
				}
			}

		}
	}

	free( errorB );
	free( errorG );
	free( errorR );
}
/////////////////////////////////////////////////////////////////////////////

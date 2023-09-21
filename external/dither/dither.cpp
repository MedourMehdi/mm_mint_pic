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

#define	f7_16	112		//const int32_t	f7	= (7 << 8) / 16;
#define	f5_16	 80		//const int32_t	f5	= (5 << 8) / 16;
#define	f3_16	 48		//const int32_t	f3	= (3 << 8) / 16;
#define	f1_16	 16		//const int32_t	f1	= (1 << 8) / 16;

/////////////////////////////////////////////////////////////////////////////

//	Back-white Floyd-Steinberg dither
void makeDitherFS( u_int8_t* pixels, int32_t width, int32_t height ) {
	const int32_t	size	= width * height;

	int32_t*	error	= (int32_t*)Mxalloc( size * sizeof(int32_t), 3 );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( int32_t x = 0; x < width; x++,i++ )
		{
			const int32_t	blue	= prow[mul_3_fast(x) + 0];
            const int32_t	green	= prow[mul_3_fast(x) + 1];
            const int32_t	red		= prow[mul_3_fast(x) + 2];

			//	Get the pixel gray value.
			// int32_t	newVal	= (red+green+blue)/3 + (error[i] >> 8);	//	PixelGray + error correction
			int32_t	newVal	= (red + green + blue) / 3 + (error[i] >> 8);
			int32_t	newc	= (newVal < 128 ? 0 : 255);
            prow[mul_3_fast(x) + 0]	= newc;	//	blue
            prow[mul_3_fast(x) + 1]	= newc;	//	green
            prow[mul_3_fast(x) + 2]	= newc;	//	red

			//	Correction - the new error
			const int32_t	cerror	= newVal - newc;

			int32_t idx = i+1;
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
void	makeDitherFSRgb3bpp( u_int8_t* pixels, int32_t width, int32_t height )	
{
	const int32_t	size	= width * height;

	int32_t*	errorB	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorG	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorR	= (int32_t*)malloc( size * sizeof(int32_t) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int32_t) );
	memset( errorG, 0, size * sizeof(int32_t) );
	memset( errorR, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( int32_t x = 0; x < width; x++,i++ )
		{
			const int32_t	blue	= prow[mul_3_fast(x)];
            const int32_t	green	= prow[mul_3_fast(x) + 1];
            const int32_t	red		= prow[mul_3_fast(x) + 2];

			int32_t	newValB	= blue  + (errorB[i] >> 8);	//	PixelRed   + error correctionB
			int32_t	newValG	= green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int32_t	newValR	= red   + (errorR[i] >> 8);	//	PixelBlue  + error correctionR

			int32_t	newcb	= (newValB < 128 ? 0 : 255);
			int32_t	newcg	= (newValG < 128 ? 0 : 255);
			int32_t	newcr	= (newValR < 128 ? 0 : 255);

            prow[mul_3_fast(x)]	= newcb;
            prow[mul_3_fast(x) + 1]	= newcg;
            prow[mul_3_fast(x) + 2]	= newcr;

			//	Correction - the new error
			int32_t	cerrorR	= newValR - newcr;
			int32_t	cerrorG	= newValG - newcg;
			int32_t	cerrorB	= newValB - newcb;

			int32_t	idx	= i+1;
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
void	makeDitherFSRgb6bpp( u_int8_t* pixels, int32_t width, int32_t height )	
{
	const int32_t	size	= width * height;

	int32_t*	errorB	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorG	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorR	= (int32_t*)malloc( size * sizeof(int32_t) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int32_t) );
	memset( errorG, 0, size * sizeof(int32_t) );
	memset( errorR, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( int32_t x = 0; x < width; x++,i++ )
		{
			const int32_t	blue	= prow[mul_3_fast(x) + 0];
            const int32_t	green	= prow[mul_3_fast(x) + 1];
            const int32_t	red		= prow[mul_3_fast(x) + 2];

			int32_t	newValB	= (int32_t)blue	 + (errorB[i] >> 8);	//	PixelBlue  + error correctionB
			int32_t	newValG	= (int32_t)green + (errorG[i] >> 8);	//	PixelGreen + error correctionG
			int32_t	newValR	= (int32_t)red	 + (errorR[i] >> 8);	//	PixelRed   + error correctionR

			//	The error could produce values beyond the borders, so need to keep the color in range
			int32_t	idxR	= CLAMPED( newValR, 0, 255 );
			int32_t	idxG	= CLAMPED( newValG, 0, 255 );
			int32_t	idxB	= CLAMPED( newValB, 0, 255 );

			int32_t	newcR	= VALUES_6BPP[idxR >> 6];	//	x >> 6 is the same as x / 64
			int32_t	newcG	= VALUES_6BPP[idxG >> 6];	//	x >> 6 is the same as x / 64
			int32_t	newcB	= VALUES_6BPP[idxB >> 6];	//	x >> 6 is the same as x / 64

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			int32_t cerrorB	= newValB - newcB;
			int32_t cerrorG	= newValG - newcG;
			int32_t cerrorR	= newValR - newcR;

			int32_t	idx = i+1;
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
void	makeDitherSierra( u_int8_t* pixels, int32_t width, int32_t height )	
{
	//	To avoid real number calculations, I will raise the level of INT arythmetics by shifting with 8 bits to the left ( << 8 )
	//	Later, when it is necessary will return to te normal level by shifting back 8 bits to the right ( >> 8 )
	//	       X   5   3
    //	2   4  5   4   2
    //	    2  3   2
    //	    (1/32)

	//~~~~~~~~

	const int32_t	size	= width * height;

	int32_t*	error	= (int32_t*)malloc( size * sizeof(int32_t) );

	//	Clear the errors buffer.
	memset( error, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		for( int32_t x = 0; x < width; x++,i++ )
		{
			const pixel	blue	= pixels[mul_3_fast(x)];
            const pixel	green	= pixels[mul_3_fast(x) + 1];
            const pixel	red		= pixels[mul_3_fast(x) + 2];

			//	Get the pixel gray value.
			int32_t	newVal	= (red + green + blue) / 3 + error[i];		//	PixelGray + error correction
			// int32_t	newVal	= GRAY(red , green , blue) + error[i];		//	PixelGray + error correction
			int32_t	newc	= (newVal < 128 ? 0 : 255);

			pixels[mul_3_fast(x)]	= newc;
			pixels[mul_3_fast(x) + 1]	= newc;
			pixels[mul_3_fast(x) + 2]	= newc;

			//	Correction - the new error
			const int32_t	cerror	= newVal - newc;

			int32_t idx = i;
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

void makeDitherSierraLiteRgbNbpp( u_int8_t* pixels, int32_t width, int32_t height, int32_t ncolors ) {
	int32_t	divider	= 256 / ncolors;

	const int32_t	size	= width * height;

	int32_t*	errorB	= (int32_t*)Mxalloc( size * sizeof(int32_t) , 3);
	int32_t*	errorG	= (int32_t*)Mxalloc( size * sizeof(int32_t) , 3);
	int32_t*	errorR	= (int32_t*)Mxalloc( size * sizeof(int32_t) , 3);

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int32_t) );
	memset( errorG, 0, size * sizeof(int32_t) );
	memset( errorR, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( int32_t x = 0; x < width; x++,i++ )
		{
			const int32_t	blue	= prow[mul_3_fast(x)];
            const int32_t	green	= prow[mul_3_fast(x) + 1];
            const int32_t	red		= prow[mul_3_fast(x) + 2];

			const int32_t	newValB	= blue	+ errorB[i];
			const int32_t	newValG	= green + errorG[i];
			const int32_t	newValR	= red	+ errorR[i];

			int32_t	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			int32_t	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			int32_t	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const int32_t	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const int32_t	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const int32_t	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			const int32_t cerrorB	= (newValB - newcB);
			const int32_t cerrorG	= (newValG - newcG);
			const int32_t cerrorR	= (newValR - newcR);

			int32_t	idx = i;
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

void makeDitherSierraRgbNbpp( u_int8_t* pixels, int32_t width, int32_t height, int32_t ncolors ) {
	int32_t	divider	= 256 / ncolors;

	const int32_t	size	= width * height;

	int32_t*	errorB	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorG	= (int32_t*)malloc( size * sizeof(int32_t) );
	int32_t*	errorR	= (int32_t*)malloc( size * sizeof(int32_t) );

	//	Clear the errors buffer.
	memset( errorB, 0, size * sizeof(int32_t) );
	memset( errorG, 0, size * sizeof(int32_t) );
	memset( errorR, 0, size * sizeof(int32_t) );

	//~~~~~~~~

	int32_t	i	= 0;

	for( int32_t y = 0; y < height; y++ )
	{
		u_int8_t*   prow   = pixels + ( y * width * 3 );

		for( int32_t x = 0; x < width; x++,i++ )
		{
			const int32_t	blue	= prow[mul_3_fast(x) + 0];
            const int32_t	green	= prow[mul_3_fast(x) + 1];
            const int32_t	red		= prow[mul_3_fast(x) + 2];

			const int32_t	newValB	= blue	+ errorB[i];
			const int32_t	newValG	= green + errorG[i];
			const int32_t	newValR	= red	+ errorR[i];

			int32_t	i1	= newValB / divider;	CLAMP( i1, 0, ncolors );
			int32_t	i2	= newValG / divider;	CLAMP( i2, 0, ncolors );
			int32_t	i3	= newValR / divider;	CLAMP( i3, 0, ncolors );

			//	If you want to compress the image, use the values of i1,i2,i3
			//	they have values in the range 0..ncolors
			//	So if the ncolors is 4 - you have values: 0,1,2,3 which is encoded in 2 bits
			//	2 bits for 3 planes == 6 bits per pixel

			const int32_t	newcB	= CLAMPED( i1 * divider, 0, 255 );	//	blue
			const int32_t	newcG	= CLAMPED( i2 * divider, 0, 255 );	//	green
			const int32_t	newcR	= CLAMPED( i3 * divider, 0, 255 );	//	red

            prow[mul_3_fast(x) + 0]	= newcB;
            prow[mul_3_fast(x) + 1]	= newcG;
            prow[mul_3_fast(x) + 2]	= newcR;

			const int32_t cerrorB	= (newValB - newcB);
			const int32_t cerrorG	= (newValG - newcG);
			const int32_t cerrorR	= (newValR - newcR);

			int32_t idx = i;
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

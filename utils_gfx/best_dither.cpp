#include "best_dither.h"

#ifndef CLAMP
#define		CLAMP( x, xmin, xmax )		(x)	= MAX( (xmin), (x) );	\
										(x)	= MIN( (xmax), (x) )
#endif

typedef struct RGB {
	int r;
	int g;
	int b;
} RGB;

void setRGB(u_int32_t* pixels, int x, int y, RGB rgb, int w, int h) {
	if (x < 0 || x >= w || y < 0 || y >= h) return;
	
	pixels[y * w + x] = 0xff000000 + (rgb.b) + (rgb.g << 8) + (rgb.r << 16);
}

RGB getRGB(u_int32_t* pixels, int x, int y, int w, int h) {
	// RGB rgb; 
	// rgb.r = 0; rgb.g = 0; rgb.b = 0;
	
	if (x < 0 || x >= w || y < 0 || y >= h) return (RGB){0, 0, 0};
	
	u_int8_t *pix_value = (u_int8_t *)&pixels[y * w + x];

	pix_value++;

	// rgb.b = (pixels[y * w + x] & 0xff);
	// rgb.g = (pixels[y * w + x] & 0xff00) >> 8;
	// rgb.r = (pixels[y * w + x] & 0xff0000) >> 16;

	// return rgb;
	return (RGB){*pix_value++,*pix_value++,*pix_value++};
}

RGB difRGB(RGB from, RGB to) {
	RGB dif;
	dif.r = to.r - from.r;
	dif.g = to.g - from.g;
	dif.b = to.b - from.b;
	
	return dif;
}

RGB addRGB(RGB a, RGB b) {
	RGB sum;
	sum.r = a.r + b.r;
	sum.g = a.g + b.g;
	sum.b = a.b + b.b;
	
	if (sum.r > 255) sum.r = 255; if (sum.r < 0) sum.r = 0;
	if (sum.g > 255) sum.g = 255; if (sum.g < 0) sum.g = 0;
	if (sum.b > 255) sum.b = 255; if (sum.b < 0) sum.b = 0;
	
	return sum;
}

RGB mulRGB(RGB rgb, double d) {
	RGB mul;
	mul.r = (int)((double)rgb.r*d);
	mul.g = (int)((double)rgb.g*d);
	mul.b = (int)((double)rgb.b*d);
	
	return mul;
}

RGB divRGB(RGB rgb, double d) {
	RGB div;
	div.r = (int)((double)rgb.r/d);
	div.g = (int)((double)rgb.g/d);
	div.b = (int)((double)rgb.b/d);
	
	return div;
}

double distRGB(RGB from, RGB to) {
	RGB dif = difRGB(from, to);
	double dist = dif.r*dif.r + dif.g*dif.g + dif.b*dif.b;
	
	return dist;
}

RGB nearestRGB(RGB rgb, int numRGBs, u_int8_t* dst_ptr) {
	double dist = -1, tempDist;
	RGB nearest;
    RGB rgb_pal;
	
	int i;
	u_int8_t index;
	for (i = 0; i < numRGBs; i++) {
		rgb_pal.r = ( ( ((palette_ori[i] >> 7) & 0x0E ) | ((palette_ori[i] >> 11) & 0x01 ) ) << 4 );
        rgb_pal.g = (( ((palette_ori[i] >> 3) & 0x0E) | ((palette_ori[i] >> 7) & 0x01 ) ) << 4 );
        rgb_pal.b = (( ((palette_ori[i]) & 0x07 ) << 1 | ((palette_ori[i] >> 3) & 0x01 ) ) << 4 );
		tempDist = distRGB(rgb, rgb_pal);
		
		if (tempDist < dist || dist < 0) {
			dist = tempDist;
			nearest = rgb_pal;
			index = i;
		}
	}
	if(dst_ptr){
		*dst_ptr = index;
	}
	return nearest;
}

void render_4bit_floydstein(MFDB* MFDB32, MFDB* MFDB8, int numCols) {

	int i, x, y;
	RGB rgb, nearest, rgberror;
    int imgw = MFDB_STRIDE(MFDB32->fd_w);
    int imgh = MFDB32->fd_h;
    u_int32_t *pixels = (u_int32_t*)MFDB32->fd_addr;
	u_int8_t* dst_ptr;
	if(MFDB8){
		dst_ptr = (u_int8_t*)MFDB8->fd_addr;
	}
	for (i = 0; i < imgw * imgh; i++) {
		rgb = getRGB(pixels, i%imgw, i/imgw, imgw, imgh); 
		nearest = nearestRGB(rgb, numCols, dst_ptr++);
		
		rgberror = difRGB(nearest, rgb);
		rgberror = divRGB(rgberror, 16);
		
		x = i%imgw; y = i/imgw;
		
		setRGB(pixels, x+1, y, addRGB(getRGB(pixels, x+1, y, imgw, imgh), mulRGB(rgberror, 7)), imgw, imgh);
		setRGB(pixels, x-1, y+1, addRGB(getRGB(pixels, x-1, y+1, imgw, imgh), mulRGB(rgberror, 3)), imgw, imgh);
		setRGB(pixels, x, y+1, addRGB(getRGB(pixels, x, y+1, imgw, imgh), mulRGB(rgberror, 5)), imgw, imgh);
		setRGB(pixels, x+1, y+1, addRGB(getRGB(pixels, x+1, y+1, imgw, imgh), rgberror), imgw, imgh);
		
		setRGB(pixels, i%imgw, i/imgw, nearest, imgw, imgh);
	}

}
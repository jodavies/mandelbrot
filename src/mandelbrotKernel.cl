// OpenCL Kernel to render the mandelbrot set

#include "config.h"


__kernel void renderMandelbrotKernel(__global float * restrict pixels, const int xRes, const int yRes,
                                     const double xMin, const double xMax, const double yMin, const double yMax,
                                     const int maxIters)
{
	const int x = get_global_id(0)%xRes;
	const int y = get_global_id(0)/xRes;

	int iter = 0;

	double u = 0.0, v = 0.0, uNew, vNew;
	double uSq = 0.0, vSq = 0.0;
	const double xPix = ((double)x/(double)xRes);
	const double yPix = ((double)y/(double)yRes);
	const double Rec = (1.0-xPix)*xMin + xPix*xMax;
	const double Imc = (1.0-yPix)*yMin + yPix*yMax;

#ifdef EARLYBAIL
	// early bail-out if point is inside cardioid or period 2 bulb
	const double RecSq = Rec*Rec;
	const double ImcSq = Imc*Imc;
	const double q = (RecSq - 0.5*Rec + 0.125) + ImcSq;
	if ((q*(q+(Rec-0.25)) < (ImcSq*0.25)) || ((RecSq + 2.0*Rec + 1.0) + ImcSq < 1.0/16.0)) {
		iter = maxIters;
	}
#endif

	while ( (uSq+vSq) <= 4.0 && iter < maxIters) {
		uNew = uSq-vSq + Rec;
		uSq = uNew*uNew;
		vNew = 2.0*u*v + Imc;
		vSq = vNew*vNew;
		u = uNew;
		v = vNew;
		iter++;
	}

	float r,g,b;

	if (iter == maxIters) {
		r = 0.0;
		g = 0.0;
		b = 0.0;
	}

	else {
		float smooth = fmod((iter -log(log(uSq+vSq)/log(2.0f))),COLOURPERIOD)/COLOURPERIOD;

		if (smooth < 0.25) {
			r = 0.0;
			g = 0.5*smooth*4.0;
			b = 1.0*smooth*4.0;
		}
		else if (smooth < 0.5) {
			r = 1.0*(smooth-0.25)*4.0;
			g = 0.5 + 0.5*(smooth-0.25)*4.0;
			b = 1.0;
		}
		else if (smooth < 0.75) {
			r = 1.0;
			g = 1.0 - 0.5*(smooth-0.5)*4.0;
			b = 1.0 - (smooth-0.5)*4.0;
		}
		else {
			r = (1.0-(smooth-0.75)*4.0);
			g = 0.5*(1.0-(smooth-0.75)*4.0);
			b = 0.0;
		}
	}

	pixels[y*xRes*3 + x*3 + 0] = r;
	pixels[y*xRes*3 + x*3 + 1] = g;
	pixels[y*xRes*3 + x*3 + 2] = b;
}



__kernel void gaussianBlurKernel(__write_only image2d_t image, const int xRes, const int yRes,
                                 __global const float * restrict pixels)
{
	const int x = get_global_id(0)%xRes;
	const int y = get_global_id(0)/xRes;
	float r,g,b;

#ifndef GAUSSIANBLUR
	r = pixels[y*xRes*3 + x*3 + 0];
	g = pixels[y*xRes*3 + x*3 + 1];
	b = pixels[y*xRes*3 + x*3 + 2];

#else
	const int yu = (y == yRes-1) ? y : y+1;
	const int yd = (y == 0) ? y : y-1;
	const int xl = (x == 0) ? x : x-1;
	const int xr = (x == xRes-1) ? x : x+1;

	r = (+1.0*pixels[yu*xRes*3 + x *3 + 0]
		  +1.0*pixels[y *xRes*3 + xr*3 + 0]
		  +4.0*pixels[y *xRes*3 + x *3 + 0]
		  +1.0*pixels[y *xRes*3 + xl*3 + 0]
		  +1.0*pixels[yd*xRes*3 + x *3 + 0])/8.0;
	g = (+1.0*pixels[yu*xRes*3 + x *3 + 1]
		  +1.0*pixels[y *xRes*3 + xr*3 + 1]
		  +4.0*pixels[y *xRes*3 + x *3 + 1]
		  +1.0*pixels[y *xRes*3 + xl*3 + 1]
		  +1.0*pixels[yd*xRes*3 + x *3 + 1])/8.0;
	b = (+1.0*pixels[yu*xRes*3 + x *3 + 2]
		  +1.0*pixels[y *xRes*3 + xr*3 + 2]
		  +4.0*pixels[y *xRes*3 + x *3 + 2]
		  +1.0*pixels[y *xRes*3 + xl*3 + 2]
		  +1.0*pixels[yd*xRes*3 + x *3 + 2])/8.0;
#endif

	int2 coord = {x,y};
	float4 colour = {r,g,b,1.0};
	write_imagef(image, coord, colour);
}



__kernel void gaussianBlurKernel2(__global float * restrict output, const int xRes, const int yRes,
                                  __global const float * restrict pixels)
{
	const int x = get_global_id(0)%xRes;
	const int y = get_global_id(0)/xRes;
	float r,g,b;

#ifndef GAUSSIANBLUR
	r = pixels[y*xRes*3 + x*3 + 0];
	g = pixels[y*xRes*3 + x*3 + 1];
	b = pixels[y*xRes*3 + x*3 + 2];

#else
	const int yu = (y == yRes-1) ? y : y+1;
	const int yd = (y == 0) ? y : y-1;
	const int xl = (x == 0) ? x : x-1;
	const int xr = (x == xRes-1) ? x : x+1;

	r = (+1.0*pixels[yu*xRes*3 + x *3 + 0]
		  +1.0*pixels[y *xRes*3 + xr*3 + 0]
		  +4.0*pixels[y *xRes*3 + x *3 + 0]
		  +1.0*pixels[y *xRes*3 + xl*3 + 0]
		  +1.0*pixels[yd*xRes*3 + x *3 + 0])/8.0;
	g = (+1.0*pixels[yu*xRes*3 + x *3 + 1]
		  +1.0*pixels[y *xRes*3 + xr*3 + 1]
		  +4.0*pixels[y *xRes*3 + x *3 + 1]
		  +1.0*pixels[y *xRes*3 + xl*3 + 1]
		  +1.0*pixels[yd*xRes*3 + x *3 + 1])/8.0;
	b = (+1.0*pixels[yu*xRes*3 + x *3 + 2]
		  +1.0*pixels[y *xRes*3 + xr*3 + 2]
		  +4.0*pixels[y *xRes*3 + x *3 + 2]
		  +1.0*pixels[y *xRes*3 + xl*3 + 2]
		  +1.0*pixels[yd*xRes*3 + x *3 + 2])/8.0;
#endif

	output[y*xRes*3 + x*3 + 0] = r;
	output[y*xRes*3 + x*3 + 1] = g;
	output[y*xRes*3 + x*3 + 2] = b;
}

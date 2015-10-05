// OpenCL Kernel to render the mandelbrot set


#define NCOLOURS 128


__kernel void renderMandelbrotKernel(__write_only image2d_t image, const int xRes, const int yRes,
                                     const double xMin, const double xMax, const double yMin, const double yMax,
                                     const int maxIters)
{
	const int x = get_global_id(0)%xRes;
	const int y = get_global_id(0)/xRes;

	int iter = 0;

	double u = 0.0, v = 0.0, uNew, vNew;
	const double xPix = ((double)x/(double)xRes);
	const double yPix = ((double)y/(double)yRes);
	const double Rec = (1.0-xPix)*xMin + xPix*xMax;
	const double Imc = (1.0-yPix)*yMin + yPix*yMax;

	double uSq = u*u;
	double vSq = v*v;
	while ( (uSq+vSq) <= 4.0 && iter < maxIters) {
		uNew = uSq-vSq + Rec;
		uSq = uNew*uNew;
		vNew = 2.0*u*v + Imc;
		vSq = vNew*vNew;
		u = uNew;
		v = vNew;
		iter++;
	}

	int2 coord = {x,y};
	float r,g,b;

	if (iter == maxIters) {
		r = 0.0;
		g = 0.0;
		b = 0.0;
	}

	else {
		float smooth = fmod((iter -log(log(uSq+vSq)/log(2.0f))),NCOLOURS)/NCOLOURS;

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

	float4 colour = {r,g,b,1.0};
	write_imagef(image, coord, colour);
}

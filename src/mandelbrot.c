#include "mandelbrot.h"


void RenderMandelbrotCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();

	if (xMin == ((1.0-(1.0/(double)xRes))*xMin + (1.0-(1.0/(double)xRes))*xMax)
	 || yMin == ((1.0-(1.0/(double)yRes))*yMin + (1.0-(1.0/(double)xRes))*yMax)) {
		printf("PRECISION WARNING!\n");
	}

	// For each pixel, iterate and store the iteration number when |z|>2 or maxIters
	#pragma omp parallel for default(none) shared(image) schedule(dynamic)
	for (int y = 0; y < yRes; y++) {
		for (int x = 0; x < xRes; x++) {

			int iter = 0;
			double u = 0.0, v = 0.0, uNew;
			const double xPix = ((double)x/(double)xRes);
			const double yPix = ((double)y/(double)yRes);
			const double Rec = (1.0-xPix)*xMin + xPix*xMax;
			const double Imc = (1.0-yPix)*yMin + yPix*yMax;


			// early stop if point is inside cardioid
			const double q = (Rec - 0.25)*(Rec - 0.25) + Imc*Imc;
			if (q*(q+(Rec-0.25)) < (Imc*Imc*0.25)) {
				iter = maxIters;
			}


			// mandelbrot iterations
			double uSq = u*u;
			double vSq = v*v;
			while ( (uSq+vSq) <= 4.0 && iter < maxIters) {
				uNew = uSq-vSq + Rec;
				v = 2*u*v + Imc;
				u = uNew;
				uSq = u*u;
				vSq = v*v;
				iter++;
			}


			// colouring
			if (iter == maxIters ) {
				// set pixel to be black
				image[y*xRes*3 + x*3 + 0] = 0.0;
				image[y*xRes*3 + x*3 + 1] = 0.0;
				image[y*xRes*3 + x*3 + 2] = 0.0;
			}
			else {
				// set pixel colour
				float smooth = fmod((iter -log(log(u*u+v*v)/log(2.0f))),128.0)/128.0;
				if (smooth < 0.25) {
					image[y*xRes*3 + x*3 + 0] = 0.0;
					image[y*xRes*3 + x*3 + 1] = 0.5*smooth*4.0;
					image[y*xRes*3 + x*3 + 2] = 1.0*smooth*4.0;
				}
				else if (smooth < 0.5) {
					image[y*xRes*3 + x*3 + 0] = 1.0*(smooth-0.25)*4.0;
					image[y*xRes*3 + x*3 + 1] = 0.5 + 0.5*(smooth-0.25)*4.0;
					image[y*xRes*3 + x*3 + 2] = 1.0;
				}
				else if (smooth < 0.75) {
					image[y*xRes*3 + x*3 + 0] = 1.0;
					image[y*xRes*3 + x*3 + 1] = 1.0 - 0.5*(smooth-0.5)*4.0;
					image[y*xRes*3 + x*3 + 2] = 1.0 - (smooth-0.5)*4.0;
				}
				else {
					image[y*xRes*3 + x*3 + 0] = (1.0-(smooth-0.75)*4.0);
					image[y*xRes*3 + x*3 + 1] = 0.5*(1.0-(smooth-0.75)*4.0);
					image[y*xRes*3 + x*3 + 2] = 0.0;
				}
			}

		}
	}
#ifdef GAUSSIANBLUR
	GaussianBlur(image, xRes, yRes);
#endif
}



void RenderMandelbrotGMPCPU(float *image, const int xResIn, const int yResIn,
                      const double xMinIn, const double xMaxIn, const double yMinIn, const double yMaxIn,
                      const int maxIters)
{
	double startTime = GetWallTime();


	// 256 bit floats
	mpf_set_default_prec(256);

	// For each pixel, iterate and store the iteration number when |z|>2 or maxIters
	#pragma omp parallel for default(none) shared(image) schedule(dynamic)
	for (int y = 0; y < yResIn; y++) {
		for (int x = 0; x < xResIn; x++) {

			int iter = 0;
			mpf_t u, v, uNew, uSq, vSq, mag, two, four;
			mpf_init(u);
			mpf_init(v);
			mpf_init(uNew);
			mpf_init(uSq);
			mpf_init(vSq);
			mpf_init(mag);
			mpf_init_set_ui(two, (unsigned long)2);
			mpf_init_set_ui(four, (unsigned long)4);

			mpf_t xPix, yPix, xRes, yRes, Rec, Imc;
			mpf_init(xPix);
			mpf_init(yPix);
			mpf_init_set_si(xRes, xResIn);
			mpf_init_set_si(yRes, yResIn);
			mpf_init(Rec);
			mpf_init(Imc);

			mpf_t xMin, xMax, yMin, yMax;
			mpf_init_set_d(xMin, xMinIn);
			mpf_init_set_d(xMax, xMaxIn);
			mpf_init_set_d(yMin, yMinIn);
			mpf_init_set_d(yMax, yMaxIn);

			mpf_t xtmp1,xtmp2;
			mpf_t ytmp1,ytmp2;
			mpf_init(xtmp1);
			mpf_init(xtmp2);
			mpf_init(ytmp1);
			mpf_init(ytmp2);

			mpf_ui_div(xPix, (unsigned long)x, xRes);
			mpf_ui_div(yPix, (unsigned long)y, yRes);

			mpf_ui_sub(xtmp1, 1, xPix); // xtmp1 = 1-xPix
			mpf_ui_sub(ytmp1, 1, yPix); // ytmp1 = 1-yPix

			mpf_mul(xtmp2, xtmp1, xMin); // xtmp2 = (1-xPix)*xMin
			mpf_mul(ytmp2, ytmp1, yMin); // ytmp2 = (1-yPix)*yMin

			mpf_mul(xtmp1, xPix, xMax); // xtmp1 = xPix*xMax
			mpf_mul(ytmp1, yPix, yMax); // ytmp1 = yPix*yMax

			mpf_add(Rec, xtmp1, xtmp2);
			mpf_add(Imc, ytmp1, ytmp2);



			// Initial values of squares and magnitude
			mpf_mul(uSq, u, u);
			mpf_mul(vSq, v, v);
			mpf_add(mag, uSq, vSq);

			while ( mpf_cmp(mag, four) <= 0 && iter < maxIters) {
				mpf_sub(xtmp1, uSq, vSq);
				mpf_add(uNew, xtmp1, Rec); // uNew = uSq - vSq + Rec

				mpf_mul(ytmp1, two, u);
				mpf_mul(ytmp2, ytmp1, v);
				mpf_add(v, ytmp2, Imc); // v = 2*u*v + Imc

				mpf_set(u, uNew);

				// update squares and magnitude
				mpf_mul(uSq, u, u);
				mpf_mul(vSq, v, v);
				mpf_add(mag, uSq, vSq);

				iter++;
			}


			// colouring
			if (iter == maxIters ) {
				// set pixel to be black
				image[y*xResIn*3 + x*3 + 0] = 0.0;
				image[y*xResIn*3 + x*3 + 1] = 0.0;
				image[y*xResIn*3 + x*3 + 2] = 0.0;
			}
			else {
				// set pixel colour
				float smooth = fmod((iter -log(log(mpf_get_d(mag))/log(2.0f))),128.0)/128.0;
				if (smooth < 0.25) {
					image[y*xResIn*3 + x*3 + 0] = 0.0;
					image[y*xResIn*3 + x*3 + 1] = 0.5*smooth*4.0;
					image[y*xResIn*3 + x*3 + 2] = 1.0*smooth*4.0;
				}
				else if (smooth < 0.5) {
					image[y*xResIn*3 + x*3 + 0] = 1.0*(smooth-0.25)*4.0;
					image[y*xResIn*3 + x*3 + 1] = 0.5 + 0.5*(smooth-0.25)*4.0;
					image[y*xResIn*3 + x*3 + 2] = 1.0;
				}
				else if (smooth < 0.75) {
					image[y*xResIn*3 + x*3 + 0] = 1.0;
					image[y*xResIn*3 + x*3 + 1] = 1.0 - 0.5*(smooth-0.5)*4.0;
					image[y*xResIn*3 + x*3 + 2] = 1.0 - (smooth-0.5)*4.0;
				}
				else {
					image[y*xResIn*3 + x*3 + 0] = (1.0-(smooth-0.75)*4.0);
					image[y*xResIn*3 + x*3 + 1] = 0.5*(1.0-(smooth-0.75)*4.0);
					image[y*xResIn*3 + x*3 + 2] = 0.0;
				}
			}

			// Clear mpf variables to free memory
			mpf_clear(u);
			mpf_clear(v);
			mpf_clear(uNew);
			mpf_clear(uSq);
			mpf_clear(vSq);
			mpf_clear(mag);
			mpf_clear(two);
			mpf_clear(four);

			mpf_clear(xPix);
			mpf_clear(yPix);
			mpf_clear(xRes);
			mpf_clear(yRes);
			mpf_clear(Rec);
			mpf_clear(Imc);

			mpf_clear(xMin);
			mpf_clear(xMax);
			mpf_clear(yMin);
			mpf_clear(yMax);

			mpf_clear(xtmp1);
			mpf_clear(xtmp2);
			mpf_clear(ytmp1);
			mpf_clear(ytmp2);

		}
	}
#ifdef GAUSSIANBLUR
	GaussianBlur(image, xResIn, yResIn);
#endif
}


// Vectorized routine using AVX intrinsics. Vector variables have a "v" prefix.
void RenderMandelbrotAVXCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();

	if (xMin == ((1.0-(1.0/(double)xRes))*xMin + (1.0-(1.0/(double)xRes))*xMax)
	 || yMin == ((1.0-(1.0/(double)yRes))*yMin + (1.0-(1.0/(double)xRes))*yMax)) {
		printf("PRECISION WARNING!\n");
	}

	const __m256d vxMin = _mm256_set1_pd(xMin);
	const __m256d vxMax = _mm256_set1_pd(xMax);
	const __m256d vyMin = _mm256_set1_pd(yMin);
	const __m256d vyMax = _mm256_set1_pd(yMax);
	const __m256d vxRes = _mm256_set1_pd((double)xRes);
	const __m256d vyRes = _mm256_set1_pd((double)yRes);

	// For each pixel, iterate and store the iteration number when |z|>2 or maxIters
	#pragma omp parallel for default(none) shared(image) schedule(dynamic)
	for (int y = 0; y < yRes; y++) {
		for (int x = 0; x < xRes; x+=4) {

			const __m256d vxPix = _mm256_div_pd(_mm256_set_pd(x+3,x+2,x+1,x+0), vxRes);
			const __m256d vyPix = _mm256_div_pd(_mm256_set1_pd(y), vyRes);

			const __m256d vRec = _mm256_add_pd(_mm256_mul_pd(_mm256_sub_pd(_mm256_set1_pd(1.0), vxPix),vxMin),
			                                   _mm256_mul_pd(vxPix, vxMax));
			const __m256d vImc = _mm256_add_pd(_mm256_mul_pd(_mm256_sub_pd(_mm256_set1_pd(1.0), vyPix),vyMin),
			                                   _mm256_mul_pd(vyPix, vyMax));


/*			// early stop if ALL FOUR points are inside cardioid ??????????? implement this.
			const double q = (Rec - 0.25)*(Rec - 0.25) + Imc*Imc;
			if (q*(q+(Rec-0.25)) < (Imc*Imc*0.25)) {
				iter = maxIters;
			}*/


			int iter = 0;
			__m256d viter = _mm256_set1_pd(0.0);
			__m256d vu = _mm256_set1_pd(0.0);
			__m256d vv = _mm256_set1_pd(0.0);
			__m256d vuFinal = _mm256_set1_pd(0.0);
			__m256d vvFinal = _mm256_set1_pd(0.0);

			// Update mask. Update viter depending on this, and also update vuFinal and vvFinal.
			// Initial value is -1.0 since we update based on the *sign bit* of this vector.
			__m256d vincrMask = _mm256_set1_pd(-1.0);

			// initial squares and u*v are zero.
			__m256d vuSq = _mm256_set1_pd(0.0);
			__m256d vvSq = _mm256_set1_pd(0.0);
			__m256d vvuvv = _mm256_set1_pd(0.0);

			// Loop on non-vector iteration count. If this hits maxIters, all four entries have diverged.
			while (iter++ < maxIters) {

				vu = _mm256_add_pd(_mm256_sub_pd(vuSq, vvSq), vRec);
				vv = _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(2.0),vvuvv), vImc);

				// compute new squares and product of u and v
				vuSq = _mm256_mul_pd(vu,vu);
				vvSq = _mm256_mul_pd(vv,vv);
				vvuvv = _mm256_mul_pd(vu, vv);

				// compute magnitude, for divergence check
				__m256d vmagnitude = _mm256_add_pd(vuSq, vvSq);

				// If pixel has not *already* diverged, update v{u,v}Final values for smooth colouring -- this was
				// very easy in the scalar code as we just left the loop upon divergence with u,v retaining their
				// values. Now we continue until all 4 values have diverged, so only store un-diverged pixels,
				// or pixels which diverged *this* iteration.
				// _mm256_maskstore_pd stores if the *sign bit* in the __m256i is 1, (which it is, for (-nan))
				// so all we have to do is bit-wise cast vincrMask to __m256i.
				_mm256_maskstore_pd( (double*)&vuFinal, _mm256_castpd_si256(vincrMask), vu);
				_mm256_maskstore_pd( (double*)&vvFinal, _mm256_castpd_si256(vincrMask), vv);

				// Now we need to increment iteration count for vector entries which have magnitude <= 4.0, ie,
				// the didn't diverge this iteration.
				vincrMask = _mm256_cmp_pd(vmagnitude, _mm256_set1_pd(4.0), _CMP_LE_OS);

				// vincr now contains 0xFFFFFFFFFFFFFFFF (-nan) where vmagnitude entry <= 4.0, and 0x0 otherwise.
				// By computing a bitwise AND with 1.0, we get 1.0 where we want to increment the iteration count.
				viter = _mm256_add_pd(viter, _mm256_and_pd(vincrMask, _mm256_set1_pd(1.0)));

				// Want to leave loop if all 4 pixels have diverged, ie, test if all entries of incr are 0x0.
				// _mm256_testz_pd does this by computing bitwise AND of the operands, and returning 1 if all
				// resulting *sign bits* are zero.
				// (-nan) has a sign bit 1, so by using testz against a negative number, (-1.0, for eg) we get
				// a sign bit of 1, and don't break out of the loop.
				if ( _mm256_testz_pd(vincrMask, _mm256_set1_pd(-1.0))) {
					break;
				}

			}


			// colouring
			for (int k = 0; k < 4; k++) {
				if ( (int)(((double*)&viter)[k]) == maxIters ) {
					// set pixel to be black
					image[y*xRes*3 + (x+k)*3 + 0] = 0.0;
					image[y*xRes*3 + (x+k)*3 + 1] = 0.0;
					image[y*xRes*3 + (x+k)*3 + 2] = 0.0;
				}
				else {
					// set pixel colour
					double u = ((double*)&vuFinal)[k];
					double v = ((double*)&vvFinal)[k];
					float smooth = fmod( ((double*)&viter)[k] -log(log(u*u+v*v)/log(2.0f)), 128.0)/128.0;
					if (smooth < 0.25) {
						image[y*xRes*3 + (x+k)*3 + 0] = 0.0;
						image[y*xRes*3 + (x+k)*3 + 1] = 0.5*smooth*4.0;
						image[y*xRes*3 + (x+k)*3 + 2] = 1.0*smooth*4.0;
					}
					else if (smooth < 0.5) {
						image[y*xRes*3 + (x+k)*3 + 0] = 1.0*(smooth-0.25)*4.0;
						image[y*xRes*3 + (x+k)*3 + 1] = 0.5 + 0.5*(smooth-0.25)*4.0;
						image[y*xRes*3 + (x+k)*3 + 2] = 1.0;
					}
					else if (smooth < 0.75) {
						image[y*xRes*3 + (x+k)*3 + 0] = 1.0;
						image[y*xRes*3 + (x+k)*3 + 1] = 1.0 - 0.5*(smooth-0.5)*4.0;
						image[y*xRes*3 + (x+k)*3 + 2] = 1.0 - (smooth-0.5)*4.0;
					}
					else {
						image[y*xRes*3 + (x+k)*3 + 0] = (1.0-(smooth-0.75)*4.0);
						image[y*xRes*3 + (x+k)*3 + 1] = 0.5*(1.0-(smooth-0.75)*4.0);
						image[y*xRes*3 + (x+k)*3 + 2] = 0.0;
					}
				}
			}

		}
	}
#ifdef GAUSSIANBLUR
	GaussianBlur(image, xRes, yRes);
#endif
}

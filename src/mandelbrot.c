#include "mandelbrot.h"


void SetPixelColour(const int iter, const int maxIters, float mag, float *r, float *g, float *b)
{
	// If pixel hit max iteration count, make it black
	if (iter == maxIters) {
		*r = 0.0;
		*g = 0.0;
		*b = 0.0;
		return;
	}
	// For non-black pixels, define a smoothed iteration count using the final
	// magnitude (passed in as argument).
	// This is > 2, unless the pixel hit the max iteration count, handled above.
	// We subtract a very slowely growing function of the magnitude, so very
	// quickly diverging pixels look as if they diverged slightly (1, 2 iterations?)
	// earlier than their iteration count suggests.
	// Then compute its mod with a number of colour steps, so that it cycles
	// through the gradient repeatedly, and scale it between 0 and 1.
	float smooth = fmod((iter -log(log(mag)/log(2.0f))),NCOLOURS)/NCOLOURS;

	if (smooth < 0.25) {
		*r = 0.0;
		*g = 0.5*smooth*4.0;
		*b = 1.0*smooth*4.0;
	}
	else if (smooth < 0.5) {
		*r = 1.0*(smooth-0.25)*4.0;
		*g = 0.5 + 0.5*(smooth-0.25)*4.0;
		*b = 1.0;
	}
	else if (smooth < 0.75) {
		*r = 1.0;
		*g = 1.0 - 0.5*(smooth-0.5)*4.0;
		*b = 1.0 - (smooth-0.5)*4.0;
	}
	else {
		*r = (1.0-(smooth-0.75)*4.0);
		*g = 0.5*(1.0-(smooth-0.75)*4.0);
		*b = 0.0;
	}

}



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
			double u = 0.0, v = 0.0, uNew, vNew;
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
				uSq = uNew*uNew;
				vNew = 2.0*u*v + Imc;
				vSq = vNew*vNew;
				u = uNew;
				v = vNew;
				iter++;
			}

			SetPixelColour(iter, maxIters, uSq+vSq,
			               &(image[y*xRes*3+x*3+0]),&(image[y*xRes*3+x*3+1]),&(image[y*xRes*3+x*3+2]));

		}
	}
#ifdef GAUSSIANBLUR
	GaussianBlur(image, xRes, yRes);
#endif
}



// Routine using GMP library for high precision. High precision variables have prefix "m".
void RenderMandelbrotGMPCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();


	// 256 bit floats
	mpf_set_default_prec(256);

	// x,y loop invariant:
	mpf_t mtwo, mfour;
	mpf_init_set_ui(mtwo, (unsigned long)2);
	mpf_init_set_ui(mfour, (unsigned long)4);
	mpf_t mxMin, mxMax, myMin, myMax;
	mpf_init_set_d(mxMin, xMin);
	mpf_init_set_d(mxMax, xMax);
	mpf_init_set_d(myMin, yMin);
	mpf_init_set_d(myMax, yMax);
	mpf_t mxRes, myRes;
	mpf_init_set_si(mxRes, xRes);
	mpf_init_set_si(myRes, yRes);


	// For each pixel, iterate and store the iteration number when |z|>2 or maxIters
	#pragma omp parallel for default(none) shared(image,mtwo,mfour,mxMin,mxMax,myMin,myMax,mxRes,myRes) schedule(dynamic)
	for (int y = 0; y < yRes; y++) {

		// x loop invariant
		mpf_t myPix;
		mpf_init(myPix);
		mpf_ui_div(myPix, (unsigned long)y, myRes);

		// init x-dependent mpf_t here, set inside loop
		mpf_t mu, mv, muNew, muSq, mvSq, mmag;
		mpf_init(mu);
		mpf_init(mv);
		mpf_init(muNew);
		mpf_init(muSq);
		mpf_init(mvSq);
		mpf_init(mmag);
		mpf_t mxPix, mRec, mImc;
		mpf_init(mxPix);
		mpf_init(mRec);
		mpf_init(mImc);
		mpf_t mxtmp1,mxtmp2;
		mpf_t mytmp1,mytmp2;
		mpf_init(mxtmp1);
		mpf_init(mxtmp2);
		mpf_init(mytmp1);
		mpf_init(mytmp2);


		for (int x = 0; x < xRes; x++) {

			int iter = 0;

			// set mu and mv to zero initially
			unsigned long zero = 0;
			mpf_set_ui(mu, zero);
			mpf_set_ui(mv, zero);

			mpf_ui_div(mxPix, (unsigned long)x, mxRes);

			mpf_ui_sub(mxtmp1, 1, mxPix); // xtmp1 = 1-xPix
			mpf_ui_sub(mytmp1, 1, myPix); // ytmp1 = 1-yPix

			mpf_mul(mxtmp2, mxtmp1, mxMin); // xtmp2 = (1-xPix)*xMin
			mpf_mul(mytmp2, mytmp1, myMin); // ytmp2 = (1-yPix)*yMin

			mpf_mul(mxtmp1, mxPix, mxMax); // xtmp1 = xPix*xMax
			mpf_mul(mytmp1, myPix, myMax); // ytmp1 = yPix*yMax

			mpf_add(mRec, mxtmp1, mxtmp2);
			mpf_add(mImc, mytmp1, mytmp2); // all tmp vars available



			// Initial values of squares and magnitude
			mpf_mul(muSq, mu, mu);
			mpf_mul(mvSq, mv, mv);
			mpf_add(mmag, muSq, mvSq);

			while ( mpf_cmp(mmag, mfour) <= 0 && iter < maxIters) {
				mpf_sub(mxtmp1, muSq, mvSq);
				mpf_add(muNew, mxtmp1, mRec); // uNew = uSq - vSq + Rec

				mpf_mul(mytmp1, mtwo, mu);
				mpf_mul(mytmp2, mytmp1, mv);
				mpf_add(mv, mytmp2, mImc); // v = 2*u*v + Imc

				mpf_set(mu, muNew);

				// update squares and magnitude
				mpf_mul(muSq, mu, mu);
				mpf_mul(mvSq, mv, mv);
				mpf_add(mmag, muSq, mvSq);

				iter++;
			}

			SetPixelColour(iter, maxIters, (float)mpf_get_d(mmag),
			               &(image[y*xRes*3+x*3+0]),&(image[y*xRes*3+x*3+1]),&(image[y*xRes*3+x*3+2]));


		}

		// Clear mpf variables to free memory
		// x loop invariant
		mpf_clear(myPix);

		// x-dependent, init, clear outside loop
		mpf_clear(mu);
		mpf_clear(mv);
		mpf_clear(muNew);
		mpf_clear(muSq);
		mpf_clear(mvSq);
		mpf_clear(mmag);
		mpf_clear(mxPix);
		mpf_clear(mRec);
		mpf_clear(mImc);
		mpf_clear(mxtmp1);
		mpf_clear(mxtmp2);
		mpf_clear(mytmp1);
		mpf_clear(mytmp2);

	}

	// x,y loop invariant
	mpf_clear(mtwo);
	mpf_clear(mfour);
	mpf_clear(mxMin);
	mpf_clear(mxMax);
	mpf_clear(myMin);
	mpf_clear(myMax);
	mpf_clear(mxRes);
	mpf_clear(myRes);


#ifdef GAUSSIANBLUR
	GaussianBlur(image, xRes, yRes);
#endif
}


// Vectorized routine using AVX intrinsics. Vector variables have a "v" prefix.
void RenderMandelbrotAVXCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();

	if (xMin == ((1.0-(1.0/(double)xRes))*xMin + (1.0-(1.0/(double)xRes))*xMax)
	 || yMin == ((1.0-(1.0/(double)yRes))*yMin + (1.0-(1.0/(double)yRes))*yMax)) {
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
			// Compute final magnitude for smooth colouring function
			__m256d vmagnitudeFinal = _mm256_add_pd(_mm256_mul_pd(vuFinal,vuFinal), _mm256_mul_pd(vvFinal,vvFinal));

			for (int k = 0; k < 4; k++) {
				SetPixelColour((int)(((double*)&viter)[k]), maxIters, ((double*)&vmagnitudeFinal)[k],
							&(image[y*xRes*3+(x+k)*3+0]),&(image[y*xRes*3+(x+k)*3+1]),&(image[y*xRes*3+(x+k)*3+2]));
			}

		}
	}
#ifdef GAUSSIANBLUR
	GaussianBlur(image, xRes, yRes);
#endif
}



void RenderMandelbrotOpenCL(const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters,
                      cl_command_queue *queue, cl_kernel *renderMandelbrotKernel, cl_mem *pixelsImage,
                      size_t *globalSize, size_t *localSize)
{
	int err;
	// Update kernel args
	err  = clSetKernelArg(*renderMandelbrotKernel, 0, sizeof(cl_mem), pixelsImage);
	err |= clSetKernelArg(*renderMandelbrotKernel, 1, sizeof(int), &xRes);
	err |= clSetKernelArg(*renderMandelbrotKernel, 2, sizeof(int), &yRes);
	err |= clSetKernelArg(*renderMandelbrotKernel, 3, sizeof(double), &xMin);
	err |= clSetKernelArg(*renderMandelbrotKernel, 4, sizeof(double), &xMax);
	err |= clSetKernelArg(*renderMandelbrotKernel, 5, sizeof(double), &yMin);
	err |= clSetKernelArg(*renderMandelbrotKernel, 6, sizeof(double), &yMax);
	err |= clSetKernelArg(*renderMandelbrotKernel, 7, sizeof(int), &maxIters);
//	CheckOpenCLError(err, __LINE__);

	glFinish();
	err = clEnqueueAcquireGLObjects(*queue, 1, pixelsImage, 0, 0, NULL);
//	CheckOpenCLError(err, __LINE__);
	err = clEnqueueNDRangeKernel(*queue, *renderMandelbrotKernel, 1, NULL, globalSize, localSize, 0, NULL, NULL);
//	CheckOpenCLError(err, __LINE__);
//	clFinish(*queue);
	err = clEnqueueReleaseGLObjects(*queue, 1, pixelsImage, 0, 0, NULL);
	clFinish(*queue);
}

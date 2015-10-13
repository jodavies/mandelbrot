// Mandelbrot Rendering routines. All have the same signature, so we can easily switch routine using
// function pointers.

// Compute mandelbrot set inside supplied coordinates, returned in *image. Resolution variables used to
// determine mandelbrot coordinates of each pixel.


// Includes
#include <stdio.h>
#include <math.h>

#ifdef WITHGMP
	#include <gmp.h>
#endif

#ifdef WITHAVX
	#include <immintrin.h>
#endif

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef WITHOPENCL
	#include <CL/opencl.h>
	#include <CL/cl_gl.h>
	#include "CheckOpenCLError.h"
#endif

#include "structs.h"
#include "GaussianBlur.h"
#include "config.h"
#include "GetWallTime.h"


// Set pixels in r,g,b pointers based on final iteration value
void SetPixelColour(const int iter, const int maxIters, float mag, float *r, float *g, float *b, const double colourPeriod);


// Basic routine, using CPU.
void RenderMandelbrotCPU(renderStruct *render, imageStruct *image);

#ifdef WITHGMP
// High precision routine using GMP
void RenderMandelbrotGMPCPU(renderStruct *render, imageStruct *image);
#endif

#ifdef WITHAVX
// AVX Vectorized
void RenderMandelbrotAVXCPU(renderStruct *render, imageStruct *image);
#endif

#ifdef WITHOPENCL
// OpenCL. Sets kernel arguments, acquires opengl texture, runs kernel, releases texture.
// This function blocks until OpenCL has finished with the texture, and OpenGL is free to
// use it.
void RenderMandelbrotOpenCL(renderStruct *render, imageStruct *image);
#endif

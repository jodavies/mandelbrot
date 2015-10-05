// Mandelbrot Rendering routines. All have the same signature, so we can easily switch routine using
// function pointers.

// Compute mandelbrot set inside supplied coordinates, returned in *image. Resolution variables used to
// determine mandelbrot coordinates of each pixel.

// Optional Gaussian blur after computation, can look "nicer"
#define GAUSSIANBLUR

// Number of colour steps
#define NCOLOURS 128


// Includes
#include <stdio.h>
#include <math.h>
#include <gmp.h>
#include <immintrin.h>

#define GLEW_STATIC
#include <GL/glew.h>

#ifdef WITHOPENCL
	#include <CL/opencl.h>
	#include <CL/cl_gl.h>
	#include "CheckOpenCLError.h"
#endif

#include "GaussianBlur.h"
#include "GetWallTime.h"


// Set pixels in r,g,b pointers based on final iteration value
void SetPixelColour(const int iter, const int maxIters, float mag, float *r, float *g, float *b);


// Basic routine, using CPU.
void RenderMandelbrotCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

// High precision routine using GMP
void RenderMandelbrotGMPCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

// AVX Vectorized
void RenderMandelbrotAVXCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

#ifdef WITHOPENCL
// OpenCL. Sets kernel arguments, acquires opengl texture, runs kernel, releases texture.
// This function blocks until OpenCL has finished with the texture, and OpenGL is free to
// use it.
void RenderMandelbrotOpenCL(const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters,
                      cl_command_queue *queue, cl_kernel *renderMandelbrotKernel, cl_mem *pixelsImage,
                      size_t *globalSize, size_t *localSize);
#endif

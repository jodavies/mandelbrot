// Mandelbrot Rendering routines. All have the same signature, so we can easily switch routine using
// function pointers.

// Compute mandelbrot set inside supplied coordinates, returned in *image. Resolution variables used to
// determine mandelbrot coordinates of each pixel.

// Optional Gaussian blur after computation, can look "nicer"
#define GAUSSIANBLUR


// Includes
#include <stdio.h>
#include <math.h>
#include <gmp.h>
#include <immintrin.h>

#include "GaussianBlur.h"
#include "GetWallTime.h"


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

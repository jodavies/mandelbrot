// Structs to hold variables/parameters. This way, we can include what would otherwise have to be
// function arguments conditionally in the structs (such as OpenCL variables/structs) yet retain
// the same signature for rendering functions etc.

// This struct holds image parameters/variables
typedef struct {
	int xRes;			// x axis (horiz.) resolution
	int yRes;			// y axis (vert.) resolution

	double xMin;		// lower and upper image boundaries,
	double xMax;		// in "fractal coordinates", ie, the
	double yMin;		// complex plane.
	double yMax;

	int maxIters;		// max iteration count before a pixel
							// is considered converged. Changes with zoom.

	float * pixels;	// array of r,g,b colour values in [0.0,1.0].
} imageStruct;

// This struct holds variables needed for rendering the image
typedef struct {
	GLFWwindow *window;

#ifdef WITHOPENCL
	cl_command_queue queue;
	cl_kernel renderMandelbrotKernel;
	cl_kernel gaussianBlurKernel;
	cl_mem pixelsDevice;
	cl_mem pixelsTex;
	size_t globalSize;
	size_t localSize;
#endif
} renderStruct;

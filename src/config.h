// Some compile-time settings that determine program behaviour

// Window resolution.
#define XRESOLUTION 1920
#define YRESOLUTION 1080
// Comment for windowed mode
//#define FULLSCREEN 1

// Resolution multiplier to use for high-resolution render, to save as bitmap
#define HIGHRESOLUTIONMULTIPLIER 20


// For smooth zoom in and out, the initial number of interpolated frames to render.
// Auto adjusted to maintain framerate
#define INITIALZOOMSTEPS 30
// Zoom factor per user zoom-in/out request
#define ZOOMFACTOR 2.0
// How much to increase/decrease the maxIters variable for zoom-in/out
#define ITERSFACTOR 1.2
// Interpolate linearly or according to a steep, shifted logistic function:
//#define INTERPFUNC(t) (t)
#define INTERPFUNC(t) (1.0/(1.0+exp(-10.0*(t - 0.5))))

// Number of pixels cursor has to move, with left button held down, to be considered
// a drag, and pan the image:
#define DRAGPIXELS 4

// Minimum value for max iteration count
#define MINITERS 60

// Initial value for gaussian blur after mandelbrot computation, can be toggled at runtime
#define DEFAULTGAUSSIANBLUR 1

// Number of colour steps
#define DEFAULTCOLOURPERIOD 128

// Test if point is inside cardioid or period-2 bulb, and if so, bail early
#define EARLYBAIL 1


// // GMP
// Number of bits to use for multiple precision floats
#define GMPPRECISION 256


// // OpenCL
// workgroup size:
#define OPENCLLOCALSIZE 64
// attempt opengl opencl interop?
#define TRYINTEROP 1

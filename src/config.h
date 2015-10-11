// Some compile-time settings that determine program behaviour

// Window resolution.
#define XRESOLUTION 1920
#define YRESOLUTION 1080
// Comment for windowed mode
//#define FULLSCREEN 1

// Resolution multiplier to use for high-resolution render, to save as bitmap
#define HIGHRESOLUTIONMULTIPLIER 7


// For smooth zoom in and out, the number of interpolated frames to render
#define ZOOMSTEPS 20
// Zoom factor per user zoom-in/out request
#define ZOOMFACTOR 2.0
// How much to increase/decrease the maxIters variable for zoom-in/out
#define ITERSFACTOR 1.2
// Interpolate linearly or according to a steep, shifted logistic function:
//#define INTERPFUNC(i) ((double)i/(double)ZOOMSTEPS)
#define INTERPFUNC(i) (1.0/(1.0+exp(-10.0*((double)i/(double)ZOOMSTEPS - 0.5))))

// Number of pixels cursor has to move, with left button held down, to be considered
// a drag, and pan the image:
#define DRAGPIXELS 4

// Minimum value for max iteration count
#define MINITERS 60

// Initial value for gaussian blur after mandelbrot computation, can be toggled at runtime
#define DEFAULTGAUSSIANBLUR 1

// Number of colour steps
#define COLOURPERIOD 128

// Test if point is inside cardioid or period-2 bulb, and if so, bail early
#define EARLYBAIL 1

// // OpenCL
// workgroup size:
#define OPENCLLOCALSIZE 64

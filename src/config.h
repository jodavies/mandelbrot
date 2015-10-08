// Some compile-time settings that determine program behaviour

// Window resolution
#define XRESOLUTION 1920
#define YRESOLUTION 1080
// Comment for windowed mode
//#define FULLSCREEN 1


// For smooth zoom in and out, the number of interpolated frames to render
#define ZOOMSTEPS 20
// Zoom factor per user zoom-in/out request
#define ZOOMFACTOR 2.0
// How much to increase/decrease the maxIters variable for zoom-in/out
#define ITERSFACTOR 1.2
// Interpolate linearly or according to a steep, shifted logistic function:
//#define INTERPFUNC(i) ((double)i/(double)ZOOMSTEPS)
#define INTERPFUNC(i) (1.0/(1.0+exp(-10.0*((double)i/(double)ZOOMSTEPS - 0.5))))


// Gaussian blur after computation, can look "nicer"
#define GAUSSIANBLUR 1

// Number of colour steps
#define COLOURPERIOD 128

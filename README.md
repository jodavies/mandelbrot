# mandelbrot

![mandelbrot](img/mandelbrot.jpg)

Interactive Mandelbrot renderer, using OpenGL + OpenCL interop.


Controls:

* Left/Right Click to zoom in/out, centreing on cursor position
* Left Click and Drag to pan
* r to reset view
* q,w to decrease, increase max iteration count
* g to toggle Gaussian Blur after computation
* b to run some benchmarks
* p to show a double-precision limited zoom
* h to save a high resolution image of the current view to current directory
* Esc to quit



Some performance numbers (fps):

Benchmark | CPU | CPU (AVX) | GMP | R9 280X GL+CL Interop | R9 280X CL | GTX960 CL
----------|----:|----------:|----:|----------------------:|-----------:|--------:
Whole fractal | 14.41 | 15.87 | - | 309.29 | 51.55 | 21.20
Early Bail-Out | 29.64 | 12.98 | - | 250.38 | 50.63 | 15.86
Spiral | 0.45 | 1.59 | - | 17.76 | 13.16 | 1.55
Highly Zoomed | 0.15 | 0.48 | - | 3.80 | 3.34 | 0.35

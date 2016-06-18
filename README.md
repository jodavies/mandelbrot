# mandelbrot

![mandelbrot](img/mandelbrot.jpg)

Interactive Mandelbrot renderer, using OpenGL + OpenCL interop.


Controls:

* Left/Right Click to zoom in/out, centring on cursor position
* Left Click and Drag to pan
* r to reset view
* q,w to decrease, increase max iteration count
* g to toggle Gaussian Blur after computation
* b to run some benchmarks
* p to show a double-precision limited zoom
* h to save a high resolution image of the current view to current directory
* Esc to quit



Some performance numbers (fps).

CPU:

Benchmark | i5-2500K | i5-2500K (AVX) | i7-5820K | i7-5820K (AVX) |
----------|---------:|---------------:|---------:|---------------:|
Whole fractal | 14.41 | 15.87 | 17.70 | 18.30
Early Bail-Out | 29.64 | - | 22.38 | -
Spiral | 0.45 | 1.59 | 1.27 | 3.44
Highly Zoomed | 0.15 | 0.48 | 0.45 | 1.09


GPU with and without OpenGL+OpenCL Interop (IO)

Benchmark | R9 280X GLCL IO | R9 280X CL | R7 260X GLCL IO | R7 260X CL | HD7730M GLCL IO | HD7730M | GTX960 CL
----------|----------------------:|-----------:|----------------------:|-----------:|----------------------:|--------:|---------:
Whole fractal | 309.29 | 51.55 | 50.30 | 32.60 | 35.79 | 18.58 | 21.20
Early Bail-Out | 250.38 | 50.63 | 38.00 | 26.44 | 13.47 | 9.90 | 15.86
Spiral | 17.76 | 13.16 | 2.75 | 2.62 | 0.72 | 0.69 | 1.55
Highly Zoomed | 3.80 | 3.34 | 0.58 | 0.58 | 0.15 | 0.15 | 0.35

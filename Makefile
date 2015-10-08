source = src/GaussianBlur.c src/GetWallTime.c src/main.c src/mandelbrot.c
openclsource = src/CheckOpenCLError.c

all:
	mkdir -p bin
	gcc -o bin/mandelbrot $(source) -std=gnu99 -O3 -fno-unsafe-math-optimizations -fopenmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g

gmp:
	mkdir -p bin
	gcc -o bin/mandelbrot -DWITHGMP $(source) -std=gnu99 -O3 -fno-unsafe-math-optimizations -fopenmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g

avx:
	mkdir -p bin
	gcc -o bin/mandelbrot -DWITHAVX $(source) -std=gnu99 -O3 -march=core-avx-i -fno-unsafe-math-optimizations -fopenmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g

opencl:
	mkdir -p bin
	gcc -o bin/mandelbrot -DWITHOPENCL $(source) $(openclsource) -std=gnu99 -O3 -march=core-avx-i -fno-unsafe-math-optimizations -fopenmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -lOpenCL -Wall -pedantic -g


clean:
	rm -r bin

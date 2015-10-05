source = src/GaussianBlur.c src/GetWallTime.c src/main.c src/mandelbrot.c
openclsource = src/CheckOpenCLError.c

all:
	mkdir -p bin
	gcc -o bin/mandelbrot $(source) -std=gnu99 -O3 -march=core-avx-i -fno-unsafe-math-optimizations -fopenmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g
#	icc -o bin/mandelbrot-icc $(source) -std=gnu99 -O3 -xAVX -fp-model=precise -openmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g

opencl:
	mkdir -p bin
	gcc -o bin/mandelbrot -DWITHOPENCL $(source) $(openclsource) -std=gnu99 -O3 -march=core-avx-i -fno-unsafe-math-optimizations -fopenmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -lOpenCL -Wall -pedantic -g
#	icc -o bin/mandelbrot-icc -DWITHOPENCL $(source) $(openclsource) -DOPENCL -std=gnu99 -O3 -xAVX -fp-model=precise -openmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -lOpenCL -Wall -pedantic -g

debug:
	mkdir -p bin
	gcc -o bin/mandelbrot-debug $(source) -std=gnu99 -march=core-avx-i -fno-unsafe-math-optimizations -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g
#	icc -o bin/mandelbrot-icc-debug $(source) -std=gnu99 -xAVX -fp-model=precise -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g


clean:
	rm -r bin

all:
	mkdir -p bin
	gcc -o bin/mandelbrot src/*.c -std=gnu99 -O3 -march=core-avx-i -ffast-math -fopenmp -lgmp -lrt -lglfw3 -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g
	icc -o bin/mandelbrot-icc src/*.c -std=gnu99 -O3 -xAVX -openmp -lgmp -lrt -lglfw3 -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g


clean:
	rm -r bin

all:
	mkdir -p bin
	gcc -o bin/mandelbrot src/mandelbrot.c -std=gnu99 -O3 -fopenmp -lrt -lglfw3 -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g


clean:
	rm -r bin

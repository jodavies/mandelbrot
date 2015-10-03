all:
	mkdir -p bin
	gcc -o bin/mandelbrot src/*.c -std=gnu99 -O3 -march=core-avx-i -fno-unsafe-math-optimizations -fopenmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g
#	icc -o bin/mandelbrot-icc src/*.c -std=gnu99 -O3 -xAVX -fp-model=precise -openmp -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g

debug:
	mkdir -p bin
	gcc -o bin/mandelbrot-debug src/*.c -std=gnu99 -march=core-avx-i -fno-unsafe-math-optimizations -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g
#	icc -o bin/mandelbrot-icc-debug src/*.c -std=gnu99 -xAVX -fp-model=precise -lgmp -lrt -lglfw -lGL -lm -lXrandr -lXi -lX11 -lXxf86vm -lpthread -lGLEW -Wall -pedantic -g


clean:
	rm -r bin

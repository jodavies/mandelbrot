source = src/GaussianBlur.c src/GetWallTime.c src/main.c src/mandelbrot.c
openclsource = src/CheckOpenCLError.c

CFLAGS += -std=c99 -pedantic -Wall
CFLAGS += -O3 -g
CFLAGS += -fno-unsafe-math-optimizations -fopenmp

CPPFLAGS += -D_POSIX_C_SOURCE=200112L

LDLIBS += -lrt -ldl -lm -lpthread

LDLIBS += -lXinerama -lXcursor -lXrandr -lXi -lX11 -lXxf86vm
LDLIBS += -lGL -lGLEW
LDLIBS += -lglfw

CPPFLAGS += -DWITHFREEIMAGE
LDLIBS += -lfreeimage

std: bin/mandelbrot
gmp: bin/mandelbrot-gmp
avx: bin/mandelbrot-avx
opencl: bin/mandelbrot-cl

bin/mandelbrot: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin/mandelbrot-gmp: LDLIBS += -lgmp
bin/mandelbrot-gmp: CPPFLAGS += -DWITHGMP
bin/mandelbrot-gmp: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin/mandelbrot-avx: CPPFLAGS += -DWITHAVX
bin/mandelbrot-avx: CFLAGS += -march=core-avx-i
bin/mandelbrot-avx: $(source) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin/mandelbrot-cl: CPPFLAGS += -DWITHOPENCL
bin/mandelbrot-cl: LDLIBS += -lOpenCL
bin/mandelbrot-cl: $(source) $(openclsource) | bin
	$(CC) -o $@ $^ $(CPPFLAGS) $(CFLAGS) $(LDLIBS)

bin:
	mkdir -p bin

clean:
	rm -rf bin

all: std gmp avx opencl

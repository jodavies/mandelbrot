#include <stdio.h>
#include <stdlib.h>
#include <time.h>		//gettimeofday()
#include <limits.h>
#include <math.h>
#include <unistd.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>


// Returns ns accurate walltime
double GetWallTime();

// Compute mandelbrot set inside supplied coordinates, returned in *image.
void RenderMandelbrotCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

// Adjust mandelbrot co-ordinates based on scale factor
void ApplyZoom(const int xRes, const int yRes, const double xCursor, const double yCursor,
               double * xMin, double * xMax, double * yMin, double * yMax, const double scaleFac);

// Test fps for a particular range/zoom, render at least 10 frames or run for 1 second
void RunBenchmark(GLFWwindow *window, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

// Set initial values for render ranges, number of iterations. This is a function as it is called
// in more than one place.
void SetInitialValues(double *xMin, double *xMax, double *yMin, double *yMax, int *maxIters,
                      const int xRes, const int yRes);

void WriteImageToFile(float *image, const int xRes, const int yRes);

int SetUpOpenGL(GLFWwindow **window, const int xRes, const int yRes,
                GLuint *vertexShader, GLuint *fragmentShader, GLuint *shaderProgram,
                GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *tex);




int main(void)
{
	// Define image size and min/max coords. x is the real axis and y the imaginary
	const int xRes = 1920;
	const int yRes = 1080;

	// boudary variables and max iteration count
	double xMin, xMax, yMin, yMax;
	int maxIters;
	SetInitialValues(&xMin, &xMax, &yMin, &yMax, &maxIters, xRes, yRes);
	printf("Initial Boundaries: %lf,%lf,  %lf,%lf\n", xMin,xMax, yMin,yMax);

	// Allocate array of floats which represent the pixels. 3 floats per pixel for RGB.
	float *image;
	image = malloc(xRes * yRes * sizeof *image *3);


	// OpenGL variables and setup
	GLFWwindow *window = NULL;
	GLuint vertexShader, fragmentShader, shaderProgram;
	GLuint vao, vbo, ebo, tex;
	SetUpOpenGL(&window, xRes, yRes, &vertexShader, &fragmentShader, &shaderProgram, &vao, &vbo, &ebo, &tex);


	// OpenCL variables and setup
	// TODO



	// Start main loop: Update until we encounter user input. Look for Esc key (quit), left and right mount
	// buttons (zoom in on cursor position, zoom out on cursor position), "r" -- reset back to initial coords etc.

	//Need an initial computation of mandelbrot, and texture update before we enter
	RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);

	while (!glfwWindowShouldClose(window)) {

		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap buffers
		glfwSwapBuffers(window);

		// poll for user input
		glfwPollEvents();

		// if user presses Esc, close window to leave loop
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		    glfwSetWindowShouldClose(window, GL_TRUE);
		}

		// if user left-clicks in window, zoom in, centering on cursor position
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
			printf("Zooming in...\n");
			// Get cursor position, in *screen coordinates*
			double xCursorPos, yCursorPos;
			glfwGetCursorPos(window, &xCursorPos, &yCursorPos);
			// Adjust mandelbrot coordinate boundaries
			ApplyZoom(xRes, yRes, xCursorPos, yCursorPos, &xMin, &xMax, &yMin, &yMax, 2.0);
			printf("New Boundaries: %lf,%lf,  %lf,%lf, iters: %d\n", xMin,xMax, yMin,yMax, maxIters);
			// Increase iteration count to maintain detail
			maxIters = (int)(maxIters*1.25);
			// Recompute mandelbrot
			RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user right-clicks in window, zoom out, centering on cursor position
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			printf("Zooming out...\n");
			// Get cursor position, in *screen coordinates*
			double xCursorPos, yCursorPos;
			glfwGetCursorPos(window, &xCursorPos, &yCursorPos);
			// Adjust mandelbrot coordinate boundaries
			ApplyZoom(xRes, yRes, xCursorPos, yCursorPos, &xMin, &xMax, &yMin, &yMax, 0.5);
			printf("New Boundaries: %lf,%lf,  %lf,%lf, iters: %d\n", xMin,xMax, yMin,yMax, maxIters);
			// Decrease iteration count to maintain performance
			maxIters = (int)(maxIters/1.25);
			if (maxIters < 50) maxIters = 50;
			// Recompute mandelbrot
			RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "r", reset view
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			printf("Resetting...\n");
			SetInitialValues(&xMin, &xMax, &yMin, &yMax, &maxIters, xRes, yRes);
			// Recompute mandelbrot
			RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "b", run some benchmarks.
		// TODO: pick good regions. One that shows effect of cardioid detection (depends heavily on maxIters,
		// one highly zoomed, high maxIters on a hard-to-draw region.
		else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			printf("Running Benchmarks...\n");
			printf("Whole fractal:\n");
			RunBenchmark(window, image, xRes, yRes, -2.5, 1.5, -1.125, 1.125, 50);
			printf("Half cardioid:\n");
			RunBenchmark(window, image, xRes, yRes, -1.5, 0.25, -0.6, 0.6, 50);
			printf("Highly zoomed:\n");
			printf("Complete.\n");

			// Re-render previous view
			RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

	}



	// clean up
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();


	// Free dynamically allocated memory
	free(image);
	return 0;
}



void RenderMandelbrotCPU(float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();

//	printf("Computing in range %lf to %lf,       %lf to %lf\n", xMin, xMax, yMin, yMax);

	// For each pixel, iterate and store the iteration number when |z|>2 or maxIters
	#pragma omp parallel for default(none) shared(image) schedule(static,xRes/4)
	for (int y = 0; y < yRes; y++) {
		for (int x = 0; x < xRes; x++) {

			int iter= 0;
			double u = 0.0, v = 0.0, uNew;
			double Rec = xMin + ((double)x/(double)xRes) * (xMax-xMin);
			double Imc = yMin + ((double)y/(double)yRes) * (yMax-yMin);

			// early stop if point is inside cardioid
			double q = (Rec - 0.25)*(Rec - 0.25) + Imc*Imc;
			if (q*(q+(Rec-0.25)) < (Imc*Imc*0.25)) {
				iter = maxIters;
			}

			while ( (u*u+v*v) <= 4.0 && iter < maxIters) {
				uNew = u*u-v*v + Rec;
				v = 2*u*v + Imc;
				u = uNew;
				iter++;
			}

			if (iter == maxIters ) {
				// set pixel to be black
				image[y*xRes*3 + x*3 + 0] = 0.0;
				image[y*xRes*3 + x*3 + 1] = 0.0;
				image[y*xRes*3 + x*3 + 2] = 0.0;
			}
			else {
				// set pixel colour
				float smooth = fmod((iter -log(log(u*u+v*v)/log(2.0f))),128.0)/128.0;
				if (smooth < 0.5) {
					image[y*xRes*3 + x*3 + 0] = smooth*2.0;
					image[y*xRes*3 + x*3 + 1] = 0.75*smooth*2.0;
					image[y*xRes*3 + x*3 + 2] = 0.125*(1.0-smooth*2.0);
				}
				else {
					smooth = 1.0 - smooth;
					image[y*xRes*3 + x*3 + 0] = smooth*2.0;
					image[y*xRes*3 + x*3 + 1] = 0.75*smooth*2.0;
					image[y*xRes*3 + x*3 + 2] = 0.125*(1.0-smooth*2.0);
				}
			}

		}
	}

	printf("Frametime: %lf s\n", GetWallTime() - startTime);
}



// {x,y}CursorPos are *screen* coordinates. Need to map them to the mandelbrot coordinates.
void ApplyZoom(const int xRes, const int yRes, const double xCursorPos, const double yCursorPos,
               double * xMin, double * xMax, double * yMin, double * yMax, const double scaleFac)
{
	double xWidth = (*xMax-*xMin);
	double yWidth = (*yMax-*yMin);

	// Map cursor position ( in range [0,res) ) to mandelbrot coords
	double xCursorCoord = (xCursorPos/(double)xRes*xWidth + *xMin);
	double yCursorCoord = (yCursorPos/(double)yRes*yWidth + *yMin);

	*xMax = xCursorCoord + xWidth/2.0/scaleFac;
	*xMin = xCursorCoord - xWidth/2.0/scaleFac;
	*yMax = yCursorCoord + yWidth/2.0/scaleFac;
	*yMin = yCursorCoord - yWidth/2.0/scaleFac;
}



void RunBenchmark(GLFWwindow *window, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();
	int framesRendered = 0;

	while ( (framesRendered < 10) || (GetWallTime() - startTime < 1.0) ) {

		RenderMandelbrotCPU(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Swap buffers
		glfwSwapBuffers(window);
		framesRendered++;
	}

	double fps = (double)framesRendered/(GetWallTime()-startTime);
	printf("       fps: %lf\n", fps);
}



int SetUpOpenGL(GLFWwindow **window, const int xRes, const int yRes,
                GLuint *vertexShader, GLuint *fragmentShader, GLuint *shaderProgram,
                GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *tex)
{
	// Initialise GLFW
	if( !glfwInit() ) {
		fprintf(stderr, "Error in SetUpOpenGL(), failed to initialize GLFW. Line: %d\n", __LINE__);
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
//	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GFLW window and create its OpenGL context
	(*window) = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", NULL, NULL);
	// For fullscreen:
//	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", glfwGetPrimaryMonitor(), NULL);
	if(*window == NULL ){
		fprintf(stderr, "Error in SetUpOpenGL(), failed to open GLFW window. Line: %d\n", __LINE__);
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(*window);

	// Initialize GLEW
	if (glewInit() != GLEW_OK) {
		fprintf(stderr, "Error in SetUpOpenGL(), failed to initialize GLEW. Line: %d\n", __LINE__);
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(*window, GLFW_STICKY_KEYS, GL_TRUE);


	// Include shader strings from file
	#include "shaders.glsl"

	// Define rectangle, on which we'll draw the texture
	float rectangleVertices[] = {
	//  Position      Texcoords
	    -1.0f,  1.0f, 0.0f, 0.0f,
	     1.0f,  1.0f, 1.0f, 0.0f,
	     1.0f, -1.0f, 1.0f, 1.0f,
	    -1.0f, -1.0f, 0.0f, 1.0f
	};

	// vertex array object
	glGenVertexArrays(1, vao);
	glBindVertexArray(*vao);

	// vertex buffer object for rectangle
	glGenBuffers(1,vbo);
	glBindBuffer(GL_ARRAY_BUFFER, *vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectangleVertices), rectangleVertices, GL_STATIC_DRAW);

	// rectangle formed of two triangles, use ebo to re-use vertices
	glGenBuffers(1, ebo);
	GLuint elements[] = {
		0, 1, 2,
	2, 3, 0
	};
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);


	// define and compile shaders
	// vertex
	*vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(*vertexShader, 1, &vertexSource, NULL);
	glCompileShader(*vertexShader);
	GLint status;
	glGetShaderiv(*vertexShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		printf("Vertex shader compilation error, line %d\n", __LINE__);
	}

	// fragment
	*fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(*fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(*fragmentShader);
	glGetShaderiv(*fragmentShader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE) {
		printf("Vertex shader compilation error, line %d\n", __LINE__);
	}

	*shaderProgram = glCreateProgram();
	glAttachShader(*shaderProgram, *vertexShader);
	glAttachShader(*shaderProgram, *fragmentShader);

	glLinkProgram(*shaderProgram);
	glUseProgram(*shaderProgram);

	// set attributes
	GLint posAttrib = glGetAttribLocation(*shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);

	GLint texAttrib = glGetAttribLocation(*shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE,
    	                   4*sizeof(float), (void*)(2*sizeof(float)));


	// define texture
	glGenTextures(1, tex);
	glBindTexture(GL_TEXTURE_2D, *tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	return 0;
}



void WriteImageToFile(float *image, const int xRes, const int yRes)
{
	// Write image to file
	FILE *fp = fopen("image.dat","w");
	for (int y = 0; y < yRes; y++) {
		for (int x = 0; x < xRes; x++) {
			fprintf(fp, "%lf ", image[y*xRes*3 + x*3 + 0]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}



void SetInitialValues(double *xMin, double *xMax, double *yMin, double *yMax, int *maxIters,
                      const int xRes, const int yRes)
{
	// max iteration count. This needs to increase as we zoom in to maintain detail
	*maxIters = 50;

	// mandelbrot coordinates
	*xMin = -2.5;
	*xMax =  1.5;
	// set y limits based on aspect ratio
	*yMin = -(*xMax-*xMin)/2.0*((double)yRes/(double)xRes);
	*yMax =  (*xMax-*xMin)/2.0*((double)yRes/(double)xRes);
}



double GetWallTime(void)
{
	struct timespec tv;
	clock_gettime(CLOCK_REALTIME, &tv);
	return (double)tv.tv_sec + 1e-9*(double)tv.tv_nsec;
}

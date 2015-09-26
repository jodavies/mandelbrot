#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>


#include "mandelbrot.h"
#include "GetWallTime.h"



// For mandelbrot function pointer:
typedef void (*RenderMandelbrotPtr)(float *image, const int xRes, const int yRes,
                                    const double xMin, const double xMax, const double yMin, const double yMax,
                                    const int maxIters);

// Adjust mandelbrot co-ordinates based on scale factor
void ApplyZoom(const int xRes, const int yRes, const double xCursor, const double yCursor,
               double * xMin, double * xMax, double * yMin, double * yMax, const double scaleFac, int * maxIters);

// Test fps for a particular range/zoom, render at least 10 frames or run for 1 second
void RunBenchmark(GLFWwindow *window, RenderMandelbrotPtr RenderMandelbrot, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);

// Set initial values for render ranges, number of iterations. This is a function as it is called
// in more than one place.
void SetInitialValues(double *xMin, double *xMax, double *yMin, double *yMax, int *maxIters,
                      const int xRes, const int yRes);

// Save pixel values to file for external use
void WriteImageToFile(float *image, const int xRes, const int yRes);

// Initialize and configure OpenGL
int SetUpOpenGL(GLFWwindow **window, const int xRes, const int yRes,
                GLuint *vertexShader, GLuint *fragmentShader, GLuint *shaderProgram,
                GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *tex);



int main(void)
{
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotAVXCPU;

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
	RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);

	while (!glfwWindowShouldClose(window)) {

		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap buffers
		glfwSwapBuffers(window);


		// USER INPUT TESTS. Poll for input:
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
			ApplyZoom(xRes, yRes, xCursorPos, yCursorPos, &xMin, &xMax, &yMin, &yMax, 2.0, &maxIters);
			// Recompute mandelbrot
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user right-clicks in window, zoom out, centering on cursor position
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			printf("Zooming out...\n");
			// Get cursor position, in *screen coordinates*
			double xCursorPos, yCursorPos;
			glfwGetCursorPos(window, &xCursorPos, &yCursorPos);
			// Adjust mandelbrot coordinate boundaries
			ApplyZoom(xRes, yRes, xCursorPos, yCursorPos, &xMin, &xMax, &yMin, &yMax, 0.5, &maxIters);
			// Recompute mandelbrot
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "r", reset view
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			printf("Resetting...\n");
			SetInitialValues(&xMin, &xMax, &yMin, &yMax, &maxIters, xRes, yRes);
			// Recompute mandelbrot
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "b", run some benchmarks.
		else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			printf("Running Benchmarks...\n");
			printf("Whole fractal:\n");
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -2.5, 1.5, -1.125, 1.125, 50);
			printf("Half cardioid:\n");
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -1.5, 0.25, -0.6, 0.6, 50);
			printf("Highly zoomed:\n");
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -0.67309614449914079,-0.67303510934289079, -0.31334835466695943,-0.31331402239156880, 2000);
			printf("Complete.\n");

			// Re-render previous view
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "p", zoom right, such that the "naive" algorithm looks pixellated
		else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			printf("Precision test...\n");
			RenderMandelbrot(image, xRes, yRes, -1.25334325335487362,-1.25334325335481678,  -0.34446232396119353,-0.34446232396116155, 1389952);
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
	glfwDestroyWindow(window);
	glfwTerminate();


	// Free dynamically allocated memory
	free(image);
	return 0;
}



// {x,y}CursorPos are *screen* coordinates. Need to map them to the mandelbrot coordinates.
void ApplyZoom(const int xRes, const int yRes, const double xCursorPos, const double yCursorPos,
               double * xMin, double * xMax, double * yMin, double * yMax, const double scaleFac, int * maxIters)
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

	if (scaleFac > 1.0) {
		*maxIters = (int)(*maxIters*1.25);
	}
	else if (scaleFac < 1.0) {
		*maxIters = (int)(*maxIters/1.25);
		if (*maxIters < 50) *maxIters = 50;
	}

	printf("New Boundaries: %.17lf,%.17lf,  %.17lf,%.17lf, iters: %d\n", *xMin,*xMax, *yMin,*yMax, *maxIters);
}



void RunBenchmark(GLFWwindow *window, RenderMandelbrotPtr RenderMandelbrot, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();
	int framesRendered = 0;

	while ( (framesRendered < 5) || (GetWallTime() - startTime < 1.0) ) {

		RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
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
	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", NULL, NULL);
	// For fullscreen:
//	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", glfwGetPrimaryMonitor(), NULL);
	if(*window == NULL ){
		fprintf(stderr, "Error in SetUpOpenGL(), failed to open GLFW window. Line: %d\n", __LINE__);
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(*window);
	// vsync
	glfwSwapInterval(1);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
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





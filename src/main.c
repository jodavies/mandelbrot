#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>

// OpenCL
#ifdef WITHOPENCL
	#include <CL/opencl.h>
	#include <CL/cl_gl.h>
	#include "CheckOpenCLError.h"
#endif


#include "mandelbrot.h"
#include "config.h"
#include "GetWallTime.h"





// For mandelbrot function pointer:
typedef void (*RenderMandelbrotPtr)(float *image, const int xRes, const int yRes,
                                    const double xMin, const double xMax, const double yMin, const double yMax,
                                    const int maxIters);

// Test fps for a particular range/zoom, render at least 10 frames or run for 1 second
void RunBenchmark(GLFWwindow *window, RenderMandelbrotPtr RenderMandelbrot, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters);
#ifdef WITHOPENCL
// OpenCL version requires extra arguments
void RunBenchmarkOpenCL(GLFWwindow *window, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters,
                      cl_command_queue *queue, cl_kernel *renderMandelbrotKernel, cl_kernel *gaussianBlurkernel,
                      cl_mem *pixelsImage, size_t *globalSize, size_t *localSize);
#endif

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

#ifdef WITHOPENCL
// OpenCL Stuff
int InitialiseCLEnvironment(cl_platform_id**, cl_device_id***, cl_context*, cl_command_queue*, cl_program*, GLFWwindow *window);
void CleanUpCLEnvironment(cl_platform_id**, cl_device_id***, cl_context*, cl_command_queue*, cl_program*);

char *kernelFileName = "src/mandelbrotKernel.cl";
#endif


int main(void)
{
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotAVXCPU;

	// Define image size and min/max coords. x is the real axis and y the imaginary
	const int xRes = 2560;
	const int yRes = 1440;

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


#ifdef WITHOPENCL
	// OpenCL variables and setup
	cl_platform_id    *platform;
	cl_device_id      **device_id;
	cl_context        contextCL;
	cl_command_queue  queue;
	cl_program        program;
	cl_kernel         renderMandelbrotKernel, gaussianBlurKernel;
	cl_int            err;
	cl_mem            pixels; // This will be OpenCL-only working memory
	cl_mem            pixelsImage; // This will be aquired from OpenGL texture
	size_t globalSize = xRes*yRes;
	size_t localSize = 64;

	if (InitialiseCLEnvironment(&platform, &device_id, &contextCL, &queue, &program, window) == EXIT_FAILURE) {
		printf("Error initialising OpenCL environment\n");
		return EXIT_FAILURE;
	}
	size_t sizeBytes = xRes * yRes * 3 * sizeof(float);
	pixels = clCreateBuffer(contextCL, CL_MEM_READ_WRITE, sizeBytes, NULL, &err);

	// finish texture initialization so that we can use with OpenCL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
	// Configure image from OpenGL texture "tex"
	pixelsImage = clCreateFromGLTexture(contextCL, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, tex, &err);
	CheckOpenCLError(err, __LINE__);

	// Create kernel
	renderMandelbrotKernel = clCreateKernel(program, "renderMandelbrotKernel", &err);
	CheckOpenCLError(err, __LINE__);
	gaussianBlurKernel = clCreateKernel(program, "gaussianBlurKernel", &err);
	CheckOpenCLError(err, __LINE__);
	err  = clSetKernelArg(renderMandelbrotKernel, 0, sizeof(cl_mem), &pixels);
	err |= clSetKernelArg(renderMandelbrotKernel, 1, sizeof(int), &xRes);
	err |= clSetKernelArg(renderMandelbrotKernel, 2, sizeof(int), &yRes);
	CheckOpenCLError(err, __LINE__);
	err  = clSetKernelArg(gaussianBlurKernel, 0, sizeof(cl_mem), &pixelsImage);
	err |= clSetKernelArg(gaussianBlurKernel, 1, sizeof(int), &xRes);
	err |= clSetKernelArg(gaussianBlurKernel, 2, sizeof(cl_mem), &pixels);
	err |= clSetKernelArg(gaussianBlurKernel, 2, sizeof(cl_mem), &pixels);
	CheckOpenCLError(err, __LINE__);
#endif


	// Start main loop: Update until we encounter user input. Look for Esc key (quit), left and right mount
	// buttons (zoom in on cursor position, zoom out on cursor position), "r" -- reset back to initial coords,
	// "b" -- run some benchmarks, "p" -- display a double precision limited zoom.

	// Re-render the Mandelbrot set as and when we need, in the user input conditionals.
	// Initial render:
#ifdef WITHOPENCL
		RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
		                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
		RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif

	while (!glfwWindowShouldClose(window)) {

		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		// Swap buffers
		glfwSwapBuffers(window);


		// USER INPUT TESTS.
		// Continuous render, poll for input:
		//glfwPollEvents();
		// Render only on update, wait for input:
		glfwWaitEvents();


		// if user presses Esc, close window to leave loop
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		    glfwSetWindowShouldClose(window, GL_TRUE);
		}


		// if user left-clicks in window, zoom in, centering on cursor position
		// if click and drag, simply re-position centre without zooming
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

			// Get Press cursor location
			double xPressPos, yPressPos, xReleasePos, yReleasePos;
			int shift = 0;
			glfwGetCursorPos(window, &xPressPos, &yPressPos);

			// Wait for mousebutton release, re-rendering as mouse moves
			while (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE) {

				glfwGetCursorPos(window, &xReleasePos, &yReleasePos);

				if (fabs(xReleasePos-xPressPos) > 3 || fabs(yReleasePos-yPressPos) > 3) {
					// Set shift variable. Don't zoom below if this is 1
					shift = 1;
					// Determine shift in mandelbrot coords
					double xShift = (xReleasePos-xPressPos)/(double)xRes*(xMax-xMin);
					double yShift = (yReleasePos-yPressPos)/(double)yRes*(yMax-yMin);

					// Add shift to boundaries
					xMin = xMin - xShift;
					xMax = xMax - xShift;
					yMin = yMin - yShift;
					yMax = yMax - yShift;

					// Update "current" (press) position
					xPressPos = xReleasePos;
					yPressPos = yReleasePos;

					// Re-render
#ifdef WITHOPENCL
					RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
					RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
					glfwSwapBuffers(window);
				}
				glfwPollEvents();
			}


			// else, zoom in smoothly over ZOOMSTEPS frames
			if (!shift) {
				// Determine new centre
				const double xCentreNew = (xReleasePos/(double)xRes*(xMax-xMin) + xMin);
				const double yCentreNew = (yReleasePos/(double)yRes*(yMax-yMin) + yMin);
				// Store old min, max values, determine new min, max values
				const double xMinOld = xMin;
				const double xMaxOld = xMax;
				const double yMinOld = yMin;
				const double yMaxOld = yMax;
				const double xMinNew = xCentreNew - (xMax-xMin)/2.0/ZOOMFACTOR;
				const double xMaxNew = xCentreNew + (xMax-xMin)/2.0/ZOOMFACTOR;
				const double yMinNew = yCentreNew - (yMax-yMin)/2.0/ZOOMFACTOR;
				const double yMaxNew = yCentreNew + (yMax-yMin)/2.0/ZOOMFACTOR;
				// Store old maxIters value
				const int maxItersOld = maxIters;

				// Zoom into new position in ZOOMSTEPS steps, interpolating between old and
				// new boundaries
				for (int i = 1; i <= ZOOMSTEPS; i++) {
					double t = INTERPFUNC(i);

					xMin = xMinOld + (xMinNew - xMinOld)*t;
					xMax = xMaxOld + (xMaxNew - xMaxOld)*t;
					yMin = yMinOld + (yMinNew - yMinOld)*t;
					yMax = yMaxOld + (yMaxNew - yMaxOld)*t;
					maxIters = maxItersOld + (ITERSFACTOR-1.0)*maxItersOld*t;

					// Re-render mandelbrot set and draw
#ifdef WITHOPENCL
					RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
					RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
					glfwSwapBuffers(window);
				}
			}

		}


		// if user right-clicks in window, zoom out, centering on cursor position
		else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			while (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE) {
				glfwPollEvents();
			}

			// Get cursor position, in *screen coordinates*
			double xReleasePos, yReleasePos;
			glfwGetCursorPos(window, &xReleasePos, &yReleasePos);

			// Determine new centre
			const double xCentreNew = (xReleasePos/(double)xRes*(xMax-xMin) + xMin);
			const double yCentreNew = (yReleasePos/(double)yRes*(yMax-yMin) + yMin);
			// Store old min, max values, determine new min, max values
			const double xMinOld = xMin;
			const double xMaxOld = xMax;
			const double yMinOld = yMin;
			const double yMaxOld = yMax;
			const double xMinNew = xCentreNew - (xMax-xMin)/2.0*ZOOMFACTOR;
			const double xMaxNew = xCentreNew + (xMax-xMin)/2.0*ZOOMFACTOR;
			const double yMinNew = yCentreNew - (yMax-yMin)/2.0*ZOOMFACTOR;
			const double yMaxNew = yCentreNew + (yMax-yMin)/2.0*ZOOMFACTOR;
			// Store old maxIters value
			const int maxItersOld = maxIters;

			// Zoom into new position in ZOOMSTEPS steps, interpolating between old and
			// new boundaries
			for (int i = 1; i <= ZOOMSTEPS; i++) {
				double t = INTERPFUNC(i);

				xMin = xMinOld + (xMinNew - xMinOld)*t;
				xMax = xMaxOld + (xMaxNew - xMaxOld)*t;
				yMin = yMinOld + (yMinNew - yMinOld)*t;
				yMax = yMaxOld + (yMaxNew - yMaxOld)*t;
				maxIters = maxItersOld + (1.0/ITERSFACTOR-1.0)*maxItersOld*t;

				// Re-render mandelbrot set and draw
#ifdef WITHOPENCL
				RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
		                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
				RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
				glfwSwapBuffers(window);
			}



		}

		// if user presses "r", reset view
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			while (glfwGetKey(window, GLFW_KEY_R) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Resetting...\n");
			SetInitialValues(&xMin, &xMax, &yMin, &yMax, &maxIters, xRes, yRes);
#ifdef WITHOPENCL
			RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
		}

		// if user presses "b", run some benchmarks.
		else if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
			while (glfwGetKey(window, GLFW_KEY_B) != GLFW_RELEASE) {
				glfwPollEvents();
			}

			printf("Running Benchmarks...\n");

			printf("Whole fractal:\n");
#ifdef WITHOPENCL
			RunBenchmarkOpenCL(window, image, xRes, yRes, -2.5, 1.5, -1.125, 1.125, 50,
			             &queue, &renderMandelbrotKernel, &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -2.5, 1.5, -1.125, 1.125, 50);
#endif

			printf("Half cardioid:\n");
#ifdef WITHOPENCL
			RunBenchmarkOpenCL(window, image, xRes, yRes, -1.5, 0.25, -0.6, 0.6, 50,
			             &queue, &renderMandelbrotKernel, &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -1.5, 0.25, -0.6, 0.6, 50);
#endif

			printf("Highly zoomed:\n");
#ifdef WITHOPENCL
			RunBenchmarkOpenCL(window, image, xRes, yRes, -0.67309614449914079,-0.67303510934289079, -0.31334835466695943,-0.31331402239156880, 2000,
			             &queue, &renderMandelbrotKernel, &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RunBenchmark(window, RenderMandelbrot, image, xRes, yRes, -0.67309614449914079,-0.67303510934289079, -0.31334835466695943,-0.31331402239156880, 2000);
#endif
			printf("Complete.\n");
		// Re-render with previous coords
#ifdef WITHOPENCL
			RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
		}

		// if user presses "p", zoom in, such that the double precision algorithm looks pixellated
		else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			while (glfwGetKey(window, GLFW_KEY_P) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Precision test...\n");
			xMin = -1.25334325335487362;
			xMax = -1.25334325335481678;
			yMin = -0.34446232396119353;
			yMax = -0.34446232396116155;
			maxIters = 1389952;
#ifdef WITHOPENCL
			RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &gaussianBlurKernel, &pixelsImage, &globalSize, &localSize);
#else
			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
#endif
		}
	}



	// clean up
#ifdef WITHOPENCL
	CleanUpCLEnvironment(&platform, &device_id, &contextCL, &queue, &program);
#endif

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



void RunBenchmark(GLFWwindow *window, RenderMandelbrotPtr RenderMandelbrot, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters)
{
	double startTime = GetWallTime();
	int framesRendered = 0;

	// disable vsync
	glfwSwapInterval(0);

	while ( (framesRendered < 5) || (GetWallTime() - startTime < 1.0) ) {

		RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Swap buffers
		glfwSwapBuffers(window);
		framesRendered++;
	}

	// reenable vsync
	glfwSwapInterval(1);

	double fps = (double)framesRendered/(GetWallTime()-startTime);
	printf("       fps: %lf\n", fps);
}



#ifdef WITHOPENCL
void RunBenchmarkOpenCL(GLFWwindow *window, float *image, const int xRes, const int yRes,
                      const double xMin, const double xMax, const double yMin, const double yMax,
                      const int maxIters,
                      cl_command_queue *queue, cl_kernel *renderMandelbrotKernel, cl_kernel *gaussianBlurKernel,
                      cl_mem *pixelsImage, size_t *globalSize, size_t *localSize)
{
	double startTime = GetWallTime();
	int framesRendered = 0;

	// disable vsync
	glfwSwapInterval(0);

	while ( (framesRendered < 100) || (GetWallTime() - startTime < 2.0) ) {

		RenderMandelbrotOpenCL(xRes, xMin, xMax, yMin, yMax, maxIters, queue, renderMandelbrotKernel,
		                       gaussianBlurKernel, pixelsImage, globalSize, localSize);
		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Swap buffers
		glfwSwapBuffers(window);
		framesRendered++;
	}

	// reenable vsync
	glfwSwapInterval(1);

	double fps = (double)framesRendered/(GetWallTime()-startTime);
	printf("       fps: %lf\n", fps);
}
#endif



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
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GFLW window and create its OpenGL context
//	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", NULL, NULL);
	// For fullscreen:
	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", glfwGetPrimaryMonitor(), NULL);

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
		2, 3, 0};
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


#ifdef WITHOPENCL
// OpenCL functions
int InitialiseCLEnvironment(cl_platform_id **platform, cl_device_id ***device_id, cl_context *context, cl_command_queue *queue, cl_program *program, GLFWwindow *window)
{
	//error flag
	cl_int err;
	char infostring[1024];

	//get kernel from file
	FILE* kernelFile = fopen(kernelFileName, "rb");
	fseek(kernelFile, 0, SEEK_END);
	long fileLength = ftell(kernelFile);
	rewind(kernelFile);
	char *kernelSource = malloc(fileLength*sizeof(char));
	long read = fread(kernelSource, sizeof(char), fileLength, kernelFile);
	if (fileLength != read) printf("Error reading kernel file, line %d\n", __LINE__);
	fclose(kernelFile);

	//get platform and device information
	cl_uint numPlatforms;
	err = clGetPlatformIDs(0, NULL, &numPlatforms);
	*platform = malloc(numPlatforms * sizeof(cl_platform_id));
	*device_id = malloc(numPlatforms * sizeof(cl_device_id*));
	err |= clGetPlatformIDs(numPlatforms, *platform, NULL);
	CheckOpenCLError(err, __LINE__);
	cl_uint *numDevices;
	numDevices = malloc(numPlatforms * sizeof(cl_uint));

	for (int i = 0; i < numPlatforms; i++) {
		clGetPlatformInfo((*platform)[i], CL_PLATFORM_VENDOR, sizeof(infostring), infostring, NULL);
		printf("\n---OpenCL: Platform Vendor %d: %s\n", i, infostring);

		err = clGetDeviceIDs((*platform)[i], CL_DEVICE_TYPE_ALL, 0, NULL, &(numDevices[i]));
		CheckOpenCLError(err, __LINE__);
		(*device_id)[i] = malloc(numDevices[i] * sizeof(cl_device_id));
		err = clGetDeviceIDs((*platform)[i], CL_DEVICE_TYPE_ALL, numDevices[i], (*device_id)[i], NULL);
		CheckOpenCLError(err, __LINE__);
		for (int j = 0; j < numDevices[i]; j++) {
			char deviceName[200];
			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
			printf("---OpenCL:    Device found %d. %s\n", j, deviceName);
			char deviceInfo[1024];
			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_EXTENSIONS, sizeof(deviceInfo), deviceInfo, NULL);
			if (strstr(deviceInfo, "cl_khr_gl_sharing") != NULL) {
				printf("---OpenCL:        cl_khr_gl_sharing supported!\n");
			}
			else {
				printf("---OpenCL:        cl_khr_gl_sharing NOT supported!\n");
			}
//			cl_ulong maxAlloc;
//			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxAlloc), &maxAlloc, NULL);
//			printf("---OpenCL:       CL_DEVICE_MAX_MEM_ALLOC_SIZE: %lu MB\n", maxAlloc/1024/1024);
//			cl_uint cacheLineSize;
//			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cacheLineSize), &cacheLineSize, NULL);
//			printf("---OpenCL:       CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: %u B\n", cacheLineSize);
		}
	}
	printf("\n");


	////////////////////////////////
	// This part is different to how we usually do things. Need to get context and device from existing
	// OpenGL context. Loop through all platforms looking for the device:
	cl_device_id glDevice;
	int deviceFound = 0;
	int checkPlatform = 0;

	while (!deviceFound) {
		printf("---OpenCL: Looking for OpenGL Context device on platform %d ... ", checkPlatform);
		clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddressForPlatform((*platform)[checkPlatform], "clGetGLContextInfoKHR");
		cl_context_properties properties[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(window),
			CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
			CL_CONTEXT_PLATFORM, (cl_context_properties) (*platform)[checkPlatform],
			0};
		err = pclGetGLContextInfoKHR(properties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &glDevice, NULL);
		if (err != CL_SUCCESS) {
			printf("Not Found.\n");
			checkPlatform++;
			if (checkPlatform > numPlatforms-1) {
				printf("---OpenCL: Error! Could not find OpenGL sharing device.\n");
				return EXIT_FAILURE;
			}
		}
		else {
			printf("Found!\n");
			deviceFound = 1;
		}
	}

	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(window),
		CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) (*platform)[checkPlatform],
		0};
	*context = clCreateContext(properties, 1, &glDevice, NULL, 0, &err);
	CheckOpenCLError(err, __LINE__);
	
	// create a command queue
	*queue = clCreateCommandQueue(*context, glDevice, 0, &err);
	CheckOpenCLError(err, __LINE__);
	////////////////////////////////



	//create the program with the source above
//	printf("Creating CL Program...\n");
	*program = clCreateProgramWithSource(*context, 1, (const char**)&kernelSource, NULL, &err);
	if (err != CL_SUCCESS) {
		printf("Error in clCreateProgramWithSource: %d, line %d.\n", err, __LINE__);
		return EXIT_FAILURE;
	}

	//build program executable
	err = clBuildProgram(*program, 0, NULL, "-I. -I src/", NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error in clBuildProgram: %d, line %d.\n", err, __LINE__);
		char buffer[5000];
		clGetProgramBuildInfo(*program, glDevice, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
		printf("%s\n", buffer);
		return EXIT_FAILURE;
	}

	// dump ptx
	size_t binSize;
	clGetProgramInfo(*program, CL_PROGRAM_BINARY_SIZES, sizeof(size_t), &binSize, NULL);
	unsigned char *bin = malloc(binSize);
	clGetProgramInfo(*program, CL_PROGRAM_BINARIES, sizeof(unsigned char *), &bin, NULL);
	FILE *fp = fopen("openclPTX.ptx", "wb");
	fwrite(bin, sizeof(char), binSize, fp);
	fclose(fp);
	free(bin);

	free(numDevices);
	free(kernelSource);
	return EXIT_SUCCESS;
}
#endif



#ifdef WITHOPENCL
void CleanUpCLEnvironment(cl_platform_id **platform, cl_device_id ***device_id, cl_context *context, cl_command_queue *queue, cl_program *program)
{
	//release CL resources
	clReleaseProgram(*program);
	clReleaseCommandQueue(*queue);
	clReleaseContext(*context);

	cl_uint numPlatforms;
	clGetPlatformIDs(0, NULL, &numPlatforms);
	for (int i = 0; i < numPlatforms; i++) {
		free((*device_id)[i]);
	}
	free(*platform);
	free(*device_id);
}
#endif

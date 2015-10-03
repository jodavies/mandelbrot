#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>

// OpenGL
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>

// OpenCL
#include <CL/opencl.h>
#include <CL/cl_gl.h>


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

// OpenCL Stuff
int InitialiseCLEnvironment(cl_platform_id**, cl_device_id***, cl_context*, cl_command_queue*, cl_program*, GLFWwindow *window);
void CleanUpCLEnvironment(cl_platform_id**, cl_device_id***, cl_context*, cl_command_queue*, cl_program*);
void CheckOpenCLError(cl_int err, int line);

char *kernelFileName = "src/mandelbrotKernel.cl";



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
	cl_platform_id    *platform;
	cl_device_id      **device_id;
	cl_context        contextCL;
	cl_command_queue  queue;
	cl_program        program;
	cl_kernel         renderMandelbrotKernel;
	cl_int            err;
	cl_mem            pixelsImage; // This will be taken from OpenGL
	size_t globalSize = xRes*yRes;
	size_t localSize = 64;

	if (InitialiseCLEnvironment(&platform, &device_id, &contextCL, &queue, &program, window) == EXIT_FAILURE) {
		printf("Error initialising OpenCL environment\n");
		return EXIT_FAILURE;
	}

	// finish texture initialization so that we can use with OpenCL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
	// Configure image from OpenGL texture "tex"
	pixelsImage = clCreateFromGLTexture(contextCL, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, tex, &err);
	CheckOpenCLError(err, __LINE__);

	// Create kernel
	renderMandelbrotKernel = clCreateKernel(program, "renderMandelbrotKernel", &err);


	// Start main loop: Update until we encounter user input. Look for Esc key (quit), left and right mount
	// buttons (zoom in on cursor position, zoom out on cursor position), "r" -- reset back to initial coords etc.

	//Need an initial computation of mandelbrot, and texture update before we enter
//	RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
//	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
	RenderMandelbrotOpenCL(xRes, yRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
	                       &pixelsImage, &globalSize, &localSize);



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
//			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
//			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
			RenderMandelbrotOpenCL(xRes, yRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &pixelsImage, &globalSize, &localSize);
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
//			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
//			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
			RenderMandelbrotOpenCL(xRes, yRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &pixelsImage, &globalSize, &localSize);
		}

		// if user presses "r", reset view
		else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
			printf("Resetting...\n");
			SetInitialValues(&xMin, &xMax, &yMin, &yMax, &maxIters, xRes, yRes);
			// Recompute mandelbrot
//			RenderMandelbrot(image, xRes, yRes, xMin, xMax, yMin, yMax, maxIters);
//			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
			RenderMandelbrotOpenCL(xRes, yRes, xMin, xMax, yMin, yMax, maxIters, &queue, &renderMandelbrotKernel,
			                       &pixelsImage, &globalSize, &localSize);
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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
		}

		// if user presses "p", zoom right, such that the "naive" algorithm looks pixellated
		else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
			printf("Precision test...\n");
//			RenderMandelbrot(image, xRes, yRes, -1.25334325335487362,-1.25334325335481678,  -0.34446232396119353,-0.34446232396116155, 1389952);
//			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xRes, yRes, 0, GL_RGB, GL_FLOAT, image);
			RenderMandelbrotOpenCL(xRes, yRes, -1.25334325335487362,-1.25334325335481678,  -0.34446232396119353,-0.34446232396116155, 1389952, &queue, &renderMandelbrotKernel,
			                       &pixelsImage, &globalSize, &localSize);
		}
	}



	// clean up
	CleanUpCLEnvironment(&platform, &device_id, &contextCL, &queue, &program);

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
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

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
//			cl_ulong maxAlloc;
//			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxAlloc), &maxAlloc, NULL);
//			printf("---OpenCL:       CL_DEVICE_MAX_MEM_ALLOC_SIZE: %lu MB\n", maxAlloc/1024/1024);
//			cl_uint cacheLineSize;
//			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE, sizeof(cacheLineSize), &cacheLineSize, NULL);
//			printf("---OpenCL:       CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: %u B\n", cacheLineSize);
		}
	}

	// Get platform and device from user:
	int chosenPlatform = -1, chosenDevice = -1;
	while (chosenPlatform < 0) {
		printf("\nChoose a platform: ");
		scanf("%d", &chosenPlatform);
		if (chosenPlatform > (numPlatforms-1) || chosenPlatform < 0) {
			chosenPlatform = -1;
			printf("Invalid platform.\n");
		}
	}
	while (chosenDevice < 0) {
		printf("Choose a device: ");
		scanf("%d", &chosenDevice);
		if (chosenDevice > (numDevices[chosenPlatform]-1) || chosenDevice < 0) {
			chosenDevice = -1;
			printf("Invalid device.\n");
		}
	}
	printf("\n");
/*
	//create a context
	*context = clCreateContext(NULL, 1, &((*device_id)[chosenPlatform][chosenDevice]), NULL, NULL, &err);
	CheckOpenCLError(err, __LINE__);
	//create a queue
	*queue = clCreateCommandQueue(*context, (*device_id)[chosenPlatform][chosenDevice], 0, &err);
	CheckOpenCLError(err, __LINE__);
*/

	////////////////////////////////
	// This part is different to how we usually do things. Need to get context and device from existing
	// OpenGL context.
//	clGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn)clGetExtensionFunctionAddress("clGetGLContextInfoKHR");
	clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddressForPlatform((*platform)[chosenPlatform], "clGetGLContextInfoKHR");

	cl_context_properties properties[] = {
		CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(window),
		CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
		CL_CONTEXT_PLATFORM, (cl_context_properties) (*platform)[chosenPlatform],
		0};
	cl_device_id glDevice;
	err = pclGetGLContextInfoKHR(properties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &glDevice, NULL);
	CheckOpenCLError(err, __LINE__);

//	printf("Creating OpenCL context...\n");
	*context = clCreateContext(properties, 1, &glDevice, NULL, 0, &err);
	CheckOpenCLError(err, __LINE__);
	
	// create a command queue
//	printf("Creating OpenCL command queue...\n");
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
//	printf("Building CL Executable...\n");
	err = clBuildProgram(*program, 0, NULL, "-I. -I src/", NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error in clBuildProgram: %d, line %d.\n", err, __LINE__);
		char buffer[5000];
//		clGetProgramBuildInfo(*program, (*device_id)[chosenPlatform][chosenDevice], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
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



void CheckOpenCLError(cl_int err, int line)
{
	if (err != CL_SUCCESS) {
		char * errString;

		switch(err) {
			case   0: errString = "CL_SUCCESS"; break;
			case  -1: errString = "CL_DEVICE_NOT_FOUND"; break;
			case  -2: errString = "CL_DEVICE_NOT_AVAILABLE"; break;
			case  -3: errString = "CL_COMPILER_NOT_AVAILABLE"; break;
			case  -4: errString = "CL_MEM_OBJECT_ALLOCATION_FAILURE"; break;
			case  -5: errString = "CL_OUT_OF_RESOURCES"; break;
			case  -6: errString = "CL_OUT_OF_HOST_MEMORY"; break;
			case  -7: errString = "CL_PROFILING_INFO_NOT_AVAILABLE"; break;
			case  -8: errString = "CL_MEM_COPY_OVERLAP"; break;
			case  -9: errString = "CL_IMAGE_FORMAT_MISMATCH"; break;
			case -10: errString = "CL_IMAGE_FORMAT_NOT_SUPPORTED"; break;
			case -11: errString = "CL_BUILD_PROGRAM_FAILURE"; break;
			case -12: errString = "CL_MAP_FAILURE"; break;
			case -13: errString = "CL_MISALIGNED_SUB_BUFFER_OFFSET"; break;
			case -14: errString = "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST"; break;
			case -15: errString = "CL_COMPILE_PROGRAM_FAILURE"; break;
			case -16: errString = "CL_LINKER_NOT_AVAILABLE"; break;
			case -17: errString = "CL_LINK_PROGRAM_FAILURE"; break;
			case -18: errString = "CL_DEVICE_PARTITION_FAILED"; break;
			case -19: errString = "CL_KERNEL_ARG_INFO_NOT_AVAILABLE"; break;
			case -30: errString = "CL_INVALID_VALUE"; break;
			case -31: errString = "CL_INVALID_DEVICE_TYPE"; break;
			case -32: errString = "CL_INVALID_PLATFORM"; break;
			case -33: errString = "CL_INVALID_DEVICE"; break;
			case -34: errString = "CL_INVALID_CONTEXT"; break;
			case -35: errString = "CL_INVALID_QUEUE_PROPERTIES"; break;
			case -36: errString = "CL_INVALID_COMMAND_QUEUE"; break;
			case -37: errString = "CL_INVALID_HOST_PTR"; break;
			case -38: errString = "CL_INVALID_MEM_OBJECT"; break;
			case -39: errString = "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR"; break;
			case -40: errString = "CL_INVALID_IMAGE_SIZE"; break;
			case -41: errString = "CL_INVALID_SAMPLER"; break;
			case -42: errString = "CL_INVALID_BINARY"; break;
			case -43: errString = "CL_INVALID_BUILD_OPTIONS"; break;
			case -44: errString = "CL_INVALID_PROGRAM"; break;
			case -45: errString = "CL_INVALID_PROGRAM_EXECUTABLE"; break;
			case -46: errString = "CL_INVALID_KERNEL_NAME"; break;
			case -47: errString = "CL_INVALID_KERNEL_DEFINITION"; break;
			case -48: errString = "CL_INVALID_KERNEL"; break;
			case -49: errString = "CL_INVALID_ARG_INDEX"; break;
			case -50: errString = "CL_INVALID_ARG_VALUE"; break;
			case -51: errString = "CL_INVALID_ARG_SIZE"; break;
			case -52: errString = "CL_INVALID_KERNEL_ARGS"; break;
			case -53: errString = "CL_INVALID_WORK_DIMENSION"; break;
			case -54: errString = "CL_INVALID_WORK_GROUP_SIZE"; break;
			case -55: errString = "CL_INVALID_WORK_ITEM_SIZE"; break;
			case -56: errString = "CL_INVALID_GLOBAL_OFFSET"; break;
			case -57: errString = "CL_INVALID_EVENT_WAIT_LIST"; break;
			case -58: errString = "CL_INVALID_EVENT"; break;
			case -59: errString = "CL_INVALID_OPERATION"; break;
			case -60: errString = "CL_INVALID_GL_OBJECT"; break;
			case -61: errString = "CL_INVALID_BUFFER_SIZE"; break;
			case -62: errString = "CL_INVALID_MIP_LEVEL"; break;
			case -63: errString = "CL_INVALID_GLOBAL_WORK_SIZE"; break;
			case -64: errString = "CL_INVALID_PROPERTY"; break;
			case -65: errString = "CL_INVALID_IMAGE_DESCRIPTOR"; break;
			case -66: errString = "CL_INVALID_COMPILER_OPTIONS"; break;
			case -67: errString = "CL_INVALID_LINKER_OPTIONS"; break;
			case -68: errString = "CL_INVALID_DEVICE_PARTITION_COUNT"; break;
			case -1000: errString = "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR"; break;
			case -1001: errString = "CL_PLATFORM_NOT_FOUND_KHR"; break;
			case -1002: errString = "CL_INVALID_D3D10_DEVICE_KHR"; break;
			case -1003: errString = "CL_INVALID_D3D10_RESOURCE_KHR"; break;
			case -1004: errString = "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR"; break;
			case -1005: errString = "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR"; break;
			default: errString = "Unknown OpenCL error";
		}
		printf("OpenCL Error %d (%s), line %d\n", err, errString, line);
	}
}

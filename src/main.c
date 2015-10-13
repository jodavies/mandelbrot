#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

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

#ifdef WITHFREEIMAGE
	#include <FreeImage.h>
#endif



// For mandelbrot function pointer:
typedef void (*RenderMandelbrotPtr)(renderStruct *render, imageStruct *image);

// Test fps for a given range/zoom, render at least 10 frames or run for 1 second
void RunBenchmark(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot);

// Set initial values for render ranges, number of iterations.
void SetInitialValues(imageStruct *image);

// Smoothly zoom from one configuration to another, drawing interpolated frames.
void SmoothZoom(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot,
                const double xReleasePos, const double yReleasePos,
                const double zoomFactor, const double itersFactor);

#ifdef WITHFREEIMAGE
// Make a high-resolution render of the current view, and save to disk as bmp with FreeImage library
void HighResolutionRender(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot);
#endif

// Initialize and configure OpenGL
int SetUpOpenGL(GLFWwindow **window, const int xRes, const int yRes,
                GLuint *vertexShader, GLuint *fragmentShader, GLuint *shaderProgram,
                GLuint *vao, GLuint *vbo, GLuint *ebo, GLuint *tex);

#ifdef WITHOPENCL
// OpenCL Stuff
int InitialiseCLEnvironment(cl_platform_id**, cl_device_id***, cl_program*, renderStruct *render);
void CleanUpCLEnvironment(cl_platform_id**, cl_device_id***, cl_context*, cl_command_queue*, cl_program*);

char *kernelFileName = "src/mandelbrotKernel.cl";
#endif


int main(void)
{
	printf("\n"
	       "Controls:  - Left/Right Click to zoom in/out, centring on cursor position.\n"
	       "           - Left Click and Drag to pan.\n"
	       "           - r to reset view.\n"
	       "           - q,w to decrease, increase max iteration count\n"
	       "           - g to toggle Gaussian Blur after computation\n"
	       "           - b to run some benchmarks.\n"
	       "           - p to show a double-precision limited zoom.\n"
	       "           - h to save a high resolution image of the current view to current directory.\n"
	       "           - Esc to quit.\n\n");

	// Set render function, dependent on compile time flag. All have the same signature,
	// with all necessary variables defined inside the structs.
#ifdef WITHOPENCL
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotOpenCL;
#elif defined(WITHAVX)
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotAVXCPU;
	// AVX double prec vector width (4) must divide horizontal (x) resolution
	assert(XRESOLUTION % 4 == 0);
#elif defined(WITHGMP)
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotGMPCPU;
#else
	RenderMandelbrotPtr RenderMandelbrot = &RenderMandelbrotCPU;
#endif


	// Define and initialize structs
	imageStruct image;
	renderStruct render;
	// Set image resolution
	image.xRes = XRESOLUTION;
	image.yRes = YRESOLUTION;
	// Initial values for boundaries, iteration count
	SetInitialValues(&image);
	// Update OpenGL texture on render. This is disabled when rendering high resolution images
	render.updateTex = 1;
	// Allocate host memory, used to set up OpenGL texture, even if we are using interop OpenCL
	image.pixels = malloc(image.xRes * image.yRes * sizeof *(image.pixels) *3);


	// OpenGL variables and setup
	render.window = NULL;
	GLuint vertexShader, fragmentShader, shaderProgram;
	GLuint vao, vbo, ebo, tex;
	SetUpOpenGL(&(render.window), image.xRes, image.yRes, &vertexShader, &fragmentShader, &shaderProgram, &vao, &vbo, &ebo, &tex);


#ifdef WITHOPENCL
	// OpenCL variables and setup
	cl_platform_id    *platform;
	cl_device_id      **device_id;
	cl_program        program;
	cl_int            err;
	render.globalSize = image.xRes * image.yRes;
	render.localSize = OPENCLLOCALSIZE;
	assert(render.globalSize % render.localSize == 0);

	// Initially set variable that controls interop of OpenGL and OpenCL to 0, set to 1 if
	// interop device found successfully
	render.glclInterop = 0;

	if (InitialiseCLEnvironment(&platform, &device_id, &program, &render) == EXIT_FAILURE) {
		printf("Error initialising OpenCL environment\n");
		return EXIT_FAILURE;
	}
	size_t sizeBytes = image.xRes * image.yRes * 3 * sizeof(float);
	render.pixelsDevice = clCreateBuffer(render.contextCL, CL_MEM_READ_WRITE, sizeBytes, NULL, &err);
	// if we aren't using interop, allocate another buffer on the device for output, on the pointer
	// for the texture
	if (render.glclInterop == 0) {
		render.pixelsTex = clCreateBuffer(render.contextCL, CL_MEM_READ_WRITE, sizeBytes, NULL, &err);
	}

	// finish texture initialization so that we can use with OpenCL if glclInterop
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.xRes, image.yRes, 0, GL_RGB, GL_FLOAT, image.pixels);
	// Configure image from OpenGL texture "tex"
	if (render.glclInterop) {
		render.pixelsTex = clCreateFromGLTexture(render.contextCL, CL_MEM_WRITE_ONLY, GL_TEXTURE_2D, 0, tex, &err);
		CheckOpenCLError(err, __LINE__);
	}


	// Create kernels
	render.renderMandelbrotKernel = clCreateKernel(program, "renderMandelbrotKernel", &err);
	CheckOpenCLError(err, __LINE__);
	render.gaussianBlurKernel = clCreateKernel(program, "gaussianBlurKernel", &err);
	CheckOpenCLError(err, __LINE__);
	render.gaussianBlurKernel2 = clCreateKernel(program, "gaussianBlurKernel2", &err);
	CheckOpenCLError(err, __LINE__);
#endif


	// Start main loop: Update until we encounter user input. Look for Esc key (quit), left and right mount
	// buttons (zoom in on cursor position, zoom out on cursor position), "r" -- reset back to initial coords,
	// "b" -- run some benchmarks, "p" -- display a double precision limited zoom.
	// Re-render the Mandelbrot set as and when we need, in the user input conditionals.

	// Initial render:
	RenderMandelbrot(&render, &image);

	while (!glfwWindowShouldClose(render.window)) {

		// draw
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		// Swap buffers
		glfwSwapBuffers(render.window);


		// USER INPUT TESTS.
		// Continuous render, poll for input:
		//glfwPollEvents();
		// Render only on update, wait for input:
		glfwWaitEvents();


		// if user presses Esc, close window to leave loop
		if (glfwGetKey(render.window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		    glfwSetWindowShouldClose(render.window, GL_TRUE);
		}


		// if user left-clicks in window, zoom in, centring on cursor position
		// if click and drag, simply re-position centre without zooming
		else if (glfwGetMouseButton(render.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {

			// Get Press cursor location
			double xPressPos, yPressPos, xReleasePos, yReleasePos;
			int shift = 0;
			glfwGetCursorPos(render.window, &xPressPos, &yPressPos);

			// Wait for mousebutton release, re-rendering as mouse moves
			while (glfwGetMouseButton(render.window, GLFW_MOUSE_BUTTON_LEFT) != GLFW_RELEASE) {

				glfwGetCursorPos(render.window, &xReleasePos, &yReleasePos);

				if (fabs(xReleasePos-xPressPos) > DRAGPIXELS || fabs(yReleasePos-yPressPos) > DRAGPIXELS) {
					// Set shift variable. Don't zoom after button release if this is 1
					shift = 1;
					// Determine shift in mandelbrot coords
					double xShift = (xReleasePos-xPressPos)/(double)image.xRes*(image.xMax-image.xMin);
					double yShift = (yReleasePos-yPressPos)/(double)image.yRes*(image.yMax-image.yMin);

					// Add shift to boundaries
					image.xMin = image.xMin - xShift;
					image.xMax = image.xMax - xShift;
					image.yMin = image.yMin - yShift;
					image.yMax = image.yMax - yShift;

					// Update "current" (press) position
					xPressPos = xReleasePos;
					yPressPos = yReleasePos;

					// Re-render and draw
					RenderMandelbrot(&render, &image);
					glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
					glfwSwapBuffers(render.window);
				}
				glfwPollEvents();
			}

			// else, zoom in smoothly over ZOOMSTEPS frames
			if (!shift) {
				SmoothZoom(&render, &image, RenderMandelbrot, xReleasePos, yReleasePos, ZOOMFACTOR, ITERSFACTOR);
			}
		}


		// if user right-clicks in window, zoom out, centring on cursor position
		else if (glfwGetMouseButton(render.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
			while (glfwGetMouseButton(render.window, GLFW_MOUSE_BUTTON_RIGHT) != GLFW_RELEASE) {
				glfwPollEvents();
			}

			// Get cursor position, in *screen coordinates*
			double xReleasePos, yReleasePos;
			glfwGetCursorPos(render.window, &xReleasePos, &yReleasePos);

			// Zooming out, so use 1/FACTORs.
			SmoothZoom(&render, &image, RenderMandelbrot, xReleasePos, yReleasePos, 1.0/ZOOMFACTOR, 1.0/ITERSFACTOR);
		}


		// if user presses "r", reset view
		else if (glfwGetKey(render.window, GLFW_KEY_R) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_R) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Resetting...\n");
			SetInitialValues(&image);
			RenderMandelbrot(&render, &image);
		}


		// if user presses "g", toggle gaussian blur
		else if (glfwGetKey(render.window, GLFW_KEY_G) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_G) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			if (image.gaussianBlur == 1) {
				printf("Toggling Gaussian Blur Off...\n");
				image.gaussianBlur = 0;
			}
			else {
				printf("Toggling Gaussian Blur On...\n");
				image.gaussianBlur = 1;
			}
			RenderMandelbrot(&render, &image);
		}


		// if user presses "q", decrease max iteration count
		else if (glfwGetKey(render.window, GLFW_KEY_Q) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_Q) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Decreasing max iteration count from %d to %d\n", image.maxIters, (int)(image.maxIters/ITERSFACTOR));
			image.maxIters /= ITERSFACTOR;
			RenderMandelbrot(&render, &image);
		}
		// if user presses "w", increase max iteration count
		else if (glfwGetKey(render.window, GLFW_KEY_W) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_W) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Increasing max iteration count from %d to %d\n", image.maxIters, (int)(image.maxIters*ITERSFACTOR));
			image.maxIters *= ITERSFACTOR;
			RenderMandelbrot(&render, &image);
		}


		// if user presses "b", run some benchmarks.
		else if (glfwGetKey(render.window, GLFW_KEY_B) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_B) != GLFW_RELEASE) {
				glfwPollEvents();
			}

			printf("Running Benchmarks...\n");

			printf("Whole fractal:\n");
			SetInitialValues(&image);
			RunBenchmark(&render, &image, RenderMandelbrot);

			printf("Early Bail-out:\n");
			image.xMin = -0.8153143016681144;
			image.xMax = -0.6839170011300622;
			image.yMin = -0.0365167077914237;
			image.yMax =  0.0373942737612310;
			image.maxIters = 112;
			RunBenchmark(&render, &image, RenderMandelbrot);

			printf("Spiral:\n");
			image.xMin = -0.8673755781976442;
			image.xMax = -0.8673711898931797;
			image.yMin = -0.2156059883952151;
			image.yMax = -0.2156035199739536;
			image.maxIters = 1757;
			RunBenchmark(&render, &image, RenderMandelbrot);

			printf("Highly zoomed:\n");
			image.xMin = -0.8712903154956539;
			image.xMax = -0.8712903108993595;
			image.yMin = -0.2293516610223087;
			image.yMax = -0.2293516584368930;
			image.maxIters = 10750;
			RunBenchmark(&render, &image, RenderMandelbrot);

			printf("Complete.\n");
		// Re-render with original coords
			SetInitialValues(&image);
			RenderMandelbrot(&render, &image);
		}


		// if user presses "p", zoom in, such that the double precision algorithm looks pixellated
		else if (glfwGetKey(render.window, GLFW_KEY_P) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_P) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			printf("Precision test...\n");
			image.xMin = -1.25334325335487362;
			image.xMax = -1.25334325335481678;
			image.yMin = -0.34446232396119353;
			image.yMax = -0.34446232396116155;
			image.maxIters = 1389952;
			RenderMandelbrot(&render, &image);
		}


		// if user presses "h", render a high resolution version of the current view, and
		// save it to disk as an image
		else if (glfwGetKey(render.window, GLFW_KEY_H) == GLFW_PRESS) {
			while (glfwGetKey(render.window, GLFW_KEY_H) != GLFW_RELEASE) {
				glfwPollEvents();
			}
			double startTime = GetWallTime();
			printf("Saving high resolution (%d x %d) image...\n",
			       image.xRes*HIGHRESOLUTIONMULTIPLIER, image.yRes*HIGHRESOLUTIONMULTIPLIER);
			HighResolutionRender(&render, &image, RenderMandelbrot);
			printf("   --- done. Total time: %lfs\n", GetWallTime()-startTime);
		}
	}



	// clean up
#ifdef WITHOPENCL
	CleanUpCLEnvironment(&platform, &device_id, &(render.contextCL), &(render.queue), &program);
#endif

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);

	// Close OpenGL window and terminate GLFW
	glfwDestroyWindow(render.window);
	glfwTerminate();

	// Free dynamically allocated memory
	free(image.pixels);
	return 0;
}



void RunBenchmark(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot)
{
	double startTime = GetWallTime();
	int framesRendered = 0;

	// disable vsync
	glfwSwapInterval(0);

	while ( (framesRendered < 20) || (GetWallTime() - startTime < 5.0) ) {

		RenderMandelbrot(render, image);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(render->window);
		framesRendered++;
	}

	// reenable vsync
	glfwSwapInterval(1);

	double fps = (double)framesRendered/(GetWallTime()-startTime);
	printf("       fps: %lf\n", fps);
}



void SmoothZoom(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot,
                const double xReleasePos, const double yReleasePos,
                const double zoomFactor, const double itersFactor)
{
	// Determine new centre
	const double xCentreNew = (xReleasePos/(double)image->xRes*(image->xMax-image->xMin) + image->xMin);
	const double yCentreNew = (yReleasePos/(double)image->yRes*(image->yMax-image->yMin) + image->yMin);

	// Store old min, max values, determine new min, max values
	const double xMinOld = image->xMin;
	const double xMaxOld = image->xMax;
	const double yMinOld = image->yMin;
	const double yMaxOld = image->yMax;
	const double xMinNew = xCentreNew - (image->xMax-image->xMin)/2.0/zoomFactor;
	const double xMaxNew = xCentreNew + (image->xMax-image->xMin)/2.0/zoomFactor;
	const double yMinNew = yCentreNew - (image->yMax-image->yMin)/2.0/zoomFactor;
	const double yMaxNew = yCentreNew + (image->yMax-image->yMin)/2.0/zoomFactor;
	// Store old maxIters value
	const int maxItersOld = image->maxIters;


	// Zoom into new position in ZOOMSTEPS steps, interpolating between old and new boundaries.
	double time = GetWallTime();
	for (int i = 1; i <= image->zoomSteps; i++) {
		double t = INTERPFUNC((double)i/(double)image->zoomSteps);
		image->xMin = xMinOld + (xMinNew - xMinOld)*t;
		image->xMax = xMaxOld + (xMaxNew - xMaxOld)*t;
		image->yMin = yMinOld + (yMinNew - yMinOld)*t;
		image->yMax = yMaxOld + (yMaxNew - yMaxOld)*t;
		image->maxIters = maxItersOld + (itersFactor-1.0)*maxItersOld*t;

		// Re-render mandelbrot set and draw
		RenderMandelbrot(render, image);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glfwSwapBuffers(render->window);
	}

	// Want zoom to take ~half a second, so if it is taking too much or too little time,
	// adjust image->zoomSteps to compensate.
	time = GetWallTime() - time;
	if (time > 0.75) {
		int newZoomSteps = (int)fmax(1.0, (0.5/time * image->zoomSteps));
		image->zoomSteps = newZoomSteps;
	}
	else if (time < 0.25) {
		int newZoomSteps = (int)fmin(INITIALZOOMSTEPS, (0.5/time * image->zoomSteps));
		image->zoomSteps = newZoomSteps;
	}
}



#ifdef WITHFREEIMAGE
void HighResolutionRender(renderStruct *render, imageStruct *image, RenderMandelbrotPtr RenderMandelbrot)
{
	// Set new resolution
	image->xRes = image->xRes*HIGHRESOLUTIONMULTIPLIER;
	image->yRes = image->yRes*HIGHRESOLUTIONMULTIPLIER;

	//Set updateTex flag to 0 so we don't try to draw it on screen.
	render->updateTex = 0;

	// Allocate new host pixels array of larger size. We need this regardless of interop
	free(image->pixels);
	// CAREFUL: these sizes can easily overflow a 32bit int. Use uint64_t
	uint64_t allocSize = image->xRes * image->yRes * sizeof*(image->pixels) *3;
	printf("   --- reallocating host pixels array: %.2lfMB\n", allocSize/1024.0/1024.0);
	image->pixels = malloc(allocSize);


#ifdef WITHOPENCL
	// Here, it is likely that the array(s) of colour values will not fit in device global memory.
	// We have to render the frame in tiles, each of which fits.
	//
	// OpenCL platform gives us a max allocation, which is "at least" 1/4 of the memory size. We
	// can't therefore, allocate two arrays of the max allocation -- they are not guaranteed to fit.
	// We allocate two arrays, each half the max allocation.
	cl_int err;

	// Determine the size (in bytes) of one row of pixels
	uint64_t rowSize = image->xRes * sizeof *(image->pixels) * 3;

	// Determine how many rows we can fit in half the max allocation
	uint64_t maxAllocRows = (render->deviceMaxAlloc/2) / rowSize;

	// The number of tiles required to render the frame:
	int tiles = (int)ceil((double)image->yRes/(double)maxAllocRows);
	// Size of a single tile
	uint64_t fullTileSize = maxAllocRows * rowSize;

	// Allocate tile-sized global memory arrays on device.
	// Store handle to OpenGL texture so it can be recovered later.
	// The struct variable will be reassigned, this makes running the kernels simpler.
	cl_mem keepPixelsTex = render->pixelsTex;
	// Release the existing pixels array
	err = clReleaseMemObject(render->pixelsDevice);
	CheckOpenCLError(err, __LINE__);
	// Allocate new arrays
	printf("   --- allocating tile arrays on device: %lfMB\n", 2.0*fullTileSize/1024.0/1024.0);
	render->pixelsDevice = clCreateBuffer(render->contextCL, CL_MEM_READ_WRITE, fullTileSize, NULL, &err);
	CheckOpenCLError(err, __LINE__);
	render->pixelsTex = clCreateBuffer(render->contextCL, CL_MEM_READ_WRITE, fullTileSize, NULL, &err);
	CheckOpenCLError(err, __LINE__);

	// Store full image resolution and boundaries, we will adjust the values in the struct
	int yResFull = image->yRes;
	double yMinFull = image->yMin;
	double yMaxFull = image->yMax;


	// Render tiles, and copy data back to the host array
	for (int t = 0; t < tiles; t++) {
		printf("   --- computing tile %d/%d...\n", t+1, tiles);

		// Set tile resolution. Each has maxAllocRows apart from the last,
		// which might have fewer as it contains the remainder.
		image->yRes = maxAllocRows;
		if (t == tiles-1) {
			image->yRes = yResFull % maxAllocRows;
		}
		// Reset global size, as the resolution has changed
		render->globalSize = image->yRes * image->xRes;
		assert(render->globalSize % render->localSize == 0);

		// Set tile boundaries. We are computing a fraction maxAllocRows/yResFull of the full
		//image, starting at the beginning of the t-th tile. The final tile uses xMaxFull for xMax.
		image->yMin = yMinFull + t*((double)maxAllocRows/(double)yResFull)*(yMaxFull-yMinFull);
		image->yMax = yMinFull + (t+1)*((double)maxAllocRows/(double)yResFull)*(yMaxFull-yMinFull);
		if (t == tiles-1) {
			image->yMax = yMaxFull;
		}

		// Render
		RenderMandelbrot(render, image);

		// Copy data from render->pixelsTex (the output of GaussianBlurKernel2)
		uint64_t storeOffset = t * (maxAllocRows * image->xRes * 3);
		// The final tile is smaller
		if (t == tiles-1) {
			fullTileSize = image->yRes * image->xRes * sizeof *(image->pixels) * 3;
		}
		err = clEnqueueReadBuffer(render->queue, render->pixelsTex, CL_TRUE, 0, fullTileSize,
		                          &(image->pixels[storeOffset]), 0, NULL, NULL);
		CheckOpenCLError(err, __LINE__);
	}

	// Reset image boundaries and resolution
	image->yRes = yResFull;
	image->yMin = yMinFull;
	image->yMax = yMaxFull;


#else
	// If not using OpenCL, simply render directly onto the reallocated image->pixels array
	RenderMandelbrot(render, image);
#endif


	// Save png
	printf("   --- creating png...\n");
	// Create raw pixel array
	GLubyte * rawPixels;
	uint64_t rawAllocSize = image->xRes * image->yRes * sizeof *rawPixels *3;
	rawPixels = malloc(rawAllocSize);
	for (uint64_t i = 0; i < image->xRes*image->yRes*3; i+=3) {
		// note change in order, rgb -> bgr!
		rawPixels[i+0] = (GLubyte)((image->pixels)[i+2]*255);
		rawPixels[i+1] = (GLubyte)((image->pixels)[i+1]*255);
		rawPixels[i+2] = (GLubyte)((image->pixels)[i+0]*255);
	}
	FIBITMAP* img = FreeImage_ConvertFromRawBits(rawPixels, image->xRes, image->yRes, 3*image->xRes,
	                                             24, 0x000000, 0x000000, 0x000000, TRUE);
	FreeImage_Save(FIF_PNG, img, "test.png", 0);
	FreeImage_Unload(img);
	free(rawPixels);


	// Reset original values to continue with live rendering
	image->xRes = image->xRes/HIGHRESOLUTIONMULTIPLIER;
	image->yRes = image->yRes/HIGHRESOLUTIONMULTIPLIER;
	render->updateTex = 1;
	free(image->pixels);
	image->pixels = malloc(image->xRes * image->yRes * sizeof *(image->pixels) *3);

#ifdef WITHOPENCL
	// Release large global memory buffers
	clReleaseMemObject(render->pixelsDevice);
	clReleaseMemObject(render->pixelsTex);
	// Recover handle to OpenGL texture
	render->pixelsTex = keepPixelsTex;
	size_t allocSizeOrig = image->xRes * image->yRes * 3 * sizeof *(image->pixels);
	render->pixelsDevice = clCreateBuffer(render->contextCL, CL_MEM_READ_WRITE, allocSizeOrig, NULL, &err);
	CheckOpenCLError(err, __LINE__);
	// Reset global size, as the resolution has changed
	render->globalSize = image->yRes * image->xRes;
	assert(render->globalSize % render->localSize == 0);
#endif

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
#ifdef FULLSCREEN
	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", glfwGetPrimaryMonitor(), NULL);
#else
	*window = glfwCreateWindow( xRes, yRes, "Mandelbrot Set", NULL, NULL);
#endif

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



void SetInitialValues(imageStruct *image)
{
	// max iteration count. This needs to increase as we zoom in to maintain detail
	image->maxIters = MINITERS;

	// mandelbrot coordinates
	image->xMin = -2.5;
	image->xMax =  1.5;
	// set y limits based on aspect ratio
	image->yMin = -(image->xMax-image->xMin)/2.0*((double)image->yRes/(double)image->xRes);
	image->yMax =  (image->xMax-image->xMin)/2.0*((double)image->yRes/(double)image->xRes);

	// Gaussian blur after computation
	image->gaussianBlur = DEFAULTGAUSSIANBLUR;

	// Intermediate frames for SmoothZoom function
	image->zoomSteps = INITIALZOOMSTEPS;
}


#ifdef WITHOPENCL
// OpenCL functions
int InitialiseCLEnvironment(cl_platform_id **platform, cl_device_id ***device_id, cl_program *program, renderStruct *render)
{
	// error flag
	cl_int err;
	char infostring[1024];
	char deviceInfo[1024];

	// need to ensure platform supports OpenGL OpenCL interop before querying devices
	// to avoid segfault when calling clGetGLContextInfoKHR
	int *platformSupportsInterop;

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
	platformSupportsInterop = malloc(numPlatforms * sizeof(*platformSupportsInterop));
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
		platformSupportsInterop[i] = 0;
		err = clGetDeviceIDs((*platform)[i], CL_DEVICE_TYPE_ALL, numDevices[i], (*device_id)[i], NULL);
		CheckOpenCLError(err, __LINE__);
		for (int j = 0; j < numDevices[i]; j++) {
			char deviceName[200];
			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_NAME, sizeof(deviceName), deviceName, NULL);
			printf("---OpenCL:    Device found %d. %s\n", j, deviceName);
			clGetDeviceInfo((*device_id)[i][j], CL_DEVICE_EXTENSIONS, sizeof(deviceInfo), deviceInfo, NULL);
			if (strstr(deviceInfo, "cl_khr_gl_sharing") != NULL) {
				printf("---OpenCL:        cl_khr_gl_sharing supported!\n");
				platformSupportsInterop[i] = 1;
			}
			else {
				printf("---OpenCL:        cl_khr_gl_sharing NOT supported!\n");
				platformSupportsInterop[i] |= 0;
			}
			if (strstr(deviceInfo, "cl_khr_fp64") != NULL) {
				printf("---OpenCL:        cl_khr_fp64 supported!\n");
			}
			else {
				printf("---OpenCL:        cl_khr_fp64 NOT supported!\n");
			}
		}
	}
	printf("\n");


	////////////////////////////////
	// This part is different to how we usually do things. Need to get context and device from existing
	// OpenGL context. Loop through all platforms looking for the device:
	cl_device_id device = NULL;
	int deviceFound = 0;
	int checkPlatform = 0;

#ifdef TRYINTEROP
	while (!deviceFound) {
		if (platformSupportsInterop[checkPlatform]) {
			printf("---OpenCL: Looking for OpenGL Context device on platform %d ... ", checkPlatform);
			clGetGLContextInfoKHR_fn pclGetGLContextInfoKHR = (clGetGLContextInfoKHR_fn) clGetExtensionFunctionAddressForPlatform((*platform)[checkPlatform], "clGetGLContextInfoKHR");
			cl_context_properties properties[] = {
				CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(render->window),
				CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
				CL_CONTEXT_PLATFORM, (cl_context_properties) (*platform)[checkPlatform],
				0};
			err = pclGetGLContextInfoKHR(properties, CL_CURRENT_DEVICE_FOR_GL_CONTEXT_KHR, sizeof(cl_device_id), &device, NULL);
			if (err != CL_SUCCESS) {
				printf("Not Found.\n");
				checkPlatform++;
				if (checkPlatform > numPlatforms-1) {
					printf("---OpenCL: Error! Could not find OpenGL sharing device.\n");
					deviceFound = 1;
					render->glclInterop = 0;
				}
			}
			else {
				printf("Found!\n");
				deviceFound = 1;
				render->glclInterop = 1;
			}
		}
	}

	if (render->glclInterop) {
		// Check the device we've found supports double precision
		clGetDeviceInfo(device, CL_DEVICE_EXTENSIONS, sizeof(deviceInfo), deviceInfo, NULL);
		if (strstr(deviceInfo, "cl_khr_fp64") == NULL) {
			printf("---OpenCL: Interop device doesn't support double precision! We cannot use it.\n");
		}
		else {
			cl_context_properties properties[] = {
				CL_GL_CONTEXT_KHR, (cl_context_properties) glfwGetGLXContext(render->window),
				CL_GLX_DISPLAY_KHR, (cl_context_properties) glfwGetX11Display(),
				CL_CONTEXT_PLATFORM, (cl_context_properties) (*platform)[checkPlatform],
				0};
			render->contextCL = clCreateContext(properties, 1, &device, NULL, 0, &err);
			CheckOpenCLError(err, __LINE__);
		}
	}
#endif

	// if render->glclInterop is 0, either we are not trying to use it, we couldn't find an interop
	// device, or we found an interop device but it doesn't support double precision.
	// In these cases, have the user choose a platform and device manually.
	if (!(render->glclInterop)) {
		printf("Choose a platform and device.\n");
		checkPlatform = -1;
		while (checkPlatform < 0) {
			printf("Platform: ");
			scanf("%d", &checkPlatform);
			if (checkPlatform > numPlatforms-1) {
				printf("Invalid Platform choice.\n");
				checkPlatform = -1;
			}
		}

		int chooseDevice = -1;
		while (chooseDevice < 0) {
			printf("Device: ");
			scanf("%d", &chooseDevice);
			if (chooseDevice > numDevices[checkPlatform]) {
				printf("Invalid Device choice.\n");
				chooseDevice = -1;
			}
			// Check the device we've chosen supports double precision
			clGetDeviceInfo((*device_id)[checkPlatform][chooseDevice], CL_DEVICE_EXTENSIONS, sizeof(deviceInfo), deviceInfo, NULL);
			if (strstr(deviceInfo, "cl_khr_fp64") == NULL) {
				printf("---OpenCL: Interop device doesn't support double precision! We cannot use it.\n");
				chooseDevice = -1;
			}
		}

		// Create non-interop context
		render->contextCL = clCreateContext(NULL, 1, &((*device_id)[checkPlatform][chooseDevice]), NULL, NULL, &err);
		device = (*device_id)[checkPlatform][chooseDevice];
	}
	////////////////////////////////

	// device is now fixed. Query its max global memory allocation size and store it, used in
	// HighResolutionRender routine, to determine into how many tiles we need to split the
	// computation.
	clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(render->deviceMaxAlloc), &(render->deviceMaxAlloc), NULL);
	printf("---OpenCL: Selected device has CL_DEVICE_MAX_MEM_ALLOC_SIZE: %lfMB\n",
	       render->deviceMaxAlloc/1024.0/1024.0);

	// create a command queue
	render->queue = clCreateCommandQueue(render->contextCL, device, 0, &err);
	CheckOpenCLError(err, __LINE__);


	//create the program with the source above
//	printf("Creating CL Program...\n");
	*program = clCreateProgramWithSource(render->contextCL, 1, (const char**)&kernelSource, NULL, &err);
	if (err != CL_SUCCESS) {
		printf("Error in clCreateProgramWithSource: %d, line %d.\n", err, __LINE__);
		return EXIT_FAILURE;
	}

	//build program executable
	err = clBuildProgram(*program, 0, NULL, "-I. -I src/", NULL, NULL);
	if (err != CL_SUCCESS) {
		printf("Error in clBuildProgram: %d, line %d.\n", err, __LINE__);
		char buffer[5000];
		clGetProgramBuildInfo(*program, device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, NULL);
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
	printf("\n");
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

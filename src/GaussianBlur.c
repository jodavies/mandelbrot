#include "GaussianBlur.h"

void GaussianBlur(float *image, const int xRes, const int yRes)
{
	// make a copy of the image
	float *imageCopy;
	imageCopy = malloc(xRes * yRes * sizeof *imageCopy *3);
	for (int j = 0; j < yRes; j++) {
		for (int i = 0; i < xRes; i++) {
			imageCopy[j*xRes*3 + i*3 + 0] = image[j*xRes*3 + i*3 + 0];
			imageCopy[j*xRes*3 + i*3 + 1] = image[j*xRes*3 + i*3 + 1];
			imageCopy[j*xRes*3 + i*3 + 2] = image[j*xRes*3 + i*3 + 2];
		}
	}

	// 3x3 gaussian blur:
	for (int j = 1; j < yRes-1; j++) {
		for (int i = 1; i < xRes-1; i++) {
/*			image[j*xRes*3 + i*3 + 0] = (+1.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 0]
                                         +2.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 0]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 0]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 0]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 0]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 0]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 0]
                                         +2.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 0]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 0])/16.0;
			image[j*xRes*3 + i*3 + 1] = (+1.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 1]
                                         +2.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 1]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 1]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 1]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 1]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 1]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 1]
                                         +2.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 1]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 1])/16.0;
			image[j*xRes*3 + i*3 + 2] = (+1.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 2]
                                         +2.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 2]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 2]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 2]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 2]
                                         +2.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 2]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 2]
                                         +2.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 2]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 2])/16.0;*/
			image[j*xRes*3 + i*3 + 0] = (+0.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 0]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 0]
                                         +0.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 0]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 0]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 0]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 0]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 0]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 0]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 0])/8.0;
			image[j*xRes*3 + i*3 + 1] = (+0.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 1]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 1]
                                         +0.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 1]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 1]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 1]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 1]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 1]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 1]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 1])/8.0;
			image[j*xRes*3 + i*3 + 2] = (+0.0*imageCopy[(j+1)*xRes*3 + (i+1)*3 + 2]
                                         +1.0*imageCopy[(j+1)*xRes*3 + (i+0)*3 + 2]
                                         +0.0*imageCopy[(j+1)*xRes*3 + (i-1)*3 + 2]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i+1)*3 + 2]
                                         +4.0*imageCopy[(j+0)*xRes*3 + (i+0)*3 + 2]
                                         +1.0*imageCopy[(j+0)*xRes*3 + (i-1)*3 + 2]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i+1)*3 + 2]
                                         +1.0*imageCopy[(j-1)*xRes*3 + (i+0)*3 + 2]
                                         +0.0*imageCopy[(j-1)*xRes*3 + (i-1)*3 + 2])/8.0;
		}
	}

	free(imageCopy);
}


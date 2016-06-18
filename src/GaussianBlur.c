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
	for (int j = 0; j < yRes; j++) {
		for (int i = 0; i < xRes; i++) {
			const int ju = (j == yRes-1) ? j : j+1;
			const int jd = (j == 0) ? j : j-1;
			const int il = (i == 0) ? i : i-1;
			const int ir = (i == xRes-1) ? i : i+1;

			image[j*xRes*3 + i*3 + 0] = (   +1.0f*imageCopy[ju*xRes*3 + i*3 + 0]
                                         +1.0f*imageCopy[j*xRes*3 + ir*3 + 0]
                                         +4.0f*imageCopy[j*xRes*3 + i*3 + 0]
                                         +1.0f*imageCopy[j*xRes*3 + il*3 + 0]
                                         +1.0f*imageCopy[jd*xRes*3 + i*3 + 0])/8.0f;
			image[j*xRes*3 + i*3 + 1] = (   +1.0f*imageCopy[ju*xRes*3 + i*3 + 1]
                                         +1.0f*imageCopy[j*xRes*3 + ir*3 + 1]
                                         +4.0f*imageCopy[j*xRes*3 + i*3 + 1]
                                         +1.0f*imageCopy[j*xRes*3 + il*3 + 1]
                                         +1.0f*imageCopy[jd*xRes*3 + i*3 + 1])/8.0f;
			image[j*xRes*3 + i*3 + 2] = (   +1.0f*imageCopy[ju*xRes*3 + i*3 + 2]
                                         +1.0f*imageCopy[j*xRes*3 + ir*3 + 2]
                                         +4.0f*imageCopy[j*xRes*3 + i*3 + 2]
                                         +1.0f*imageCopy[j*xRes*3 + il*3 + 2]
                                         +1.0f*imageCopy[jd*xRes*3 + i*3 + 2])/8.0f;
		}
	}

	free(imageCopy);
}


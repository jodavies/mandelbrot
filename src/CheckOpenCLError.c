#include <stdio.h>
#include "CheckOpenCLError.h"


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

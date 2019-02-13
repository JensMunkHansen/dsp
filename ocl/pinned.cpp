#include "config.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>

#include <string>
using std::string;

#include <sstream>
using std::stringstream;

#include <iostream>
using std::cout;
using std::endl;

/* OpenCL macros */
#include "ocl_common.h"

// #include "shrUtils.h"

static void Cleanup ();
static void (*pCleanup)() = &Cleanup;

static char* file_to_string(const char *fileName);


bool CreateKernelFromFile(const char* filename, const char* kernelname, cl_handles_t& clHandles, size_t i);

cl_platform_id* allPlatforms = NULL;


cl_handles_t clHandles;
cl_buffers_t clBuffers;

size_t n_emissions = 1;

float* iData;
float* oData;

int main(int argc, const char* argv[]) {

  cl_uint numPlatforms;

  cl_platform_id targetPlatform = NULL;

  clHandles.context  = NULL;
  clHandles.devices  = NULL;
  clHandles.queue[0]    = NULL;
  clHandles.program[0]  = NULL;
  clHandles.kernel[0]  = NULL;
  clHandles.ndevices = 0;

  iData = NULL;
  oData = NULL;

  clBuffers.input = NULL;
  clBuffers.output = NULL;

  clBuffers.input_pinned = NULL;

  /* Exits if no platforms found */
  CallClErrExit(clGetPlatformIDs,
                (0,
                 NULL,
                 &numPlatforms),
                EXIT_FAILURE, pCleanup);

  printf("Number of platforms: %d\n",numPlatforms);

  // Call for malloc
  allPlatforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));

  CallClErrExit(clGetPlatformIDs,
                (numPlatforms,
                 allPlatforms,
                 NULL),
                EXIT_FAILURE, pCleanup);

  /* Select the target platform */
  targetPlatform = allPlatforms[0];

  // Find NVIDIA platform
  for (size_t i = 0; i < numPlatforms; i++) {
    char pbuff[128];
    CallClErrExit(clGetPlatformInfo,
                  (allPlatforms[i],
                   CL_PLATFORM_VENDOR,
                   sizeof(pbuff),
                   pbuff,
                   NULL),
                  EXIT_FAILURE, pCleanup);

    printf("%u: %s\n",i,pbuff);
    if (!strcmp(pbuff, "NVIDIA Corporation")) {
      targetPlatform = allPlatforms[i];
      break;
    }
    else if (!strcmp(pbuff, "Intel")) {
      targetPlatform = allPlatforms[i];
      break;
    }
  }

  cl_context_properties cprops[3] =
    { CL_CONTEXT_PLATFORM, (cl_context_properties)targetPlatform, 0 };

  // Select GPU device
  CallClErrExit(clHandles.context = clCreateContextFromType,
                (cprops,
                 CL_DEVICE_TYPE_GPU,
                 NULL,
                 NULL,
                 &clerrno),
                EXIT_FAILURE, pCleanup);

  size_t nFloats = 1024;
  oData = (float*) malloc(nFloats*sizeof(float));
  memset(oData, 12, nFloats*sizeof(float));
  oData[0] = 1.2f;

  CallClErrExit(clBuffers.input = clCreateBuffer,
                (clHandles.context,
                 CL_MEM_READ_ONLY,
                 nFloats*sizeof(float),
                 NULL,
                 &clerrno),
                EXIT_FAILURE, pCleanup);

  CallClErrExit(clBuffers.output = clCreateBuffer,
                (clHandles.context,
                 CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR,
                 nFloats*sizeof(float),
                 oData,
                 &clerrno),
                EXIT_FAILURE, pCleanup);

  // Input data
  /*
  CallClErrExit(clBuffers.input = clCreateBuffer,
                (clHandles.context,
                 CL_MEM_READ_ONLY, // For the GPU
                 nFloats*sizeof(float),
                 NULL,
                 &clerrno),
                EXIT_FAILURE, pCleanup);
*/



  size_t deviceListSize;
  CallClErrExit(clGetContextInfo,
                (clHandles.context,
                 CL_CONTEXT_DEVICES,
                 0,
                 NULL,
                 &deviceListSize),
                EXIT_FAILURE, pCleanup);


  if (deviceListSize == 0) {
    fprintf(stderr, "FAILED: clGetContextInfo(no devices found)\n");
    Cleanup();
    return EXIT_FAILURE;
  }

  clHandles.ndevices = deviceListSize / sizeof(cl_device_id);

  /* Now, allocate the device list */
  clHandles.devices = (cl_device_id *)malloc(deviceListSize);

  if (clHandles.devices == 0) {
    fprintf(stderr, "FAILED: allocating memory for devices\n");
    Cleanup();
    return EXIT_FAILURE;
  }


  /* Next, get the device list data */
  CallClErrExit(clGetContextInfo,
                (clHandles.context,
                 CL_CONTEXT_DEVICES,
                 deviceListSize,
                 clHandles.devices,
                 NULL),
                EXIT_FAILURE, pCleanup);

#if CL_VERSION_2_0
  const int maxQueueSize = 1;
  const cl_queue_properties prop[] = {  CL_QUEUE_PROPERTIES,
                                        CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE|CL_QUEUE_ON_DEVICE|CL_QUEUE_ON_DEVICE_DEFAULT,
                                        CL_QUEUE_SIZE, maxQueueSize, 0 };
  CallClErrExit(clHandles.queue[0] = clCreateCommandQueueWithProperties,
                (clHandles.context,
                 clHandles.devices[0],
                 prop,
                 &clerrno),
                EXIT_FAILURE, pCleanup);
#else
  CallClErrExit(clHandles.queue[0] = clCreateCommandQueue,
                (clHandles.context,
                 clHandles.devices[0],
                 CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE,
                 &clerrno),
                EXIT_FAILURE, pCleanup);
#endif


  // Pinned (Page Locked) memory
  CallClErrExit(clBuffers.input_pinned = clCreateBuffer,
                (clHandles.context,
                 CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR,
                 nFloats*sizeof(float),
                 NULL,
                 &clerrno),
                EXIT_FAILURE, pCleanup);

  CallClErrExit(iData = (cl_float*)clEnqueueMapBuffer,
                (clHandles.queue[0], clBuffers.input_pinned,
                 CL_TRUE, CL_MAP_WRITE, 0,
                 nFloats*sizeof(float), 0,
                 NULL, NULL, &clerrno), EXIT_FAILURE, pCleanup);

  // Write input data
  memset(iData, 0, nFloats*sizeof(float));

  if (!CreateKernelFromFile("./pinned.cl", "Unity", clHandles,0)) {
    Cleanup();
    return EXIT_FAILURE;
  }

  CallClErrExit(clSetKernelArg,
                (clHandles.kernel[0],
                 0,
                 sizeof(cl_mem),
                 (void *)&clBuffers.input),
                EXIT_FAILURE, pCleanup);

  CallClErrExit(clSetKernelArg,
                (clHandles.kernel[0],
                 1,
                 sizeof(cl_mem),
                 (void *)&clBuffers.output),
                EXIT_FAILURE, pCleanup);


  size_t globalNDWorkSize[1];
  size_t localNDWorkSize[1];

  globalNDWorkSize[0] = (size_t)nFloats;

  localNDWorkSize[0] = 16;

  clFinish(clHandles.queue[0]);

  CallClErrExit(clEnqueueWriteBuffer,
		(clHandles.queue[0], clBuffers.input, CL_FALSE, 0,
		 nFloats*sizeof(float), (void*)&iData[0], 0, NULL, NULL),
		EXIT_FAILURE, pCleanup);


  CallClErrExit(clEnqueueNDRangeKernel,
		(clHandles.queue[0], clHandles.kernel[0], 1, NULL,
		 globalNDWorkSize, localNDWorkSize, 0, NULL, NULL),
		EXIT_FAILURE, pCleanup);

  clFinish(clHandles.queue[0]);

  CallClErrExit(clEnqueueReadBuffer,
                (clHandles.queue[0],
                 clBuffers.output,
                 CL_TRUE,
                 0,
                 sizeof(float)*nFloats,
                 oData,
                 0,
                 NULL,
                 NULL),
                EXIT_FAILURE, pCleanup);

  clFinish(clHandles.queue[0]);
  printf("%f\n", oData[0]);
  pCleanup();
  return EXIT_SUCCESS;
}

void Cleanup () {

  if(allPlatforms) {
    cout << "Freeing platforms" << endl;
    free(allPlatforms);
  }

  if(clHandles.kernel[0]) clReleaseKernel(clHandles.kernel[0]);
  if(clHandles.program[0]) clReleaseProgram(clHandles.program[0]);

  if(clBuffers.output) {
    cout << "Release output memory object" << endl;
    clReleaseMemObject(clBuffers.output);
  }

  if (oData) {
    cout << "Freeing oData" << endl;
    free(oData);
  }
  if(iData) {
    cout << "Unmapping iData memory object" << endl;
    clEnqueueUnmapMemObject(clHandles.queue[0], clBuffers.input_pinned, (void*)iData, 0, NULL, NULL);
  }

  if(clBuffers.input) {
    cout << "Release input memory object" << endl;
    clReleaseMemObject(clBuffers.input);
  }

  if (clBuffers.input_pinned) {
    cout << "Release pinned memory object" << endl;
    clReleaseMemObject(clBuffers.input_pinned);
  }

  if(clHandles.queue[0]) {
    cout << "Release command queue" << endl;
    clReleaseCommandQueue(clHandles.queue[0]);
  }
  if(clHandles.context) {
    cout << "Release context" << endl;
    clReleaseContext(clHandles.context);
  }
  if(clHandles.devices) {
    cout << "Release devices" << endl;
    free(clHandles.devices);
  }

  fprintf(stdout, "Cleaned up\n");
}

bool CreateKernelFromFile(const char* filename, const char* kernelname, cl_handles_t& clHandles, size_t i) {

  cl_uint resultCL = CL_SUCCESS;
  char* source = NULL;
  char* builderr = NULL;

  bool retval = true;

  while (true) {
    source = file_to_string(filename);
    if (!source) {
      retval = false;
      break;
    }

    size_t sourceSize[] = { strlen(source) };

    CallClErr(clHandles.program[i] = clCreateProgramWithSource,
	      (clHandles.context,
	       1,
	       (const char**)&source,
	       sourceSize,
	       &clerrno));

    if (!clHandles.program[i]) {
      retval = false;
      break;
    }

    CallClErr(resultCL = clBuildProgram,
	      (clHandles.program[i], 1, clHandles.devices, NULL, NULL, NULL));

    if ((resultCL != CL_SUCCESS) || (clHandles.program[i] == NULL)) {
      fprintf(stderr, "FAILED: clBuildProgram(clerrno=%d strclerror=%s)\n",
	      resultCL, strclerror(resultCL));

      retval = false;

      size_t length;
      CallClErr(resultCL = clGetProgramBuildInfo,
		(clHandles.program[i],
		 clHandles.devices[0],
		 CL_PROGRAM_BUILD_LOG,
		 0,
		 NULL,
		 &length));
      if (resultCL != CL_SUCCESS)
	break;

      builderr = (char*)malloc(length);
      CallClErr(resultCL = clGetProgramBuildInfo,
		(clHandles.program[i],
		 clHandles.devices[0],
		 CL_PROGRAM_BUILD_LOG,
		 length,
		 builderr,
		 NULL));
      if (resultCL != CL_SUCCESS)
	break;
      fprintf(stderr, "%s\n", builderr);
    }

    CallClErr(clHandles.kernel[i] = clCreateKernel,
		  (clHandles.program[i],
		   kernelname,
		   &clerrno));
    if (clHandles.kernel[i] == NULL)
      retval = false;

    break;
  }

  if (source) free(source);
  if (builderr) free(builderr);

  return retval;
}


static char* file_to_string(const char *fileName) {
  size_t numBytes, numRead;
  struct stat fileInfo;
  char *contents;
  FILE *file;

  CallErrExit(stat, (fileName, &fileInfo), NULL);
  CallErrExit(file = fopen, (fileName, "rb"),NULL);

  numBytes = fileInfo.st_size + 1;

  if ((contents = (char *) malloc(numBytes)) == NULL) {
    fprintf(stderr, "file_to_string: Unable to allocate %zu bytes!\n", numBytes);
    return NULL;
  }

  if ((numRead = fread(contents,
                       1, fileInfo.st_size, file)) != (size_t)fileInfo.st_size) {
    fprintf(stderr, "file_to_string: Expected %zu bytes, but only read %zu!\n",
            numBytes, numRead);
  }
  contents[numRead] = '\0';
  return contents;
}

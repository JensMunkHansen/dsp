#include <ocl/utils.h>
#include <gtest/gtest.h>

static void Cleanup ();

static void (*pCleanup)() = &Cleanup;

void fillRandRange(const uint32_t len,
                   const float lower,
                   const float upper, float* pArr);

typedef enum oclPlatformVendor {
  oclPlatformVendorINTEL  = 0x01,
  oclPlatformVendorAMD    = 0x02,
  oclPlatformVendorNVIDIA = 0x03
} oclPlatformVendor;

static struct { oclPlatformVendor vendor; const char *strName; } oclPlatformVendorNameTable[] = {
  { oclPlatformVendorINTEL, "Intel(R) Corporation"},
  { oclPlatformVendorAMD,   "NVIDIA Corporation"},
  { oclPlatformVendorAMD,   "Advanced Micro Devices, Inc"}
};

int fun() {
  cl_uint numPlatforms;
  CallClErrReturn(clGetPlatformIDs,
                (0,
                 NULL,
                 &numPlatforms), EXIT_FAILURE, pCleanup);
  printf("Number of platforms: %d\n", numPlatforms);

  cl_platform_id* allPlatforms = NULL;

  allPlatforms = (cl_platform_id*) malloc(numPlatforms * sizeof(cl_platform_id));

  CallClErrReturn(clGetPlatformIDs,
                (numPlatforms,
                 allPlatforms,
                 NULL),
                EXIT_FAILURE, pCleanup);

  cl_platform_id targetPlatform = NULL;

  /* Select the target platform */
  targetPlatform = allPlatforms[0];

  // Find NVIDIA platform
  for (cl_uint i = 0; i < numPlatforms; i++) {
    char pbuff[128];
    CallClErrReturn(clGetPlatformInfo,
                  (allPlatforms[i],
                   CL_PLATFORM_VENDOR,
                   sizeof(pbuff),
                   pbuff,
                   NULL),
                  EXIT_FAILURE, pCleanup);

    // MSVC does not support %zu for C code, it is C99
    printf("%lu: %s\n", i, pbuff);

    // "Intel(R) Corporation"
    // "NVIDIA Corporation"
    // "Advanced Micro Devices, Inc"

    if (!strcmp(pbuff, "NVIDIA Corporation")) {
      targetPlatform = allPlatforms[i];
      break;
    }
  }

  cl_context_properties cprops[3] =
      { CL_CONTEXT_PLATFORM, (cl_context_properties)targetPlatform, 0 };

  cl_context context;

  // Select GPU device
  CallClErrReturn(context = clCreateContextFromType,
                (cprops,
                 CL_DEVICE_TYPE_GPU,
                 NULL,
                 NULL,
                 &clerrno),
                EXIT_FAILURE, pCleanup);

  size_t deviceListSize;
  CallClErrReturn(clGetContextInfo,
                (context,
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

  size_t ndevices = deviceListSize / sizeof(cl_device_id);

  /* Now, allocate the device list */
  cl_device_id* devices = (cl_device_id *)malloc(deviceListSize);

  if (devices == 0) {
    Cleanup();
    return EXIT_FAILURE;
  }

  /* Next, get the device list data */
  CallClErrReturn(clGetContextInfo,
                (context,
                 CL_CONTEXT_DEVICES,
                 deviceListSize,
                 devices,
                 NULL),
                EXIT_FAILURE, pCleanup);

  return EXIT_SUCCESS;
}

TEST(sliding_test, reference) {
  int retval = fun();
}

void Cleanup () {

}

int main(int argc, char* argv[]) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

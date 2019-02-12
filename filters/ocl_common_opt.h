#include <cstdarg>

#define __STDC_FORMAT_MACROS 1 // Necessary for __cplusplus and -std=c++0x
#include <inttypes.h>

#include "CL/cl.h"

typedef struct cl_handles
{
  cl_context              context;
  cl_uint                 ndevices;
  cl_device_id            *devices;
  cl_command_queue        queue1;
  cl_command_queue        queue2;
  cl_program              program1;
  cl_program              program2;
  cl_kernel               kernel1;
  cl_kernel               kernel2;
} cl_handles_t;

typedef struct cl_buffers {
  cl_mem  iinput;
  cl_mem  qinput;
  cl_mem  ioutput;
  cl_mem  qoutput;
  cl_mem  debug_output;
} cl_buffers_t;

/* Clean-up handler: Calls argument with prototype void (*)() if present */
#ifdef __GNUC__
# define NUMARGS(...)  (sizeof(( void (*[])() ){0, ##__VA_ARGS__})/sizeof(void (*)())-1)
# define CLEANUP(...)  cleanup(NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#else
# define NUMARGS(...)  (sizeof(( void (*[])() ){__VA_ARGS__})/sizeof(void (*)()))
# define CLEANUP(...)  (cleanup(NUMARGS(__VA_ARGS__), __VA_ARGS__))
#endif

void cleanup(int numargs, ...) {
  if (numargs == 0)
    return;
  
  void (*pCleanup)();
  
  va_list ap;
  va_start(ap, numargs);

  if (numargs--)
    pCleanup = va_arg(ap, void (*)());
  va_end(ap);
  
  pCleanup();
  
  return;
}

/*
   Calling macros for OpenCL functions using clerrno: 

     CallClErr(function, (arguments))
     CallClErrExit(function, (arguments), return value)
     CallClErrExit(function, (arguments), return value, void (clean*)())

*/

/// Global errno variable
cl_int clerrno = CL_SUCCESS;

/** 
 * strclerror
 * 
 * @param status 
 * 
 * @return 
 */
static const char *
strclerror(cl_int status) {
  static struct { cl_int code; const char *msg; } error_table[] = {
    { CL_SUCCESS, "success" },
    { CL_DEVICE_NOT_FOUND, "device not found", },
    { CL_DEVICE_NOT_AVAILABLE, "device not available", },
    { CL_COMPILER_NOT_AVAILABLE, "compiler not available", },
    { CL_MEM_OBJECT_ALLOCATION_FAILURE, "mem object allocation failure", },
    { CL_OUT_OF_RESOURCES, "out of resources", },
    { CL_OUT_OF_HOST_MEMORY, "out of host memory", },
    { CL_PROFILING_INFO_NOT_AVAILABLE, "profiling not available", },
    { CL_MEM_COPY_OVERLAP, "memcopy overlaps", },
    { CL_IMAGE_FORMAT_MISMATCH, "image format mismatch", },
    { CL_IMAGE_FORMAT_NOT_SUPPORTED, "image format not supported", },
    { CL_BUILD_PROGRAM_FAILURE, "build program failed", },
    { CL_MAP_FAILURE, "map failed", },
    { CL_INVALID_VALUE, "invalid value", },
    { CL_INVALID_DEVICE_TYPE, "invalid device type", },
    { 0, NULL },
  };

  static char unknown[25];
  size_t i;

  for (i = 0; error_table[i].msg != NULL; i++) {
    if (error_table[i].code == status) {
      return error_table[i].msg;
    }
  }

  snprintf(unknown, sizeof(unknown), "unknown error %d", status);
  return unknown;
}

#define CallClErr(fun, arg) {                                     \
  intptr_t clerr64 = (intptr_t)(fun arg);			  \
  cl_int clerr     = (cl_int) clerr64;                            \
  if ((clerr == 0) && (clerrno != CL_SUCCESS))	  \
    FailClErr(#fun, clerrno)                                      \
  else if (clerr != CL_SUCCESS)                                   \
    FailClErr(#fun, clerr)                                        \
  clerrno = CL_SUCCESS;                                           \
}

#define CallClErrExit(fun, arg, ret, ...) {                       \
  intptr_t clerr64 = (intptr_t)(fun arg);			  \
  cl_int clerr     = (cl_int) clerr64;                            \
  if ((clerr == 0) && (clerrno != CL_SUCCESS)) {                  \
    FailClErr(#fun, clerrno)                                      \
    CLEANUP(__VA_ARGS__);                                         \
    return ret;                                                   \
  }                                                               \
  else if ((clerr != CL_SUCCESS) && (clerrno != CL_SUCCESS)) {    \
    FailClErr(#fun, clerr)                                        \
    CLEANUP(__VA_ARGS__);                                         \
    return ret;                                                   \
  }                                                               \
  clerrno = CL_SUCCESS;                                           \
}

#define FailClErr(msg,clerrno) {                                  \
  (void)fprintf(stderr, "FAILED: %s(clerrno=%d strclerror=%s)\n", \
                msg, clerrno, strclerror(clerrno));               \
  (void)fflush(stderr);                                           \
}

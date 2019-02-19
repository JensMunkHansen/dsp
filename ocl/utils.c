#include <ocl/utils.h>

#ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS 1
#endif

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

// stdLLufstrean in(".cl");
// std::string str(static_cast<std::stringstream const&>(std::stringstream() << in.rdbuf()).str());
/*
string readFile2(const string &fileName)
{
  ifstream ifs(fileName.c_str(), ios::in | ios::binary | ios::ate);

  ifstream::pos_type fileSize = ifs.tellg();
  ifs.seekg(0, ios::beg);

  vector<char> bytes(fileSize);
  ifs.read(bytes.data(), fileSize);

  return string(bytes.data(), fileSize);
}

However, the concept behind fast POSIX file to memory usage involves the following:
Direct file access functions (the classic open/read/close).
Best alignment of data in memory and multiple of the file's blocking factor (posix_memalign and fstat with st_blksize).
Informing the OS of I/O strategy (posix_fadvise and posix_madvise).


*/

char* slurp_file(const char *filename) {
  char *s = NULL;
  FILE *f = fopen(filename, "rb");
  if (f != NULL) {
    fseek(f, 0, SEEK_END);
    size_t src_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    s = (char*) malloc(src_size + 1);
    if (fread(s, 1, src_size, f) != src_size) {
      free(s);
      s = NULL;
    } else {
      s[src_size] = '\0';
    }
    fclose(f);
  }
  return s;
}

cl_program opencl_build_program(cl_context ctx, cl_device_id device,
                                const char *file, const char *options_fmt, ...) {
  cl_int error;
  const char *kernel_src = slurp_file(file);
  if (kernel_src == NULL) {
    fprintf(stderr, "Cannot open %s: %s\n", file, strerror(errno));
    abort();
  }
  size_t src_len = strlen(kernel_src);

  cl_program program = clCreateProgramWithSource(ctx, 1, &kernel_src, &src_len, &error);
  // OPENCL_SUCCEED(error);

  // Construct the actual options string, which involves some varargs
  // magic.
  va_list vl;
  va_start(vl, options_fmt);
  size_t needed = 1 + vsnprintf(NULL, 0, options_fmt, vl);
  char *options = malloc(needed);
  va_start(vl, options_fmt); /* Must re-init. */
  vsnprintf(options, needed, options_fmt, vl);

  // Here we are a bit more generous than usual and do not terminate
  // the process immediately on a build error.  Instead, we print the
  // error messages first.
  error = clBuildProgram(program, 1, &device, options, NULL, NULL);
  if (error != CL_SUCCESS && error != CL_BUILD_PROGRAM_FAILURE) {
    // OPENCL_SUCCEED(error);
  }
  free(options);

  cl_build_status build_status;
  CallClErr(clGetProgramBuildInfo,
            (program,
             device,
             CL_PROGRAM_BUILD_STATUS,
             sizeof(cl_build_status),
             &build_status,
             NULL));

  if (build_status != CL_SUCCESS) {
    char *build_log;
    size_t ret_val_size;
    CallClErr(
        clGetProgramBuildInfo,
        (program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &ret_val_size));

    build_log = (char*) malloc(ret_val_size+1);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, ret_val_size, build_log, NULL);

    // The spec technically does not say whether the build log is
    // zero-terminated, so let's be careful.
    build_log[ret_val_size] = '\0';

    fprintf(stderr, "Build log:\n%s\n", build_log);

    free(build_log);

    ClErrExit(build_status)
  }

  return program;
}

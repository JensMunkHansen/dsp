include(GenerateExportHeader)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config.h.in" "${CMAKE_CURRENT_BINARY_DIR}/config.h" @ONLY)

set(LIB_TYPE STATIC)

set(ocl_SOURCES
  utils.c
)

find_package( OpenCL )

add_library(ocl ${LIB_TYPE} ${ocl_SOURCES})

include_directories(${OpenCL_INCLUDE_DIRS})


target_link_libraries(ocl ${OpenCL_LIBRARY})

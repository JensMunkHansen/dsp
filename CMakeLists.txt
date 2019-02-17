cmake_minimum_required(VERSION 2.8)

include_directories(
  "${PROJECT_SOURCE_DIR}"   # to find fnm/fnm_types.hpp
  "${PROJECT_BINARY_DIR}"   # to find fnm/config.h
  )

add_subdirectory(ocl)

add_subdirectory(filters)

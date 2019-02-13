cmake_minimum_required(VERSION 2.8)

if (WIN32)
  set(GTEST_ROOT "C:/Program Files/googletest-distribution")
endif()

find_package( GTest REQUIRED )
find_package( Threads)

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(sliding_test sliding_test.cpp)

target_link_libraries(sliding_test ${GTEST_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
enable_testing()

add_test(NAME    sliding_test
  COMMAND sliding_test)

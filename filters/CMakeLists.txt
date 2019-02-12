cmake_minimum_required(VERSION 2.8)

find_package( GTest REQUIRED )

include_directories(${GTEST_INCLUDE_DIRS})

add_executable(sliding_test sliding_test.cpp)

target_link_libraries(sliding_test ${GTEST_LIBRARIES} pthread)
enable_testing()

add_test(NAME    sliding_test
  COMMAND sliding_test)

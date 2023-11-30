cmake_minimum_required (VERSION 3.4)

project(tbb_tutorials LANGUAGES CXX)

set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_LINKER "icpx")
set(CMAKE_CXX_STANDARD 20)

include (CTest)

if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(algorithms)
endif (BUILD_TESTING)


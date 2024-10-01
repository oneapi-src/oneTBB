cmake_minimum_required (VERSION 3.4)

project(tbb_tutorials LANGUAGES CXX)

set(CMAKE_CXX_COMPILER "icpx")
set(CMAKE_LINKER "icpx")
set(CMAKE_CXX_STANDARD 20)

include (CTest)

if (BUILD_TESTING)
  enable_testing()
  add_subdirectory(algorithms)
  add_subdirectory(cancellation)
  add_subdirectory(exception)
  add_subdirectory(containers)
  add_subdirectory(graph)
  add_subdirectory(intro)
  add_subdirectory(memalloc)
  add_subdirectory(migration)
  add_subdirectory(performance_tuning)
  add_subdirectory(synchronization)
endif (BUILD_TESTING)


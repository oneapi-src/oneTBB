set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb")
set(CMAKE_CXX_LINKER_FLAGS "-tbb")

foreach(tpp parallel_invoke_recursive_quicksort.cpp parallel_invoke_two_quicksorts.cpp 
            parallel_for_trivial.cpp parallel_for_unoptimized_mxm.cpp
            parallel_reduce_max.cpp parallel_reduce_pi.cpp
            parallel_scan_running_sum.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()


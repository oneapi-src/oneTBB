foreach(tpp parallel_invoke_recursive_quicksort.cpp parallel_invoke_two_quicksorts.cpp 
            parallel_for_trivial.cpp parallel_for_unoptimized_mxm.cpp
            parallel_reduce_max.cpp parallel_reduce_pi.cpp
            parallel_scan_running_sum.cpp parallel_scan_line_of_sight.cpp
            parallel_for_each_primes.cpp parallel_for_each_fwd_substition.cpp
            parallel_pipeline_case.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()


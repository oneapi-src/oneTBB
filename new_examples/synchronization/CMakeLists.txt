set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb")
set(CMAKE_CXX_LINKER_FLAGS "-tbb")

foreach(tpp histogram_01_sequential.cpp
    histogram_02_unsafe_parallel.cpp
    histogram_03_1st_safe_parallel.cpp
    histogram_04_2nd_safe_parallel.cpp
    histogram_05_fetch_and_triple.cpp
    histogram_06_3rd_safe_parallel.cpp
    histogram_07_3rd_safe_parallel_cache_aligned.cpp
    histogram_08_4th_safe_parallel_private.cpp
    histogram_09_5th_safe_parallel_private.cpp
    histogram_10_6th_safe_parallel_private.cpp
    histogram_11_7th_safe_parallel_combine.cpp
    histogram_12_8th_safe_parallel_combine.cpp
    histogram_13_9th_safe_parallel_reduction.cpp
    histogram_14_10th_safe_parallel_reduction.cpp
    laurel_and_hardy.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()

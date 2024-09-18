set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb")
set(CMAKE_CXX_LINKER_FLAGS "-tbb")

foreach(tpp intro_pi.cpp
            intro_pi_timing.cpp
            intro_flowgraph.cpp
            intro_gamma.cpp
            intro_helloPSTL.cpp
            intro_helloTBB.cpp
	    intro_parallel_for.cpp
            intro_parallel_for_transform.cpp
	    intro_sidebar_functor.cpp
            intro_sidebar_lambda.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()


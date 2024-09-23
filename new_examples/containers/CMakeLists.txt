set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb")
set(CMAKE_CXX_LINKER_FLAGS "-tbb")

foreach(tpp concurrent_hash_maps.cpp
        concurrent_maps.cpp
	concurrent_queues.cpp
	concurrent_vectors.cpp
	debugging_queue.cpp
	pop_danger.cpp
	pop_danger_fixed.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()

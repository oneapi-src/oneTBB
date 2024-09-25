set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb -ltbbmalloc")
set(CMAKE_CXX_LINKER_FLAGS "-tbb -ltbbmalloc")

foreach(tpp tbb_mem.cpp
        replace_new_n_delete.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()

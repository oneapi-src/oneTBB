find_package(TBB)

foreach(tpp tbb_mem.cpp windows_proxy.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20)
  target_link_libraries(${texe} TBB::tbbmalloc)
  add_test(${texe} ${texe})
endforeach()
add_executable(replace_new_n_delete replace_new_n_delete.cpp)

target_link_libraries(replace_new_n_delete TBB::tbbmalloc)
set_property(TARGET replace_new_n_delete PROPERTY CXX_STANDARD 20) 
add_test(replace_new_n_delete replace_new_n_delete)

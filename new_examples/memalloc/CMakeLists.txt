find_package(TBB)

add_executable(tbb_mem tbb_mem.cpp) 
set_property(TARGET tbb_mem PROPERTY CXX_STANDARD 20)
target_link_libraries(tbb_mem TBB::tbbmalloc)
add_test(tbb_mem tbb_mem)

if (WIN32)
add_executable(windows_proxy windows_proxy.cpp)
target_link_libraries(windows_proxy TBB::tbbmalloc)
set_property(TARGET windows_proxy PROPERTY CXX_STANDARD 20) 
add_test(windows_proxy windows_proxy)
endif()

add_executable(replace_new_n_delete replace_new_n_delete.cpp)
target_link_libraries(replace_new_n_delete TBB::tbbmalloc)
set_property(TARGET replace_new_n_delete PROPERTY CXX_STANDARD 20) 
add_test(replace_new_n_delete replace_new_n_delete)

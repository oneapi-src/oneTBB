set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -tbb -pthread")
set(CMAKE_CXX_LINKER_FLAGS "-tbb -pthread")

foreach(tpp parallel_invoke_fib.cpp
            task_group_fib.cpp
            task_group_poor_scaling.cpp
            task_group_fib_defer.cpp
            task_group_with_dependencies.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()

add_executable(resumable_tasks resumable_tasks.cpp)
target_compile_options(resumable_tasks PUBLIC "-fsycl")
target_link_options(resumable_tasks PUBLIC "-fsycl")
target_include_directories(resumable_tasks PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>)   
set_property(TARGET resumable_tasks PROPERTY CXX_STANDARD 20) 
add_test(resumable_tasks resumable_tasks)


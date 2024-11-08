set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Qtbb")
set(CMAKE_CXX_LINKER_FLAGS "-Qtbb")

foreach(tpp global_control_and_implicit_arena.cpp 
            global_control_and_explicit_arena.cpp 
            global_control_and_implicit_conflict.cpp 
            global_control_and_explicit_conflict.cpp 
            priorities_and_conflict.cpp 
            blocked_ranges_trivial.cpp
            constraints.cpp
            parallel_for_spin_partitioners.cpp
            parallel_for_spin_partitioners_timed.cpp
            parallel_for_addition_partitioners.cpp
            parallel_for_transpose_partitioners.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
  )   
  target_link_libraries(${texe} ${CMAKE_THREAD_LIBS_INIT})
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()


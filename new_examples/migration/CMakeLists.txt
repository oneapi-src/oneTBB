foreach(tpp migrate_atomics.cpp
            migrate_task_scheduler_init.cpp
            migrate_parallel_do.cpp
            migrate_priorities.cpp
            migrate_task_blocking.cpp
            migrate_tasks_adding_work.cpp
            migrate_recycling_tasks.cpp
            migrate_bypass_tasks.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
  )   
  target_link_libraries(${texe} ${CMAKE_THREAD_LIBS_INIT})
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()


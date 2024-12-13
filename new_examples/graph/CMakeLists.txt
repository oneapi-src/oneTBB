foreach(tpp graph_composite_node.cpp
            graph_execute_while_building.cpp
            graph_fwd_substitution.cpp
            graph_limiting_messages.cpp
            graph_loops.cpp
            graph_node_priorities.cpp
            graph_reestablish_order.cpp
            graph_small_nodes.cpp
            graph_stereoscopic_3d.cpp
            graph_two_nodes.cpp 
            graph_two_nodes_deduced.cpp
            graph_with_join.cpp)
  string(REPLACE ".cpp" "" texe ${tpp})
  add_executable(${texe} ${tpp})
  target_include_directories(${texe} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>
    )   
  set_property(TARGET ${texe} PROPERTY CXX_STANDARD 20) 
  add_test(${texe} ${texe})
endforeach()

add_executable(graph_async_sycl graph_async_sycl.cpp)
target_compile_options(graph_async_sycl PUBLIC "-fsycl")
target_link_options(graph_async_sycl PUBLIC "-fsycl")
target_include_directories(graph_async_sycl PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../common>)   
set_property(TARGET graph_async_sycl PROPERTY CXX_STANDARD 20) 
add_test(graph_async_sycl graph_async_sycl)


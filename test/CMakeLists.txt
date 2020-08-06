# Copyright (c) 2020 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# General function for test target generation
function(tbb_add_test)
    set(oneValueArgs SUBDIR NAME)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments(_tbb_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Define the target for test

    add_executable(${_tbb_test_NAME} ${_tbb_test_SUBDIR}/${_tbb_test_NAME}.cpp)
    target_include_directories(${_tbb_test_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR})

    target_compile_options(${_tbb_test_NAME}
        PRIVATE
        ${TBB_CXX_STD_FLAG}
        ${TBB_WARNING_LEVEL}
        ${TBB_TEST_WARNING_FLAGS}
        ${TBB_WARNING_SUPPRESS}
        ${TBB_TEST_COMPILE_FLAGS}
        ${TBB_COMMON_COMPILE_FLAGS}
    )

    if (ANDROID_PLATFORM)
        target_link_libraries(${_tbb_test_NAME} PRIVATE -rdynamic) # for the test_dynamic_link
        add_test(NAME ${_tbb_test_NAME}
                 COMMAND ${CMAKE_COMMAND}
                         -DBINARIES_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                         -DTEST_NAME=${_tbb_test_NAME}
                         -P ${PROJECT_SOURCE_DIR}/cmake/android/test_launcher.cmake)
    else()
        add_test(NAME ${_tbb_test_NAME} COMMAND ${_tbb_test_NAME} --force-colors=1 WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()

    # Prefer using target_link_options instead of target_link_libraries to specify link options because
    # target_link_libraries may incorrectly handle some options (on Windows, for example).
    if (COMMAND target_link_options)
        target_link_options(${_tbb_test_NAME} PRIVATE ${TBB_COMMON_LINK_FLAGS})
    else()
        target_link_libraries(${_tbb_test_NAME} PRIVATE ${TBB_COMMON_LINK_FLAGS})
    endif()

    target_compile_definitions(${_tbb_test_NAME} PRIVATE
        $<$<CONFIG:DEBUG>:TBB_USE_DEBUG>
        $<$<BOOL:${TBB_CPF}>:__TBB_CPF_BUILD=1>)

    target_link_libraries(${_tbb_test_NAME} PRIVATE ${_tbb_test_DEPENDENCIES} Threads::Threads ${TBB_COMMON_LINK_LIBS})
endfunction()

# Function for C test target generation
function(tbb_add_c_test)
    set(oneValueArgs SUBDIR NAME)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments(_tbb_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Define the target for test

    add_executable(${_tbb_test_NAME} ${_tbb_test_SUBDIR}/${_tbb_test_NAME}.c)
    target_include_directories(${_tbb_test_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR})

    if (ANDROID_PLATFORM)
        target_link_libraries(${_tbb_test_NAME} PRIVATE -rdynamic) # for the test_dynamic_link
        add_test(NAME ${_tbb_test_NAME}
                COMMAND ${CMAKE_COMMAND}
                        -DBINARIES_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                        -DTEST_NAME=${_tbb_test_NAME}
                        -P ${PROJECT_SOURCE_DIR}/cmake/android/test_launcher.cmake)
    else()
        add_test(NAME ${_tbb_test_NAME} COMMAND ${_tbb_test_NAME} --force-colors=1 WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    endif()

    target_compile_definitions(${_tbb_test_NAME} PRIVATE
        $<$<CONFIG:DEBUG>:TBB_USE_DEBUG>
        $<$<BOOL:${TBB_CPF}>:__TBB_CPF_BUILD=1>)

    target_link_libraries(${_tbb_test_NAME} PRIVATE ${_tbb_test_DEPENDENCIES} Threads::Threads)
endfunction()

# Function for lib test target generation
function(tbb_add_lib_test)
    set(oneValueArgs SUBDIR NAME)
    set(multiValueArgs DEPENDENCIES)
    cmake_parse_arguments(_tbb_test "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_library(_${_tbb_test_NAME} SHARED ${_tbb_test_SUBDIR}/${_tbb_test_NAME}.cpp)

    target_include_directories(_${_tbb_test_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR})

    # TODO: fix warnings
    if (MSVC)
        # signed unsigned mismatch, declaration hides class member
        set(TBB_WARNING_SUPPRESS ${TBB_WARNING_SUPPRESS} /wd4267 /wd4244 /wd4245 /wd4018 /wd4458)
    endif()

    set(TEST_LIB_COMPILE_FLAGS -D_USRDLL)
    # TODO: add ${TBB_WARNING_LEVEL} and fix problems
    target_compile_options(_${_tbb_test_NAME}
        PRIVATE
        ${TBB_CXX_STD_FLAG} # TODO: consider making it PUBLIC.
        ${TBB_MMD_FLAG}
        ${TBB_DSE_FLAG}
        ${TBB_WARNING_SUPPRESS}
        ${TBB_LIB_COMPILE_FLAGS}
        ${TBBMALLOC_LIB_COMPILE_FLAGS}
        ${TBB_COMMON_COMPILE_FLAGS}
        ${TEST_LIB_COMPILE_FLAGS}
    )

    target_compile_definitions(_${_tbb_test_NAME} PRIVATE
        $<$<CONFIG:DEBUG>:TBB_USE_DEBUG>
        $<$<BOOL:${TBB_CPF}>:__TBB_CPF_BUILD=1>)

    # Prefer using target_link_options instead of target_link_libraries to specify link options because
    # target_link_libraries may incorrectly handle some options (on Windows, for example).
    if (COMMAND target_link_options)
        target_link_options(_${_tbb_test_NAME}
            PRIVATE
            ${TBB_LIB_LINK_FLAGS}
            ${TBB_COMMON_LINK_FLAGS}
        )
    else()
        target_link_libraries(_${_tbb_test_NAME}
            PRIVATE
            ${TBB_LIB_LINK_FLAGS}
            ${TBB_COMMON_LINK_FLAGS}
        )
    endif()

    target_link_libraries(_${_tbb_test_NAME}
        PRIVATE
        Threads::Threads
        ${_tbb_test_DEPENDENCIES}
        ${TBB_LIB_LINK_LIBS}
        ${TBB_COMMON_LINK_LIBS}
    )
endfunction()

# Copy libraries to test folder to make it visible during tests execution if external TBB is tested.
if (TBB_FOUND)
    get_property(TBB_LIB_FILE_LOCATION TARGET TBB::tbb PROPERTY LOCATION)
    get_filename_component(TBB_LIB_FILE_NAME ${TBB_LIB_FILE_LOCATION} NAME)
    file(COPY ${TBB_LIB_FILE_LOCATION} DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}) # TODO: check and update for multi-config generators.
endif()

# Define the tests
tbb_add_test(SUBDIR tbb NAME test_tick_count DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_allocators DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_arena_constraints DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_arena_priorities DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_dynamic_link DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_lru_cache DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_unordered_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_unordered_set DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_set DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_priority_queue DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_for DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_for_each DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_reduce DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_sort DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_invoke DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_scan DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_parallel_pipeline DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_eh_algorithms DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_blocked_range DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_vector DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_task_group DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_hash_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_task_arena DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_enumerable_thread_specific DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_queue DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_resumable_tasks DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_mutex DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_function_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_multifunction_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_broadcast_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_buffer_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_composite_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_continue_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_eh_flow_graph DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_flow_graph DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_flow_graph_priorities DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_flow_graph_whitebox DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_indexer_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_join_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_join_node_key_matching DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_join_node_msg_key_matching DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_limiter_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_priority_queue_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_queue_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_sequencer_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_split_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_tagged_msg DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_overwrite_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_write_once_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_async_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_input_node DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_profiling DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_concurrent_queue_whitebox DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_intrusive_list DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_semaphore DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_environment_whitebox DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_handle_perror DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR tbb NAME test_hw_concurrency DEPENDENCIES TBB::tbb)

tbb_add_test(SUBDIR tbb NAME test_tbb_header DEPENDENCIES TBB::tbb)
target_sources(test_tbb_header PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tbb/test_tbb_header_secondary.cpp)

# Define the conformance tests
tbb_add_test(SUBDIR conformance NAME conformance_tick_count DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_allocators DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_arena_constraints DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_mutex DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_task_group_context DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_task_arena DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_lru_cache DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_unordered_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_unordered_set DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_set DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_priority_queue DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_for DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_for_each DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_reduce DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_scan DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_sort DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_pipeline DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_parallel_invoke DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_blocked_range DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_blocked_range2d DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_blocked_range3d DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_blocked_rangeNd DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_vector DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_global_control DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_hash_map DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_enumerable_thread_specific DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_combinable DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_concurrent_queue DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_resumable_tasks DEPENDENCIES TBB::tbb)
tbb_add_test(SUBDIR conformance NAME conformance_version DEPENDENCIES TBB::tbb)

if (TBB_NUMA_SUPPORT)
    find_package(HWLOC QUIET)
    if (HWLOC_FOUND)
        target_link_libraries(test_arena_constraints PRIVATE ${HWLOC_LIBRARIES})
        target_include_directories(test_arena_constraints PRIVATE ${HWLOC_INCLUDE_DIRS})
        target_compile_definitions(test_arena_constraints PRIVATE __TBB_HWLOC_PRESENT)
        add_dependencies(test_arena_constraints TBB::tbbbind)

        target_link_libraries(conformance_arena_constraints PRIVATE ${HWLOC_LIBRARIES})
        target_include_directories(conformance_arena_constraints PRIVATE ${HWLOC_INCLUDE_DIRS})
        target_compile_definitions(conformance_arena_constraints PRIVATE __TBB_HWLOC_PRESENT)
        add_dependencies(conformance_arena_constraints TBB::tbbbind)
    endif()
endif()

# TBB allocator tests
if (NOT "${CMAKE_SYSTEM_PROCESSOR}" MATCHES "mips")
    # Define TBB malloc tests
    tbb_add_test(SUBDIR tbbmalloc NAME test_scalable_allocator DEPENDENCIES TBB::tbbmalloc)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_pools DEPENDENCIES TBB::tbbmalloc)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_init_shutdown DEPENDENCIES TBB::tbbmalloc)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_regression DEPENDENCIES TBB::tbbmalloc)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_shutdown_hang DEPENDENCIES TBB::tbb TBB::tbbmalloc)

    if (NOT (WINDOWS_STORE OR TBB_WINDOWS_DRIVER))
        # TODO: Consider adding following tests on WINDOWS_STORE and TBB_WINDOWS_DRIVER platforms
        tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_compliance DEPENDENCIES TBB::tbbmalloc)
        tbb_add_lib_test(SUBDIR tbbmalloc NAME test_malloc_used_by_lib DEPENDENCIES TBB::tbbmalloc)
        tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_used_by_lib DEPENDENCIES _test_malloc_used_by_lib)
        tbb_add_lib_test(SUBDIR tbbmalloc NAME test_malloc_lib_unload)
        tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_lib_unload DEPENDENCIES _test_malloc_lib_unload)
    endif()

    enable_language(C)
    tbb_add_c_test(SUBDIR tbbmalloc NAME test_malloc_pure_c DEPENDENCIES TBB::tbbmalloc)

    # ----------------------------------------------------------------------------------------
    # Whitebox testing

    add_executable(test_malloc_whitebox tbbmalloc/test_malloc_whitebox.cpp)

    target_include_directories(test_malloc_whitebox
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR})
    target_compile_definitions(test_malloc_whitebox PRIVATE __TBBMALLOC_BUILD)
    target_compile_options(test_malloc_whitebox
        PRIVATE
        ${TBB_CXX_STD_FLAG}
        ${TBB_WARNING_SUPPRESS}
        ${TBB_TEST_COMPILE_FLAGS}
        ${TBB_COMMON_COMPILE_FLAGS}
        ${TBBMALLOC_LIB_COMPILE_FLAGS}
    )
    if (ANDROID_PLATFORM)
        target_link_libraries(test_malloc_whitebox PRIVATE -rdynamic) # for the test_dynamic_link
        add_test(NAME test_malloc_whitebox
                 COMMAND ${CMAKE_COMMAND}
                         -DBINARIES_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
                         -DTEST_NAME=test_malloc_whitebox
                         -P ${PROJECT_SOURCE_DIR}/cmake/android/test_launcher.cmake)
    else()
        add_test(NAME test_malloc_whitebox COMMAND test_malloc_whitebox --force-colors=1)
    endif()
    if (COMMAND target_link_options)
        target_link_options(test_malloc_whitebox PRIVATE ${TBB_COMMON_LINK_FLAGS})
    else()
        target_link_libraries(test_malloc_whitebox PRIVATE ${TBB_COMMON_LINK_FLAGS})
    endif()
    target_link_libraries(test_malloc_whitebox PRIVATE Threads::Threads ${TBB_COMMON_LINK_LIBS})

    # ------------------------------------------------------------------------------------------

    # Define TBB malloc conformance tests
    # tbbmalloc_add_test(conformance conformance_scalable_allocator)

    if ("${CMAKE_MSVC_RUNTIME_LIBRARY}" STREQUAL MultiThreaded OR "${CMAKE_MSVC_RUNTIME_LIBRARY}" STREQUAL MultiThreadedDebug)
        if ("${CMAKE_MSVC_RUNTIME_LIBRARY}" STREQUAL MultiThreaded)
            set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)
        else()
            set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDebugDLL)
        endif()
    endif()

    tbb_add_lib_test(SUBDIR tbbmalloc NAME test_malloc_atexit DEPENDENCIES TBB::tbbmalloc_proxy TBB::tbbmalloc)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_atexit DEPENDENCIES TBB::tbbmalloc_proxy TBB::tbbmalloc _test_malloc_atexit)

    # Define TBB malloc proxy tests
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_overload DEPENDENCIES TBB::tbbmalloc_proxy)
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_overload_disable DEPENDENCIES TBB::tbbmalloc_proxy TBB::tbbmalloc) # safer_msize call need to be available
    tbb_add_test(SUBDIR tbbmalloc NAME test_malloc_new_handler DEPENDENCIES TBB::tbbmalloc_proxy)
endif()

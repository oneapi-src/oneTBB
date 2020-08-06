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

add_library(tbb SHARED
    allocator.cpp
    arena.cpp
    arena_slot.cpp
    concurrent_monitor.cpp
    concurrent_bounded_queue.cpp
    dynamic_link.cpp
    exception.cpp
    governor.cpp
    global_control.cpp
    itt_notify.cpp
    main.cpp
    market.cpp
    misc.cpp
    misc_ex.cpp
    observer_proxy.cpp
    parallel_pipeline.cpp
    private_server.cpp
    profiling.cpp
    rml_tbb.cpp
    rtm_mutex.cpp
    rtm_rw_mutex.cpp
    semaphore.cpp
    small_object_pool.cpp
    task.cpp
    task_dispatcher.cpp
    task_group_context.cpp
    version.cpp
    queuing_rw_mutex.cpp)

add_library(TBB::tbb ALIAS tbb)

# TODO: Add statistics.cpp

target_compile_definitions(tbb PRIVATE __TBB_BUILD)

if (NOT ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(armv7-a|aarch64|mips)" OR WINDOWS_STORE OR TBB_WINDOWS_DRIVER))
    target_compile_definitions(tbb PRIVATE __TBB_USE_ITT_NOTIFY)
endif()

target_include_directories(tbb
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>)

target_compile_options(tbb
    PRIVATE
    ${TBB_CXX_STD_FLAG} # TODO: consider making it PUBLIC.
    ${TBB_MMD_FLAG}
    ${TBB_DSE_FLAG}
    ${TBB_WARNING_LEVEL}
    ${TBB_WARNING_SUPPRESS}
    ${TBB_LIB_COMPILE_FLAGS}
    ${TBB_COMMON_COMPILE_FLAGS}
)

# Avoid use of target_link_libraries here as it changes /DEF option to \DEF on Windows.
set_target_properties(tbb PROPERTIES
    LINK_FLAGS ${TBB_LINK_DEF_FILE_FLAG}${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb.def
    DEFINE_SYMBOL ""
    VERSION ${TBB_BINARY_VERSION}
)

# Prefer using target_link_options instead of target_link_libraries to specify link options because
# target_link_libraries may incorrectly handle some options (on Windows, for example).
if (COMMAND target_link_options)
    target_link_options(tbb
        PRIVATE
        ${TBB_LIB_LINK_FLAGS}
        ${TBB_COMMON_LINK_FLAGS}
    )
else()
    target_link_libraries(tbb
        PRIVATE
        ${TBB_LIB_LINK_FLAGS}
        ${TBB_COMMON_LINK_FLAGS}
    )
endif()

target_link_libraries(tbb
    PRIVATE
    Threads::Threads
    ${TBB_LIB_LINK_LIBS}
    ${TBB_COMMON_LINK_LIBS}
)

install(TARGETS tbb
    EXPORT TBBTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

if (COMMAND tbb_gen_vars)
    tbb_gen_vars(tbb)
endif()

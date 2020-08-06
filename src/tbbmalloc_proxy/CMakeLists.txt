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

add_library(tbbmalloc_proxy SHARED
    function_replacement.cpp
    proxy.cpp)

add_library(TBB::tbbmalloc_proxy ALIAS tbbmalloc_proxy)

target_include_directories(tbbmalloc_proxy
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>)

target_compile_options(tbbmalloc_proxy
    PRIVATE
    ${TBB_CXX_STD_FLAG} # TODO: consider making it PUBLIC.
    ${TBB_MMD_FLAG}
    ${TBB_DSE_FLAG}
    ${TBB_WARNING_LEVEL}
    ${TBB_WARNING_SUPPRESS}
    ${TBB_LIB_COMPILE_FLAGS}
    ${TBB_COMMON_COMPILE_FLAGS}
)

set_target_properties(tbbmalloc_proxy PROPERTIES VERSION ${TBBMALLOC_BINARY_VERSION})

if (UNIX AND NOT APPLE)
    # Avoid use of target_link_libraries here as it changes /DEF option to \DEF on Windows.
    set_target_properties(tbbmalloc_proxy PROPERTIES
        LINK_FLAGS ${TBB_LINK_DEF_FILE_FLAG}${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-proxy.def
        DEFINE_SYMBOL "")
endif()

# Prefer using target_link_options instead of target_link_libraries to specify link options because
# target_link_libraries may incorrectly handle some options (on Windows, for example).
if (COMMAND target_link_options)
    target_link_options(tbbmalloc_proxy
        PRIVATE
        ${TBB_LIB_LINK_FLAGS}
        ${TBB_COMMON_LINK_FLAGS}
    )
else()
    target_link_libraries(tbbmalloc_proxy
        PRIVATE
        ${TBB_LIB_LINK_FLAGS}
        ${TBB_COMMON_LINK_FLAGS}
    )
endif()

target_link_libraries(tbbmalloc_proxy
    PRIVATE
    TBB::tbbmalloc
    Threads::Threads
    ${TBB_LIB_LINK_LIBS}
    ${TBB_COMMON_LINK_LIBS}
)

install(TARGETS tbbmalloc_proxy
    EXPORT TBBTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

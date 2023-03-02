# Copyright (c) 2020-2022 Intel Corporation
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

add_library(tbbmalloc 
    backend.cpp
    backref.cpp
    frontend.cpp
    large_objects.cpp
    tbbmalloc.cpp
    ../tbb/itt_notify.cpp)
    
if (WIN32)
    target_sources(tbbmalloc PRIVATE tbbmalloc.rc)
endif()

add_library(TBB::tbbmalloc ALIAS tbbmalloc)

target_compile_definitions(tbbmalloc
                           PUBLIC
                           $<$<CONFIG:DEBUG>:TBB_USE_DEBUG>
                           PRIVATE
                           __TBBMALLOC_BUILD
                           $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:__TBB_DYNAMIC_LOAD_ENABLED=0>
                           $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:__TBB_SOURCE_DIRECTLY_INCLUDED=1>)

if (NOT ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(armv7-a|aarch64|mips|arm64)" OR
         "${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64" OR
         WINDOWS_STORE OR
         TBB_WINDOWS_DRIVER OR
         TBB_SANITIZE MATCHES "thread"))
    target_compile_definitions(tbbmalloc PRIVATE __TBB_USE_ITT_NOTIFY)
endif()

target_include_directories(tbbmalloc
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE # CJH: backport compatibility - 2020 sources in other folders!
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>  #"tbb/tbb_environment.h": No such file or directory 
)

# TODO: fix warnings
if (MSVC)
    # signed unsigned mismatch, declaration hides class member
    set(TBB_WARNING_SUPPRESS ${TBB_WARNING_SUPPRESS} /wd4267 /wd4244 /wd4245 /wd4018 /wd4458)
endif()

# TODO: add ${TBB_WARNING_LEVEL} and fix problems
target_compile_options(tbbmalloc
    PRIVATE
    ${TBB_CXX_STD_FLAG} # TODO: consider making it PUBLIC.
    ${TBB_MMD_FLAG}
    ${TBB_DSE_FLAG}
    ${TBB_WARNING_SUPPRESS}
    ${TBB_LIB_COMPILE_FLAGS}
    ${TBBMALLOC_LIB_COMPILE_FLAGS}
    ${TBB_COMMON_COMPILE_FLAGS}
)

enable_language(C)

# Avoid use of target_link_libraries here as it changes /DEF option to \DEF on Windows.
set_target_properties(tbbmalloc PROPERTIES
    DEFINE_SYMBOL ""
    VERSION ${TBBMALLOC_BINARY_VERSION}.${TBB_BINARY_MINOR_VERSION}
    SOVERSION ${TBBMALLOC_BINARY_VERSION}
    LINKER_LANGUAGE C
)

tbb_handle_ipo(tbbmalloc)

if (TBB_DEF_FILE_PREFIX) # If there's no prefix, assume we're using export directives
    if (MSVC)
      add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/tbbmalloc.def"
        COMMAND ${CMAKE_CXX_COMPILER} /TC /EP ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbmalloc-export.def  -I ${PROJECT_SOURCE_DIR}/include > "${CMAKE_BINARY_DIR}/tbbmalloc.def"
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbmalloc-export.def
        COMMENT "Preprocessing tbbmalloc.def"
      )
    else()
      add_custom_command(OUTPUT "${CMAKE_BINARY_DIR}/tbbmalloc.def"
        COMMAND ${CMAKE_CXX_COMPILER} -xc++ -std=c++11 -E ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbmalloc-export.def  -I ${PROJECT_SOURCE_DIR}/include -o "${CMAKE_BINARY_DIR}/tbbmalloc.def"
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbmalloc-export.def
        COMMENT "Preprocessing tbbmalloc.def"
      )
    endif()
    
    target_sources(tbbmalloc PRIVATE ${CMAKE_BINARY_DIR}/tbbmalloc.def)
    set_target_properties(tbbmalloc PROPERTIES
        LINK_FLAGS "${TBB_LINK_DEF_FILE_FLAG}\"${CMAKE_BINARY_DIR}/tbbmalloc.def\""
        LINK_DEPENDS "${CMAKE_BINARY_DIR}/tbbmalloc.def"
    )
endif()

set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")

# Prefer using target_link_options instead of target_link_libraries to specify link options because
# target_link_libraries may incorrectly handle some options (on Windows, for example).
target_link_options(tbbmalloc
    PRIVATE
    ${TBB_LIB_LINK_FLAGS}
    ${TBB_COMMON_LINK_FLAGS}
)

target_link_libraries(tbbmalloc
    PRIVATE
    Threads::Threads
    ${TBB_LIB_LINK_LIBS}
    ${TBB_COMMON_LINK_LIBS}
)

tbb_install_target(tbbmalloc)


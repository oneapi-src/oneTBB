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

add_library(tbb )

target_sources( tbb PRIVATE
    ../old/concurrent_queue_v2.cpp
    ../old/concurrent_vector_v2.cpp
    ../old/spin_rw_mutex_v2.cpp
    ../old/task_v2.cpp
    arena.cpp
    cache_aligned_allocator.cpp
    concurrent_hash_map.cpp
    concurrent_monitor.cpp
    concurrent_queue.cpp
    concurrent_vector.cpp
    condition_variable.cpp
    critical_section.cpp
    dynamic_link.cpp
    governor.cpp
    itt_notify.cpp
    market.cpp
    mutex.cpp
    observer_proxy.cpp
    pipeline.cpp
    private_server.cpp
    queuing_mutex.cpp
    queuing_rw_mutex.cpp
    reader_writer_lock.cpp
    recursive_mutex.cpp
    scheduler.cpp
    semaphore.cpp
    spin_mutex.cpp
    spin_rw_mutex.cpp
    task.cpp
    task_group_context.cpp
    tbb_main.cpp
    tbb_misc.cpp
    tbb_misc_ex.cpp
    tbb_statistics.cpp
    tbb_thread.cpp
    x86_rtm_rw_mutex.cpp
    ../rml/client/rml_tbb.cpp
)

if(WIN32)
    #target_compile_definitions(tbb_interface INTERFACE USE_WINTHREAD _WIN32_WINNT=0x0600)
    if (MSVC AND NOT "${MSVC_CXX_ARCHITECTURE_ID}" MATCHES "ARM64" )
        enable_language(ASM_MASM)

        # CJH: TODO - support target_compile_options(tbb INTERFACE $<$<COMPILE_LANGUAGE:CXX>:/wd4267 /wd4800 /wd4146 /wd4244 /wd4577 /wd4018>)
        if (NOT CMAKE_SIZEOF_VOID_P)
           message(FATAL_ERROR "'CMAKE_SIZEOF_VOID_P' is undefined. Please delete your build directory and rerun CMake again!")
        endif()

        if (CMAKE_SIZEOF_VOID_P EQUAL 8)
            target_sources(tbb PRIVATE
                intel64-masm/atomic_support.asm
                intel64-masm/itsx.asm 
                intel64-masm/intel64_misc.asm)
            set_source_files_properties(
                intel64-masm/atomic_support.asm
                intel64-masm/itsx.asm 
                intel64-masm/intel64_misc.asm
                intel64-masm/atomic_support.asm
                PROPERTIES 
                    LANGUAGE ASM_MASM)
            
                list(APPEND tbbmalloc_src intel64-masm/atomic_support.asm)
                set(CMAKE_ASM_MASM_FLAGS "/DEM64T=1 ${CMAKE_ASM_MASM_FLAGS}")
        else()
            target_sources(tbb PRIVATE 
                ia32-masm/atomic_support.asm
                ia32-masm/itsx.asm
                ia32-masm/lock_byte.asm)
            set_source_files_properties(
                ia32-masm/atomic_support.asm
                ia32-masm/itsx.asm 
                ia32-masm/lock_byte.asm
                PROPERTIES 
                    LANGUAGE ASM_MASM)
            # Enable SAFESEH feature for assembly (x86 builds only).
            set(CMAKE_ASM_MASM_FLAGS "/safeseh ${CMAKE_ASM_MASM_FLAGS}")
        endif()
    endif()
endif()

add_library(TBB::tbb ALIAS tbb)

if (WIN32)
    target_sources(tbb PRIVATE tbb.rc)
    set_target_properties(tbb PROPERTIES OUTPUT_NAME "tbb${TBB_BINARY_VERSION}")
endif()

# TODO: Add statistics.cpp

target_compile_definitions(tbb
                           PUBLIC
                           $<$<CONFIG:DEBUG>:TBB_USE_DEBUG>
                           PRIVATE
                           __TBB_BUILD
                           $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:__TBB_DYNAMIC_LOAD_ENABLED=0>
                           $<$<NOT:$<BOOL:${BUILD_SHARED_LIBS}>>:__TBB_SOURCE_DIRECTLY_INCLUDED=1>)

if (NOT ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "(armv7-a|aarch64|mips|arm64)" OR
         "${CMAKE_OSX_ARCHITECTURES}" MATCHES "arm64" OR
         WINDOWS_STORE OR
         TBB_WINDOWS_DRIVER))
    target_compile_definitions(tbb PRIVATE __TBB_USE_ITT_NOTIFY)
endif()

target_include_directories(tbb
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE # CJH: backport compatibility - 2020 sources in other folders!
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>  #'tbb/dynamic_link.h': No such file or directory 
)

target_compile_options(tbb
    PRIVATE
    ${TBB_CXX_STD_FLAG} # TODO: consider making it PUBLIC.
    ${TBB_MMD_FLAG}
    ${TBB_DSE_FLAG}
    $<$<COMPILE_LANGUAGE:CXX>:${TBB_WARNING_LEVEL}>
    ${TBB_WARNING_SUPPRESS}
    ${TBB_LIB_COMPILE_FLAGS}
    ${TBB_COMMON_COMPILE_FLAGS}
)

# Avoid use of target_link_libraries here as it changes /DEF option to \DEF on Windows.
set_target_properties(tbb PROPERTIES
    DEFINE_SYMBOL ""
    VERSION ${TBB_BINARY_VERSION}.${TBB_BINARY_MINOR_VERSION}
    SOVERSION ${TBB_BINARY_VERSION}
)

tbb_handle_ipo(tbb)

if (TBB_DEF_FILE_PREFIX) # If there's no prefix, assume we're using export directives

    if (MINGW)
      set (TBB_DEF_FILE_PREFIX "${TBB_DEF_FILE_PREFIX}-gcc")
      # there's no win32-gcc-tbb-export.def, use lin32-tbb-export.def
      execute_process (COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/def/lin32-tbb-export.def ${CMAKE_CURRENT_SOURCE_DIR}/def/win32-gcc-tbb-export.def)
    endif()

    if (MSVC)
      add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/tbb.def
        COMMAND ${CMAKE_CXX_COMPILER} /TC /EP ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.def  -I ${PROJECT_SOURCE_DIR}/include > ${CMAKE_BINARY_DIR}/tbb.def
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.def
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.lst
        COMMENT "Preprocessing tbb.def"
      )
    else()
      add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/tbb.def
        COMMAND ${CMAKE_CXX_COMPILER} -xc++ -std=c++11 -E ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.def  -I ${PROJECT_SOURCE_DIR}/include -o ${CMAKE_BINARY_DIR}/tbb.def
        MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.def
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbb-export.lst
        COMMENT "Preprocessing tbb.def"
      )
    endif()
    
    target_sources(tbb PRIVATE ${CMAKE_BINARY_DIR}/tbb.def)
    set_target_properties(tbb PROPERTIES
        LINK_FLAGS "${TBB_LINK_DEF_FILE_FLAG}\"${CMAKE_BINARY_DIR}/tbb.def\""
        LINK_DEPENDS ${CMAKE_BINARY_DIR}/tbb.def
    )
endif()

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

tbb_install_target(tbb)

if (MSVC)
    # Create a copy of target linker file (tbb<ver>[_debug].lib) with legacy name (tbb[_debug].lib)
    # to support previous user experience for linkage.
    install(FILES
            $<TARGET_LINKER_FILE:tbb>
            DESTINATION lib
            CONFIGURATIONS RelWithDebInfo Release MinSizeRel
            RENAME tbb.lib
            COMPONENT devel
    )

    install(FILES
            $<TARGET_LINKER_FILE:tbb>
            DESTINATION lib
            CONFIGURATIONS Debug
            RENAME tbb_debug.lib
            COMPONENT devel
    )
endif()

set(_tbb_pc_lib_name tbb)

if (WIN32)
    set(_tbb_pc_lib_name ${_tbb_pc_lib_name}${TBB_BINARY_VERSION})
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(TBB_PC_NAME tbb)
else()
    set(TBB_PC_NAME tbb32)
endif()

set(_prefix_for_pc_file "${CMAKE_INSTALL_PREFIX}")

if (IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
    set(_libdir_for_pc_file "${CMAKE_INSTALL_LIBDIR}")
else()
    set(_libdir_for_pc_file "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()

if (IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
    set(_includedir_for_pc_file "${CMAKE_INSTALL_INCLUDEDIR}")
else()
    set(_includedir_for_pc_file "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()

configure_file(${PROJECT_SOURCE_DIR}/integration/pkg-config/tbb.pc.in ${CMAKE_CURRENT_BINARY_DIR}/${TBB_PC_NAME}.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${TBB_PC_NAME}.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig/
        COMPONENT devel)

if (COMMAND tbb_gen_vars)
    tbb_gen_vars(tbb)
endif()

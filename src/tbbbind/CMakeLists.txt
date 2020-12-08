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

macro(tbbbind_build TBBBIND_NAME HWLOC_VERSION)
    find_package(HWLOC ${HWLOC_VERSION} EXACT)
    if (NOT HWLOC_FOUND)
        message(STATUS "HWLOC ${HWLOC_VERSION} was not found, the corresponding TBBBind library target was disabled.")
    else()
        add_library(${TBBBIND_NAME} SHARED tbb_bind.cpp)
        add_library(TBB::${TBBBIND_NAME} ALIAS ${TBBBIND_NAME})

        target_compile_definitions(${TBBBIND_NAME} PRIVATE __TBBBIND_BUILD)

        target_include_directories(${TBBBIND_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
            $<INSTALL_INTERFACE:include>)

        target_compile_options(${TBBBIND_NAME}
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
        set_target_properties(${TBBBIND_NAME} PROPERTIES
            DEFINE_SYMBOL ""
            VERSION ${TBBBIND_BINARY_VERSION}.${TBB_BINARY_MINOR_VERSION}
            SOVERSION ${TBBBIND_BINARY_VERSION}
            LINK_FLAGS ${TBB_LINK_DEF_FILE_FLAG}${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbbind.def
            LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/def/${TBB_DEF_FILE_PREFIX}-tbbbind.def
        )

        # Prefer using target_link_options instead of target_link_libraries to specify link options because
        # target_link_libraries may incorrectly handle some options (on Windows, for example).
        if (COMMAND target_link_options)
            target_link_options(${TBBBIND_NAME}
                PRIVATE
                ${TBB_LIB_LINK_FLAGS}
                ${TBB_COMMON_LINK_FLAGS}
            )
        else()
            target_link_libraries(${TBBBIND_NAME}
                PRIVATE
                ${TBB_LIB_LINK_FLAGS}
                ${TBB_COMMON_LINK_FLAGS}
            )
        endif()

        string(REPLACE "." "_" HWLOC_TARGET_SUFFIX "${HWLOC_VERSION}")
        target_link_libraries(${TBBBIND_NAME}
            PUBLIC
            HWLOC::hwloc_${HWLOC_TARGET_SUFFIX}
            PRIVATE
            ${TBB_LIB_LINK_LIBS}
            ${TBB_COMMON_LINK_LIBS}
        )
        unset(HWLOC_TARGET_SUFFIX)

        tbb_install_target(${TBBBIND_NAME})

        if (COMMAND tbb_gen_vars)
            tbb_gen_vars(${TBBBIND_NAME})
        endif()
    endif()
endmacro()

message(STATUS "Trying to find HWLOC (versions 2 and 1.11) to build TBBBind library")
tbbbind_build(tbbbind 1.11)
tbbbind_build(tbbbind_2_0 2)

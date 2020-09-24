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

include(FindPackageHandleStandardArgs)

# Firstly search for HWLOC in config mode (i.e. search for HWLOCConfig.cmake).
find_package(HWLOC QUIET CONFIG)
if (HWLOC_FOUND)
    find_package_handle_standard_args(HWLOC CONFIG_MODE)
    return()
endif()

find_program(_hwloc_info_exe
    NAMES hwloc-info
    PATHS ENV HWLOC_ROOT ENV PATH
    PATH_SUFFIXES bin
)

if (_hwloc_info_exe)
    execute_process(
        COMMAND ${_hwloc_info_exe} "--version"
        OUTPUT_VARIABLE _hwloc_info_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REGEX MATCH "([0-9]+.[0-9]+.[0-9]+)$" HWLOC_VERSION "${_hwloc_info_output}")
    if ("${HWLOC_VERSION}" STREQUAL "")
        unset(HWLOC_VERSION)
    endif()

    unset(_hwloc_info_output)
    unset(_hwloc_info_exe)
endif()

if (WIN32)
    list(APPEND _additional_lib_dirs ENV PATH ENV LIB)
    list(APPEND _additional_include_dirs ENV INCLUDE ENV CPATH)
    set(_hwloc_lib_name libhwloc)
else()
    list(APPEND _additional_lib_dirs ENV LIBRARY_PATH ENV LD_LIBRARY_PATH ENV DYLD_LIBRARY_PATH)
    list(APPEND _additional_include_dirs ENV CPATH ENV C_INCLUDE_PATH ENV CPLUS_INCLUDE_PATH ENV INCLUDE_PATH)
    set(_hwloc_lib_name hwloc)
endif()

if (NOT TARGET HWLOC::hwloc)
    find_path(_hwloc_include_dirs
        NAMES hwloc.h
        PATHS ${_additional_include_dirs}
        PATH_SUFFIXES "hwloc")

    if (_hwloc_include_dirs)
        add_library(HWLOC::hwloc SHARED IMPORTED)
        set_property(TARGET HWLOC::hwloc APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${_hwloc_include_dirs})

        find_library(_hwloc_lib_dirs ${_hwloc_lib_name} PATHS ${_additional_lib_dirs})
        if (_hwloc_lib_dirs)
            if (WIN32)
                set_target_properties(HWLOC::hwloc PROPERTIES
                                      IMPORTED_LOCATION "${_hwloc_lib_dirs}"
                                      IMPORTED_IMPLIB   "${_hwloc_lib_dirs}")
            else()
                set_target_properties(HWLOC::hwloc PROPERTIES
                                      IMPORTED_LOCATION "${_hwloc_lib_dirs}")
            endif()

            set(HWLOC_FOUND 1)
        endif()
    endif()
endif()

unset(_additional_include_dirs CACHE)
unset(_additional_lib_dirs CACHE)
unset(_hwloc_lib_name CACHE)

find_package_handle_standard_args(
    HWLOC
    REQUIRED_VARS _hwloc_include_dirs _hwloc_lib_dirs
    VERSION_VAR HWLOC_VERSION
)

unset(_hwloc_include_dirs CACHE)
unset(_hwloc_lib_dirs CACHE)

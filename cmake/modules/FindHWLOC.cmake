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

# Check Find module settings
# --------------------------------------------------------------------------------------------------
if (HWLOC_FIND_VERSION)
    if (NOT ${HWLOC_FIND_VERSION} STREQUAL "2" AND NOT ${HWLOC_FIND_VERSION} STREQUAL "1.11")
        message(FATAL_ERROR "This find module can find only following HWLOC versions: 1.11, 2")
    endif()
else()
    set(HWLOC_FIND_VERSION "1.11")
endif()

if (NOT HWLOC_FIND_VERSION_EXACT)
    message(FATAL_ERROR "Please pass exact argument to the find_package() call")
endif()

# Check required target availability
# --------------------------------------------------------------------------------------------------
string(REPLACE "." "_" _target_version_suffix "${HWLOC_FIND_VERSION}")
set(_hwloc_target_name "HWLOC::hwloc_${_target_version_suffix}")
if (TARGET ${_hwloc_target_name})
    unset(_hwloc_target_name)
    return()
endif()

# Search for HWLOC in config mode (i.e. search for HWLOCConfig.cmake).
# --------------------------------------------------------------------------------------------------
unset(HWLOC_FOUND)
find_package(HWLOC QUIET CONFIG)
if (HWLOC_FOUND)
    find_package_handle_standard_args(HWLOC CONFIG_MODE)
    return()
endif()

# Variables definition
# --------------------------------------------------------------------------------------------------
if (WIN32)
    list(APPEND _additional_lib_dirs $ENV{PATH} $ENV{LIB})
    list(APPEND _additional_include_dirs $ENV{INCLUDE} $ENV{CPATH})
else()
    list(APPEND _additional_lib_dirs $ENV{LIBRARY_PATH} $ENV{LD_LIBRARY_PATH} $ENV{DYLD_LIBRARY_PATH})
    list(APPEND _additional_include_dirs $ENV{CPATH} $ENV{C_INCLUDE_PATH} $ENV{CPLUS_INCLUDE_PATH} $ENV{INCLUDE_PATH})
endif()
list(APPEND _additional_tools_dirs $ENV{HWLOC_ROOT} $ENV{PATH})
set(_hwloc_lib_name libhwloc)

# We should not replace : by ; on Windows since it brakes the system paths (e.g. C:\...)
if (NOT WIN32)
    string(REPLACE ":" ";" _additional_lib_dirs "${_additional_lib_dirs}")
    string(REPLACE ":" ";" _additional_include_dirs "${_additional_include_dirs}")
    string(REPLACE ":" ";" _additional_tools_dirs "${_additional_tools_dirs}")
endif()

if (${HWLOC_FIND_VERSION} STREQUAL "2")
    set(_api_version_pattern "2[0-9a-fA-F]+$")
    if (NOT WIN32)
        set(_hwloc_lib_name "${_hwloc_lib_name}.so.15")
    endif()
elseif (${HWLOC_FIND_VERSION} STREQUAL "1.11")
    set(_api_version_pattern "10b[0-9a-fA-F]+$")
    if (NOT WIN32)
        set(_hwloc_lib_name "${_hwloc_lib_name}.so.5")
    endif()
endif()
set(_include_version_pattern "#define HWLOC_API_VERSION 0x000${_api_version_pattern}")

# Detect the HWLOC version
# --------------------------------------------------------------------------------------------------
macro(find_required_version _hwloc_info_exe)
    execute_process(COMMAND ${_hwloc_info_exe} "--version"
        OUTPUT_VARIABLE _hwloc_info_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if (_hwloc_info_output)
        string(REGEX MATCH "([0-9]+\.[0-9]+\.[0-9]+)$" _current_hwloc_version "${_hwloc_info_output}")

        string(REPLACE "." ";" _version_list ${_current_hwloc_version})
        list(GET _version_list 0 _current_hwloc_version_major)
        list(GET _version_list 1 _current_hwloc_version_minor)

        if (${HWLOC_FIND_VERSION_MAJOR} STREQUAL ${_current_hwloc_version_major})
            if(${HWLOC_FIND_VERSION_MINOR} STREQUAL "0" OR ${HWLOC_FIND_VERSION_MINOR} STREQUAL ${_current_hwloc_version_minor})
                set(HWLOC_VERSION ${_current_hwloc_version})
            endif()
        endif()

        unset(_hwloc_info_output)
        unset(_version_list)
        unset(_current_hwloc_version)
        unset(_current_hwloc_version_major)
        unset(_current_hwloc_version_minor)
        unset(_current_hwloc_version_patch)
    endif()
endmacro()

foreach(path ${_additional_tools_dirs})
    unset(HWLOC_VERSION)
    find_program(_hwloc_info_exe NAMES hwloc-info HINTS ${path} PATH_SUFFIXES bin NO_DEFAULT_PATH)
    if (_hwloc_info_exe)
        find_required_version(${_hwloc_info_exe})
        unset(_hwloc_info_exe CACHE)
        if (HWLOC_VERSION)
            break()
        endif()
    endif()
endforeach()
if (NOT HWLOC_VERSION)
    find_program(_hwloc_info_exe NAMES hwloc-info PATH_SUFFIXES bin)
    if (_hwloc_info_exe)
        find_required_version(${_hwloc_info_exe})
        unset(_hwloc_info_exe CACHE)
    endif()
endif()

# Find the include path
# --------------------------------------------------------------------------------------------------
foreach(path ${_additional_include_dirs})
    find_path(HWLOC_INCLUDE_PATH NAMES hwloc.h PATHS ${path} NO_DEFAULT_PATH)

    if (HWLOC_INCLUDE_PATH)
        file(STRINGS ${HWLOC_INCLUDE_PATH}/hwloc.h _hwloc_api_version
            REGEX "${_include_version_pattern}"
        )
        if (_hwloc_api_version)
            unset(_hwloc_api_version CACHE)
            break()
        endif()
        unset(HWLOC_INCLUDE_PATH CACHE)
    endif()
endforeach()

if (NOT HWLOC_INCLUDE_PATH)
    find_path(HWLOC_INCLUDE_PATH NAMES hwloc.h)
    if (HWLOC_INCLUDE_PATH)
        file(STRINGS ${HWLOC_INCLUDE_PATH}/hwloc.h _hwloc_api_version
            REGEX "${_include_version_pattern}"
        )
        if (NOT _hwloc_api_version)
            unset(HWLOC_INCLUDE_PATH CACHE)
        endif()
        unset(_hwloc_api_version CACHE)
    endif()
endif()

# Find the library path
# --------------------------------------------------------------------------------------------------
macro(check_hwloc_runtime_version _hwloc_lib_path)
    file(WRITE ${CMAKE_BINARY_DIR}/hwloc_version_check/version_check.cpp
        "#include <hwloc.h>\n"
        "int main() {printf(\"%x\", hwloc_get_api_version());}\n"
    )
    try_run(RUN_RESULT COMPILE_RESULT
        ${CMAKE_BINARY_DIR}/hwloc_version_check
        ${CMAKE_BINARY_DIR}/hwloc_version_check/version_check.cpp
        LINK_LIBRARIES ${_hwloc_lib_path}
        CMAKE_FLAGS "-DINCLUDE_DIRECTORIES=${HWLOC_INCLUDE_PATH}"
        RUN_OUTPUT_VARIABLE _execution_result
    )
    string(REGEX MATCH "${_api_version_pattern}" _runtime_api_version "${_execution_result}")
    if (_runtime_api_version)
        set(HWLOC_LIBRARY_PATH "${_hwloc_lib_path}")
    endif()
    unset(_execution_result CACHE)
    unset(_api_version_pattern CACHE)
endmacro()

if (HWLOC_INCLUDE_PATH)
    foreach(path ${_additional_lib_dirs})
        find_library(_hwloc_lib_path ${_hwloc_lib_name} PATHS ${path} NO_DEFAULT_PATH)
        if (_hwloc_lib_path)
            check_hwloc_runtime_version(${_hwloc_lib_path})
            unset(_hwloc_lib_path CACHE)
            if (HWLOC_LIBRARY_PATH)
                break()
            endif()
        endif()
    endforeach()
    if (NOT HWLOC_LIBRARY_PATH)
        find_library(_hwloc_lib_path ${_hwloc_lib_name})
        if (_hwloc_lib_path)
            check_hwloc_runtime_version(${_hwloc_lib_path})
            unset(_hwloc_lib_path CACHE)
        endif()
    endif()
endif()

# Define the library target
# --------------------------------------------------------------------------------------------------
if (HWLOC_VERSION AND HWLOC_INCLUDE_PATH AND HWLOC_LIBRARY_PATH)
    add_library(${_hwloc_target_name} SHARED IMPORTED)
    set_target_properties(${_hwloc_target_name} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${HWLOC_INCLUDE_PATH}")
    if (WIN32)
        set_target_properties(${_hwloc_target_name} PROPERTIES
                              IMPORTED_LOCATION "${HWLOC_LIBRARY_PATH}"
                              IMPORTED_IMPLIB   "${HWLOC_LIBRARY_PATH}")
    else()
        set_target_properties(${_hwloc_target_name} PROPERTIES
                              IMPORTED_LOCATION "${HWLOC_LIBRARY_PATH}")
    endif()

    set(HWLOC_${_target_version_suffix}_TARGET_DEFINED TRUE)
endif()

find_package_handle_standard_args(
    HWLOC
    REQUIRED_VARS HWLOC_${_target_version_suffix}_TARGET_DEFINED
    VERSION_VAR HWLOC_VERSION
)

unset(_additional_include_dirs CACHE)
unset(_additional_lib_dirs CACHE)
unset(_additional_tools_dirs CACHE)
unset(_hwloc_lib_name CACHE)
unset(_hwloc_lib_dirs CACHE)
unset(_api_version_pattern CACHE)
unset(_include_version_pattern CACHE)
unset(_hwloc_target_name)

unset(HWLOC_INCLUDE_PATH CACHE)
unset(HWLOC_LIBRARY_PATH)

unset(HWLOC_FIND_VERSION)

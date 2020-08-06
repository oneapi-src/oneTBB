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

if (NOT HWLOC_FOUND)
    find_path(HWLOC_INCLUDE_DIRS
        NAMES hwloc.h
        HINTS $ENV{INCLUDE} $ENV{CPATH} $ENV{C_INCLUDE_PATH} $ENV{INCLUDE_PATH}
        PATH_SUFFIXES "hwloc")

    if (UNIX)
        set(HWLOC_LIB_NAME hwloc)
    elseif(WIN32)
        set(HWLOC_LIB_NAME libhwloc)
    endif()

    find_library(HWLOC_LIBRARIES
        NAMES ${HWLOC_LIB_NAME}
        HINTS $ENV{LIBRARY_PATH} $ENV{LD_LIBRARY_PATH} $ENV{DYLD_LIBRARY_PATH})

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(HWLOC DEFAULT_MSG HWLOC_LIBRARIES HWLOC_INCLUDE_DIRS)

    mark_as_advanced(HWLOC_LIB_NAME)
endif()

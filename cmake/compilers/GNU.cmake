# Copyright (c) 2020-2021 Intel Corporation
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

set(TBB_LINK_DEF_FILE_FLAG -Wl,--version-script=)
set(TBB_DEF_FILE_PREFIX lin${TBB_ARCH})
set(TBB_WARNING_LEVEL -Wall -Wextra $<$<BOOL:${TBB_STRICT}>:-Werror> -Wfatal-errors)
set(TBB_TEST_WARNING_FLAGS -Wshadow -Wcast-qual -Woverloaded-virtual -Wnon-virtual-dtor)

set(TBB_MMD_FLAG -MMD)
if (CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
    set(TBB_COMMON_COMPILE_FLAGS -mrtm)
endif()

set(TBB_COMMON_LINK_LIBS dl)

# Ignore -Werror set through add_compile_options() or added to CMAKE_CXX_FLAGS if TBB_STRICT is disabled.
if (NOT TBB_STRICT AND COMMAND tbb_remove_compile_flag)
    tbb_remove_compile_flag(-Werror)
endif()

if (NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL Intel)
    # gcc 6.0 and later have -flifetime-dse option that controls elimination of stores done outside the object lifetime
    set(TBB_DSE_FLAG $<$<NOT:$<VERSION_LESS:${CMAKE_CXX_COMPILER_VERSION},6.0>>:-flifetime-dse=1>)
endif()

# Workaround for heavy tests
if ("${CMAKE_SYSTEM_PROCESSOR}" MATCHES "mips")
    set(TBB_TEST_COMPILE_FLAGS ${TBB_TEST_COMPILE_FLAGS} -DTBB_TEST_LOW_WORKLOAD)
endif()

# TBB malloc settings
set(TBBMALLOC_LIB_COMPILE_FLAGS -fno-rtti -fno-exceptions)
set(TBB_OPENMP_FLAG -fopenmp)

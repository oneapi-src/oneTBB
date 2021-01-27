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

option(TBB_VALGRIND_MEMCHECK "Enable scan for memory leaks using Valgrind" OFF)

if (NOT TBB_VALGRIND_MEMCHECK)
    return()
endif()

add_custom_target(memcheck-all
    COMMENT "Run memcheck on all tests")

find_program(VALGRIND_EXE valgrind)

if (NOT VALGRIND_EXE)
    message(FATAL_ERROR "Valgrind executable is not found, add tool to PATH or turn off TBB_VALGRIND_MEMCHECK")
else()
    message(STATUS "Found Valgrind to run memory leak scan")
endif()

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/memcheck)

function(_tbb_run_memcheck test_target)
    set(target_name memcheck-${test_target})
    add_custom_target(${target_name} 
        COMMAND ${VALGRIND_EXE} --leak-check=full --show-leak-kinds=all --log-file=${CMAKE_BINARY_DIR}/memcheck/${target_name}.log -v $<TARGET_FILE:${test_target}>)
    add_dependencies(memcheck-all ${target_name})
endfunction()

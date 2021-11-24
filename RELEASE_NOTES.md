<!--
******************************************************************************
* 
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/-->

# Release Notes <!-- omit in toc -->
This document contains changes of oneTBB compared to the last release.

## Table of Contents <!-- omit in toc -->
- [Preview Features](#preview-features)
- [Known Limitations](#known-limitations)
- [Fixed Issues](#fixed-issues)
- [Open-source Contributions Integrated](#open-source-contributions-integrated)


## :computer: Preview Features
- Introduced the collaborative_call_once algorithm that executes the callable object exactly once, but allows other threads to join oneTBB parallel construction used inside this callable object. Inspired by Ben FrantzDale and Henry Heffan.

## :rotating_light: Known Limitations
- An application using Parallel STL algorithms in libstdc++ versions 9 and 10 may fail to compile due to incompatible interface changes between earlier versions of Threading Building Blocks (TBB) and oneAPI Threading Building Blocks (oneTBB). Disable support for Parallel STL algorithms by defining PSTL_USE_PARALLEL_POLICIES (in libstdc++ 9) or _GLIBCXX_USE_TBB_PAR_BACKEND (in libstdc++ 10) macro to zero before inclusion of the first standard header file in each translation unit.
- On Linux* OS, if oneAPI Threading Building Blocks (oneTBB) or Threading Building Blocks (TBB) are installed in a system folder like /usr/lib64, the application may fail to link due to the order in which the linker searches for libraries. Use the -L linker option to specify the correct location of oneTBB library. This issue does not affect the program execution.
- The oneapi::tbb::info namespace interfaces might unexpectedly change the process affinity mask on Windows* OS systems (see https://github.com/open-mpi/hwloc/issues/366 for details) when using hwloc version lower than 2.5.
- Using a hwloc version other than 1.11, 2.0, or 2.5 may cause an undefined behavior on Windows OS. See https://github.com/open-mpi/hwloc/issues/477 for details.
- The NUMA topology may be detected incorrectly on Windows OS machines where the number of NUMA node threads exceeds the size of 1 processor group.
- On Windows OS on ARM64*, when compiling an application using oneTBB with the Microsoft* Compiler, the compiler issues a warning C4324 that a structure was padded due to the alignment specifier. Consider suppressing the warning by specifying /wd4324 to the compiler command line.
- oneTBB does not support fork(), to work-around the issue, consider using task_scheduler_handle to join oneTBB worker threads before using fork().

## :hammer: Fixed Issues
- Enabled full support of Address Sanitizer and Thread Sanitizer.
- Fixed a race condition in tbbmalloc that may cause a crash in realloc() when using tbbmalloc_proxy.
- Enabled GCC* 11 support.
- Fixed limiter_node behavior when an integral type is used as an argument for the DecrementType template parameter.
- Fixed a possible memory leak when the static or affinity partitioners are used.
- Fixed possible system oversubscription when using a process mask and an explicit number of threads in the task arena.

## :octocat: Open-source Contributions Integrated
- Enabled PowerPC* Linux* OS support. Contributed by Mircho Rodozov (https://github.com/oneapi-src/oneTBB/pull/461)
- Improved UNIX* system support and enabled QNX Neutrino* RTOS support. Contributed by Pablo Romero (https://github.com/oneapi-src/oneTBB/pull/440)
- Enabled experimental Bazel* build system support. Contributed by Julian Amann (https://github.com/oneapi-src/oneTBB/pull/442)
- Enabled oneTBB build for Windows* OS on ARM64*. Contributed by Michael Vitrano (https://github.com/oneapi-src/oneTBB/pull/507)
- Added MinGW* and export attributes support. Contributed by Long Nguyen (https://github.com/oneapi-src/oneTBB/pull/351)

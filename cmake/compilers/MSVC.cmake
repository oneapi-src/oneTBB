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

set(TBB_LINK_DEF_FILE_FLAG ${CMAKE_LINK_DEF_FILE_FLAG})
set(TBB_DEF_FILE_PREFIX win${TBB_ARCH})

# Workaround for CMake issue https://gitlab.kitware.com/cmake/cmake/issues/18317.
# TODO: consider use of CMP0092 CMake policy.
string(REGEX REPLACE "/W[0-4]" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

# Warning suppression C4324: structure was padded due to alignment specifier
set(TBB_WARNING_LEVEL $<$<NOT:$<CXX_COMPILER_ID:Intel>>:/W4> $<$<BOOL:${TBB_STRICT}>:/WX>)
set(TBB_WARNING_SUPPRESS /wd4324 /wd4530 /wd4577)
set(TBB_TEST_COMPILE_FLAGS /bigobj)
set(TBB_LIB_COMPILE_FLAGS -D_CRT_SECURE_NO_WARNINGS /GS)

set(TBB_COMMON_COMPILE_FLAGS /volatile:iso /FS)

if (WINDOWS_STORE OR TBB_WINDOWS_DRIVER)
    set(TBB_COMMON_COMPILE_FLAGS ${TBB_COMMON_COMPILE_FLAGS} /D_WIN32_WINNT=0x0A00)
    set(TBB_COMMON_LINK_FLAGS /NODEFAULTLIB:kernel32.lib /INCREMENTAL:NO)
    set(TBB_COMMON_LINK_LIBS OneCore.lib)
endif()

if (WINDOWS_STORE)
    if (NOT CMAKE_SYSTEM_VERSION EQUAL 10.0)
        message(FATAL_ERROR "CMAKE_SYSTEM_VERSION must be equal to 10.0")
    endif()

    set(TBB_COMMON_COMPILE_FLAGS ${TBB_COMMON_COMPILE_FLAGS} /ZW /ZW:nostdlib)

    # CMake define this extra lib, remove it for this build type
    string(REGEX REPLACE "WindowsApp.lib" "" CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES}")

    if (TBB_NO_APPCONTAINER)
        set(TBB_LIB_LINK_FLAGS ${TBB_LIB_LINK_FLAGS} /APPCONTAINER:NO)
    endif()
endif()

if (TBB_WINDOWS_DRIVER)
    # Since this is universal driver disable this variable
    set(CMAKE_SYSTEM_PROCESSOR "")

    # CMake define list additional libs, remove it for this build type
    set(CMAKE_CXX_STANDARD_LIBRARIES "")

    set(TBB_COMMON_COMPILE_FLAGS ${TBB_COMMON_COMPILE_FLAGS} /D _UNICODE /DUNICODE /DWINAPI_FAMILY=WINAPI_FAMILY_APP /D__WRL_NO_DEFAULT_LIB__)
endif()

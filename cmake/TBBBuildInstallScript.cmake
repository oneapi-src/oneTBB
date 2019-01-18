# Copyright (c) 2017-2019 Intel Corporation
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
#
#
# This file is a cmake wrapper around existing tools to facilitate
# building tbb in a way that installs the files
# consistent with standard directory layouts
#

#
# CMAKE_INSTALL_PREFIX  <- Where the installed resources should be placed
#    ${CMAKE_INSTALL_PREFIX}/lib             <- The library files (both static & shared, and both release & debug)
#    ${CMAKE_INSTALL_PREFIX}/lib/cmake/TBB   <- Where the cmake package files should be installed
#    ${CMAKE_INSTALL_PREFIX/include          <- The include files
# SYSTEM_NAME                                <- The system name for the binaries (i.e. Darwin/Linux/Windows/Android)
# TBB_BUILD_DIR:PATH                         <- Where temporary object files are to be generated
# TBB_BUILD_PREFIX:STRING                    <- BUILDPREFIX, or os build prefix
# TBB_RELATIVE_CMAKE_CONFIG_PATH             <- on installation, relative to CMAEK_INSTALL_PREFIX, where should the cmake package files be placed
# TBB_COMPILER:STRING                        <- (optional) clang or gcc
# TBB_BUILD_STATIC:BOOL                      <- (optional) By default static libs are not built.
# TBB_BUILD_SHARED:BOOL                      <- (optional) By default shared libs are built.
# TBB_DO_EXTERNAL_CMAKE_BUILD_TEST:BOOL      <- (optional) After install, do test against the install

message( STATUS " ## Example command line usage
## Darwin
cmake  \
     -DCMAKE_INSTALL_PREFIX=/tmp/tbbinstall                                        \
     -DSYSTEM_NAME:STRING=Darwin                                                   \
     -DTBB_BUILD_DIR:PATH=/tmp/tbbbuild                                            \
     -DTBB_BUILD_PREFIX:STRING=mac_OSX_BLDPRX                                      \
     -DTBB_RELATIVE_CMAKE_CONFIG_PATH:PATH=lib/dummy1/cmake/dummy2/TBB/dummy3      \
     -DTBB_COMPILER:STRING=clang                                                   \
     -DTBB_BUILD_STATIC:BOOL=ON                                                    \
     -DTBB_BUILD_SHARED:BOOL=ON                                                    \
     -DTBB_DO_EXTERNAL_CMAKE_BUILD_TEST:BOOL=ON                                    \
     -P cmake/TBBBuildInstallScript.cmake

# Linux
cmake \
      -DCMAKE_INSTALL_PREFIX=/tmp/tbbinstall2                                      \
      -DSYSTEM_NAME:STRING=Linux                                                   \
      -DTBB_BUILD_DIR:PATH=/tmp/tbbbuild2                                          \
      -DTBB_BUILD_PREFIX:STRING=lnxPX                                              \
      -DTBB_RELATIVE_CMAKE_CONFIG_PATH:PATH=lib/dummy1/cmake/dummy2/TBB/lnx        \
      -DTBB_BUILD_STATIC:BOOL=ON                                                   \
      -DTBB_BUILD_SHARED:BOOL=ON                                                   \
      -DTBB_DO_EXTERNAL_CMAKE_BUILD_TEST:BOOL=ON                                   \
      -DTBB_COMPILER:STRING=clang                                                  \
      -P cmake/TBBBuildInstallScript.cmake
"
)

get_filename_component(TBB_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/../ ABSOLUTE)

if(NOT MAKE_ARGS)
  set(MAKE_ARGS "")
endif()
if(TBB_COMPILER)
  list(APPEND MAKE_ARGS "compiler=${TBB_COMPILER}")
endif()
if(NOT TBB_BUILD_DIR)
  set(TBB_BUILD_DIR "${TBB_SOURCE_DIR}/unix_cmake_bld")
endif()
list(APPEND MAKE_ARGS "tbb_build_dir=${TBB_BUILD_DIR}")
if(NOT TBB_BUILD_PREFIX)
  set(TBB_BUILD_PREFIX "BUILDPREFIX")
endif()
list(APPEND MAKE_ARGS "tbb_build_prefix=${TBB_BUILD_PREFIX}")

if(SYSTEM_NAME)
    set(tbb_system_name ${SYSTEM_NAME})
else()
  set(tbb_system_name ${CMAKE_SYSTEM_NAME})
endif()
include(${CMAKE_CURRENT_LIST_DIR}/TBBPlatformDefaults.cmake)

if(NOT TBB_RELATIVE_CMAKE_CONFIG_PATH)
  set(TBB_RELATIVE_CMAKE_CONFIG_PATH "${TBB_SHARED_LIB_DIR}/cmake/TBB" )
endif()

# "CMAKE_SYSTEM_NAME" not defined by default in scripts executed with -P, use CMAKE_HOST_SYSTEM_NAME
set(CMAKE_SYSTEM_NAME ${CMAKE_HOST_SYSTEM_NAME})
include(${CMAKE_CURRENT_LIST_DIR}/TBBBuild.cmake)
message(STATUS "Building from source directory :${TBB_SOURCE_DIR}:")
message(STATUS "Building in temporary location: ${TBB_BUILD_DIR} under build prefix ${TBB_BUILD_PREFIX}")

if( NOT DEFINED TBB_BUILD_SHARED OR TBB_BUILD_SHARED )
  message(STATUS "Building shared libs with MAKE_ARGS:${MAKE_ARGS}:")
  tbb_build(TBB_ROOT ${TBB_SOURCE_DIR}
            MAKE_ARGS ${MAKE_ARGS}
            CONFIG_DIR tbb_config_dir
            TBB_CMAKE_PACKAGE_CONFIG_DIR ${TBB_BUILD_DIR}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}
          )
endif()

if( TBB_BUILD_STATIC )
  list(APPEND MAKE_ARGS "extra_inc=big_iron.inc") #<- big_iron.inc is included to force build of static libs
  message(STATUS "Building static libs with MAKE_ARGS:${MAKE_ARGS}:")
  tbb_build(TBB_ROOT ${TBB_SOURCE_DIR}
            MAKE_ARGS ${MAKE_ARGS}
            CONFIG_FOR_INSTALL
            CONFIG_DIR tbb_config_dir
            TBB_CMAKE_PACKAGE_CONFIG_DIR ${TBB_BUILD_DIR}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}
          )
endif()
message(STATUS "Configured temporary cmake config files at:${tbb_build_CONFIG_DIR}:") # set from inside tbb_build

##------------------------------------- Install in standard unix directory organization --------------------------------
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX})
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${TBB_SHARED_LIB_DIR})
file(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH})

## First copy the include directory
file(COPY ${TBB_SOURCE_DIR}/include
        DESTINATION ${CMAKE_INSTALL_PREFIX}
        FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE
        DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE
        )

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    get_filename_component(LIB_INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}/${TBB_SHARED_LIB_DIR}/${TBB_X32_SUBDIR}" ABSOLUTE)
else()
    get_filename_component(LIB_INSTALL_DESTINATION "${CMAKE_INSTALL_PREFIX}/${TBB_SHARED_LIB_DIR}/${TBB_X64_SUBDIR}" ABSOLUTE)
endif()

## Second copy the release and debug binary library files to a common lib directory
foreach( tbb_build_suffix "_release" "_debug")

  file(GLOB TBB_LIB_FILES ${TBB_BUILD_DIR}/${TBB_BUILD_PREFIX}${tbb_build_suffix}/${TBB_LIB_PREFIX}*.${TBB_LIB_EXT}
                          ${TBB_BUILD_DIR}/${TBB_BUILD_PREFIX}${tbb_build_suffix}/${TBB_LIB_PREFIX}*.${TBB_STATICLIB_EXT} )

  message(STATUS "INSTALLING: ${TBB_LIB_FILES}, from ${TBB_BUILD_DIR}/${TBB_BUILD_PREFIX}${tbb_build_suffix}")
  file(COPY ${TBB_LIB_FILES}
        DESTINATION ${LIB_INSTALL_DESTINATION}
        FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE
        DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE
  )
endforeach()

## Third copy the cmake configuration files
file(GLOB TBB_CMAKE_CONFIG_FILES ${tbb_config_dir}/*.cmake)
file(COPY ${TBB_CMAKE_CONFIG_FILES}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}
        FILE_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_WRITE
        DIRECTORY_PERMISSIONS OWNER_READ GROUP_READ WORLD_READ OWNER_EXECUTE GROUP_EXECUTE WORLD_EXECUTE OWNER_WRITE
)


####################################################
#
if( TBB_DO_EXTERNAL_CMAKE_BUILD_TEST )

  message(STATUS "For cmake find_package(TBB REQUIRED)
          TBB_DIR=${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}")

  set($ENV{TBB_DIR} "${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}")
  file(WRITE ${TBB_BUILD_DIR}/test_code/CMakeLists.txt
"cmake_minimum_required(VERSION 3.3.0)
set(CMAKE_CXX_STANDARD 11)
project(test_intel VERSION 0.0.1 LANGUAGES CXX)

find_package(TBB REQUIRED)
if(NOT TBB_FOUND)
message(FATAL_ERROR \"TBB not found\")
else()
message(STATUS \"TBB found at :\${TBB_DIR}\")
message(STATUS \"TBB_INTERFACE_VERSION :\${TBB_INTERFACE_VERSION}:\")
message(STATUS \"TBB_IMPORTED_TARGETS :\${TBB_IMPORTED_TARGETS}:\")
endif()

add_executable(tbb_test_application tbb_test_application.cxx)
target_link_libraries(tbb_test_application \${TBB_IMPORTED_TARGETS})
")


  file(WRITE ${TBB_BUILD_DIR}/test_code/tbb_test_application.cxx
"
#include <iostream>
#include \"tbb/tbb.h\"

void ParallelApplyFoo(int a[], size_t n) {
  tbb::parallel_for(size_t(0), n, size_t(1) , [=](size_t i) { a[i]++; });
}

int main()
{
  tbb::tick_count t0 = tbb::tick_count::now();
  const int low = tbb::task_scheduler_init::automatic;
  constexpr size_t N=10000;
  int a[N] = {1,2,3,4,5,6,7,8,9,10};
  ParallelApplyFoo(a,N);

  tbb::tick_count t1 = tbb::tick_count::now();
  std::cout << \"Time Elapsed: \" << ( t1 - t0 ).seconds() << std::endl;
  return EXIT_SUCCESS;
}
")
  message(STATUS "TESTING INSTALL REFERENCES -- CMAKE CONFIGURE against TBB: ${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH}\n")
  file(MAKE_DIRECTORY ${TBB_BUILD_DIR}/test_code-bld)
  execute_process(COMMAND ${CMAKE_COMMAND} -DTBB_DIR:PATH=${CMAKE_INSTALL_PREFIX}/${TBB_RELATIVE_CMAKE_CONFIG_PATH} ${TBB_BUILD_DIR}/test_code
          #COMMAND make
          WORKING_DIRECTORY ${TBB_BUILD_DIR}/test_code-bld
          RESULT_VARIABLE TEST_CONFIG_RESULT
          OUTPUT_VARIABLE TEST_CONFIG_OUTPUT
          ERROR_VARIABLE TEST_CONFIG_ERROR
          )
    if(NOT ${TEST_CONFIG_RESULT} EQUAL 0)
      message(FATAL_ERROR "${TEST_CONFIG_RESULT}\n${TEST_CONFIG_OUTPUT}\n${TEST_CONFIG_ERROR}")
    else()
      message(STATUS "TESTING INSTALL REFERENCES -- CMAKE BUILD\n")
      execute_process(COMMAND make VERBOSE=ON
              WORKING_DIRECTORY ${TBB_BUILD_DIR}/test_code-bld
              RESULT_VARIABLE TEST_BUILD_RESULT
              OUTPUT_VARIABLE TEST_BUILD_OUTPUT
              ERROR_VARIABLE TEST_BUILD_ERROR
              )
      if(NOT ${TEST_BUILD_RESULT} EQUAL 0)
        message(FATAL_ERROR "${TEST_BUILD_RESULT}\n${TEST_BUILD_OUTPUT}\n${TEST_BUILD_ERROR}")
      else()
        message(STATUS "TESTING OF EXTERNAL BUILD AGAINST INSTALLED TBB SUCCEEDED!")
      endif()
    endif()
  message(STATUS "TESTING COMPLETE \n\n")
endif()

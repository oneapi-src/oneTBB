#!/bin/bash
# \author Hans J. Johnson
#
# This is a wrapper around the various components
# tbb build environment needed to build and configure
# a typical unix file system layout with cmake packing
# support
#
# It requires make, python, and cmake 
# to get a standard build and install
# layout.
#
# Usage:  buld_tbb.sh -b build_directory -p install_prefix
#    EG:  ./build_tbb.sh -b /tmp -p /home/myaccout/local
#
# A test case then attempts to build an external application
# against the newly installed version of tbb by referencing
# the cmake packaging files.  The external application use
# of find_package(TBB) is purposefully more complicated
# than this test requires in order to simulate support
# for usecases where multiple modules may require
# different components of tbb, thus requiring multiple
# calls to find_package(TBB)
#
set -ev

while getopts ":b:p:" opt; do
  case ${opt} in
    b ) # process option for build CACHE
      BUILD_CACHE=$OPTARG
      ;;
    p ) # process option
      INSTALL_PREFIX=$OPTARG
      ;;
    \? ) echo "Usage: cmd [-h] [-t]"
      ;;
  esac
done

if [ -z ${BUILD_CACHE+x} ]; then
    echo "Usage was wrong, -b is a required option"
    exit -1
fi
if [ -z ${INSTALL_PREFIX+x} ]; then
    INSTALL_PREFIX=${BUILD_CACHE}/tbb-install
fi

SRC_DIR=${BUILD_CACHE}/tbb

SYSTEM_NAME=Darwin
TBB_VERSION_FILE=${SRC_DIR}/include/tbb/tbb_stddef.h

mkdir -p ${BUILD_CACHE}
mkdir -p ${INSTALL_PREFIX}

pushd ${BUILD_CACHE}
if [[ ! -d tbb ]]; then
  git clone https://github.com/AlexVeprev/tbb.git
fi
GIT_TAG=cmake-install-config
pushd tbb
  git checkout ${GIT_TAG}
popd


python ${SRC_DIR}/build/build.py  \
       --tbbroot ${SRC_DIR} \
       --prefix ${INSTALL_PREFIX} \
       --install-libs --install-devel --install-docs

cmake -DINSTALL_DIR=${INSTALL_PREFIX}/lib/cmake/tbb \
      -DLIB_REL_PATH=../../../lib \
      -DINC_REL_PATH=../../../include \
      -DSYSTEM_NAME=$(uname) \
      -DTBB_VERSION_FILE=${TBB_VERSION_FILE} \
      -P ${SRC_DIR}/cmake/tbb_config_installer.cmake


DO_TESTING=1
if [[ ${DO_TESTING} -eq 1 ]]; then

TEST_DIR=${BUILD_CACHE}/tbb_test
mkdir -p ${TEST_DIR}

cat > ${TEST_DIR}/CMakeLists.txt << EOF
# Put this cmake file in a directory, run "cmake ."  finding TBB results in a FATAL_ERROR from cmake
cmake_minimum_required(VERSION 3.0)
# tbb;tbbmalloc;tbbmalloc_proxy
find_package(TBB REQUIRED COMPONENTS tbbmalloc CONFIG)       #Find one component - test
message(STATUS "COMPONENTS FOUND: TBB_IMPORTED_TARGETS=:\${TBB_IMPORTED_TARGETS}: TBB_tbb_FOUND=:\${TBB_tbb_FOUND}: TBB_tbbmalloc_FOUND=:\${TBB_tbbmalloc_FOUND}: TBB_tbbmalloc_proxy_FOUND=:\${TBB_tbbmalloc_proxy_FOUND}:")
find_package(TBB REQUIRED COMPONENTS tbbmalloc_proxy CONFIG) #Find one component - test
message(STATUS "COMPONENTS FOUND: TBB_IMPORTED_TARGETS=:\${TBB_IMPORTED_TARGETS}: TBB_tbb_FOUND=:\${TBB_tbb_FOUND}: TBB_tbbmalloc_FOUND=:\${TBB_tbbmalloc_FOUND}: TBB_tbbmalloc_proxy_FOUND=:\${TBB_tbbmalloc_proxy_FOUND}:")
find_package(TBB REQUIRED COMPONENTS tbb CONFIG)             #Find one component - test
message(STATUS "COMPONENTS FOUND: TBB_IMPORTED_TARGETS=:\${TBB_IMPORTED_TARGETS}: TBB_tbb_FOUND=:\${TBB_tbb_FOUND}: TBB_tbbmalloc_FOUND=:\${TBB_tbbmalloc_FOUND}: TBB_tbbmalloc_proxy_FOUND=:\${TBB_tbbmalloc_proxy_FOUND}:")
find_package(TBB REQUIRED CONFIG) # Should be able to find_package many times
message(STATUS "COMPONENTS FOUND: TBB_IMPORTED_TARGETS=:\${TBB_IMPORTED_TARGETS}: TBB_tbb_FOUND=:\${TBB_tbb_FOUND}: TBB_tbbmalloc_FOUND=:\${TBB_tbbmalloc_FOUND}: TBB_tbbmalloc_proxy_FOUND=:\${TBB_tbbmalloc_proxy_FOUND}:")
if(NOT TBB_FOUND)
  message(FATAL_ERROR "NO TBB")
else()
  message(STATUS "FOUND TBB")
endif()
add_executable(tbb_test test.cpp)
target_link_libraries(tbb_test TBB::tbb)
EOF

cat > ${TEST_DIR}/test.cpp << EOF
#include <tbb/task_scheduler_init.h>
#include <iostream>
int main()
{
  std::cout << "Default number of threads: " << tbb::task_scheduler_init::default_num_threads() << std::endl;;
  return 0;
}
EOF

mkdir -p ${TEST_DIR}-bld
pushd ${TEST_DIR}-bld
cmake -DCMAKE_BUILD_TYPE:STRING=Release -DTBB_DIR:PATH=${INSTALL_PREFIX}/lib/cmake/tbb  ${TEST_DIR}
make
./tbb_test

if [[ $? -eq 0 ]]; then
  echo "SUCCESSFUL TESTING"
  exit 0
else
  echo "FAILED TESTING"
  exit -1
fi

fi

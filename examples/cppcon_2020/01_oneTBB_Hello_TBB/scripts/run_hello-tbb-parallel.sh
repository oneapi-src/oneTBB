#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction sample hello-tbb-parallel 
dpcpp -std=c++17 -O2 lab/hello-tbb-parallel.cpp -o bin/hello-tbb-parallel -tbb
bin/hello-tbb-parallel

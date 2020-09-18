#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction sample hello-tbb-solved 
dpcpp -std=c++17 -O2 solutions/hello-tbb-solved.cpp -o bin/hello-tbb-solved -tbb
bin/hello-tbb-solved

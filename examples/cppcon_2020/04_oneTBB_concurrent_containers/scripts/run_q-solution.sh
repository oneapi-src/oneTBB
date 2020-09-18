#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction exercise q-solution 
dpcpp -std=c++17 -O2 solutions/q-parallel.cpp -o bin/q-parallel-solution -tbb -lpthread
bin/q-parallel-solution

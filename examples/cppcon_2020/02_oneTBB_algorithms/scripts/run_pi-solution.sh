#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction sample pi-parallel 
dpcpp -std=c++17 -O2 solutions/pi-parallel.cpp -o bin/pi-parallel-solution -tbb
bin/pi-parallel-solution

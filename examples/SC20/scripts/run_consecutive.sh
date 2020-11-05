#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL exercise triad consecutive
dpcpp -std=c++20 -O2 lab/triad-consecutive.cpp -o bin/triad-consecutive -tbb -lpthread
if [ $? -eq 0 ]; then bin/triad-consecutive; fi

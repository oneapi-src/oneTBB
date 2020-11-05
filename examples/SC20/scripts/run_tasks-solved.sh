#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL solution using a task_group
dpcpp -std=c++20 -O2 solutions/triad-task_group-solved.cpp -o bin/triad-task_group-solved -tbb -lpthread
if [ $? -eq 0 ]; then bin/triad-task_group-solved; fi

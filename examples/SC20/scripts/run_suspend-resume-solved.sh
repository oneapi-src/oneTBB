#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL hetero solution using a resumable task
dpcpp -std=c++20 -O2 solutions/triad-hetero-suspend-resume-solved.cpp -o bin/triad-hetero-suspend-resume-solved -tbb -lpthread
if [ $? -eq 0 ]; then bin/triad-hetero-suspend-resume-solved; fi

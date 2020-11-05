#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL hetero exercise using a resumable task
dpcpp -std=c++20 -O2 lab/triad-hetero-suspend-resume.cpp -o bin/triad-hetero-suspend-resume -tbb -lpthread
if [ $? -eq 0 ]; then bin/triad-hetero-suspend-resume; fi

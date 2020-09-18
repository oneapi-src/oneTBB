#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction exercise pipeline-parallel 
dpcpp -std=c++17 -O2 lab/pipeline-parallel.cpp common/case.cpp -o bin/pipeline-parallel -tbb
bin/pipeline-parallel

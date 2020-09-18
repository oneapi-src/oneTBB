#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction exercise fwd-sub-solution 
dpcpp -std=c++17 -O2 solutions/fwd-sub-solution.cpp common/fwd_sub.cpp -o bin/fwd-sub-solution -tbb
bin/fwd-sub-solution

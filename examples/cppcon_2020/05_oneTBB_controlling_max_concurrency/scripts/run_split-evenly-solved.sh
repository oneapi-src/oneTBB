#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB exercise split-evenly
dpcpp -std=c++17 -O2 solutions/split-evenly-solved.cpp -o bin/split-evenly-solved -tbb
bin/split-evenly-solved

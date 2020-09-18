#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB exercise observer-solved
dpcpp -std=c++17 -O2 solutions/observer-solved.cpp -o bin/observer-solved -tbb
bin/observer-solved

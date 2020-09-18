#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB exercise scalability-gc-ta
dpcpp -std=c++17 -O2 lab/scalability-gc-ta.cpp -o bin/scalability-gc-ta -tbb
bin/scalability-gc-ta

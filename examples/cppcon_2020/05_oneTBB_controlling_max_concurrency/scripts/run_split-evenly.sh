#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB exercise split-evenly
dpcpp -std=c++17 -O2 lab/split-evenly.cpp -o bin/split-evenly -tbb
bin/split-evenly

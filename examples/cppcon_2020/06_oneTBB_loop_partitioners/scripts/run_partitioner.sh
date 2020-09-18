#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB exercise partitioner
dpcpp -std=c++17 -O2 lab/partitioner.cpp -o bin/partitioner -tbb
bin/partitioner

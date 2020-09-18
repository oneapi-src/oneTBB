#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling Parallel STL sample sort by keys
rm -rf bin/fancy_sort
dpcpp -std=c++17 -O2 lab/fancy_sort.cpp -o bin/fancy_sort -tbb
bin/fancy_sort

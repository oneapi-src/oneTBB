#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL hetero exercise using flow::async_node
dpcpp -std=c++20 -O2 lab/triad-hetero-async_node.cpp -o bin/triad-hetero-async_node -tbb -lpthread
if [ $? -eq 0 ]; then bin/triad-hetero-async_node; fi

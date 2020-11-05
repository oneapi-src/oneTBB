#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB with SYCL exercise using a task_group
dpcpp -std=c++20 -O2 lab/triad-task_group.cpp -o bin/triad-task_group -ltbb -lpthread
if [ $? -eq 0 ]; then bin/triad-task_group; fi

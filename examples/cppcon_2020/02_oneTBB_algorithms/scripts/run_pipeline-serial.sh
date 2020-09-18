#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction exercise pipeline-serial 
dpcpp -std=c++17 -O2 lab/pipeline-serial.cpp common/case.cpp -o bin/pipeline-serial -tbb
bin/pipeline-serial

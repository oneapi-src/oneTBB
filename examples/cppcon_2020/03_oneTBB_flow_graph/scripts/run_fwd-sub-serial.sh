#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling oneTBB Introduction exercise fwd-sub-serial 
dpcpp -std=c++17 -O2 lab/fwd-sub-serial.cpp common/fwd_sub.cpp -o bin/fwd-sub-serial -tbb
bin/fwd-sub-serial

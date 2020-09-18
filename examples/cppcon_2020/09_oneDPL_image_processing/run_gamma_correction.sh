#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling STL Introduction sample sort
rm -rf bin/gamma-correction
dpcpp -std=c++17 -O2 -I ./common lab/gamma-correction.cpp -o bin/gamma-correction -tbb
bin/gamma-correction

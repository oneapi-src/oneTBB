#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling TBB composability example Matrix multiplication
/bin/echo Configuration TBB with OpenMP
rm -rf bin/mat_mul
dpcpp -O2 -o bin/mat_mul lab/mat_mul.cpp -tbb -lmkl_rt -lrt -std=c++11
# options are INTEL,SEQUENTIAL,TBB
export MKL_THREADING_LAYER=INTEL
bin/mat_mul

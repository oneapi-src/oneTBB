#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling TBB composability example
rm -rf bin/cholesky
dpcpp -O2 -DNDEBUG -o bin/cholesky -I. lab/cholesky.cpp lab/init.cpp -tbb -lmkl_rt -lrt -std=c++11
# options are INTEL,SEQUENTIAL,TBB
export MKL_THREADING_LAYER=TBB
bin/cholesky 4096 256

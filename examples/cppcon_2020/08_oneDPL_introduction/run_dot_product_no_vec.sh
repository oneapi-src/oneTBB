#!/bin/bash
#==========================================
# Copyright (c) 2020 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
#==========================================

source /opt/intel/inteloneapi/setvars.sh > /dev/null 2>&1
/bin/echo "##" $(whoami) is compiling PSTL example
rm -rf bin/dot_product
icpc -std=c++17 -O2 -qopenmp-simd -no-vec lab/dot_product.cpp -o bin/dot_product -tbb -DONEDPL_STANDARD_POLICIES_ONLY
bin/dot_product

//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <vector>
#include <iostream>

std::vector<double> init_fwd_sub(std::vector<double>& x, 
                                 std::vector<double>& a, 
                                 std::vector<double>& b) {
  const int N = x.size();
  for (int i = 0; i < N; ++i) {
    x[i] = 0;
    b[i] = i*i;
    for (int j = 0; j <= i; ++j) {
      a[j + i*N] = 1 + j*i;
    }
  }

  std::vector<double> b_tmp = b;
  std::vector<double> x_gold = x;
  for (int i = 0; i < N; ++i) {
    for (int j = 0; j < i; ++j) {
      b_tmp[i] -= a[j + i*N] * x_gold[j];
    }
    x_gold[i] = b_tmp[i] / a[i + i*N];
  }
  return x_gold;
}

void check_fwd_sub(const std::vector<double>& x,
                   const std::vector<double>& x_gold) {
  if (x.size() != x_gold.size()) {
    std::cerr << "ERROR: Invalid result" << std::endl;
  }
  for (int i = 0; i < x.size(); ++i) {
    if (x[i] > 1.1*x_gold[i] || x[i] < 0.9*x_gold[i]) {
      std::cerr << "ERROR: Invalid result" 
                << "  at " << i << " " << x[i] << " != " << x_gold[i] << std::endl;
    }
  }
}  

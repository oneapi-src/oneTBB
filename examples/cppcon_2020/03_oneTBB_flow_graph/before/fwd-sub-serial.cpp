//==============================================================
// Copyright (c) 2020 Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0
// =============================================================

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>


// defined in common/fwd_sub.cpp
std::vector<double> init_fwd_sub(std::vector<double>& x,
                                 std::vector<double>& a,
                                 std::vector<double>& b); 

void check_fwd_sub(const std::vector<double>& x,
                   const std::vector<double>& x_gold); 
        
void fwd_sub_serial(std::vector<double>& x, const std::vector<double>& a, std::vector<double>& b) {
  const int N = x.size();
  const int block_size = 512;
  const int num_blocks = N / block_size;

  for ( int r = 0; r < num_blocks; ++r ) {
    for ( int c = 0; c <= r; ++c ) {
      int i_start = r*block_size, i_end = i_start + block_size;
      int j_start = c*block_size, j_max = j_start + block_size - 1;
      for (int i = i_start; i < i_end; ++i) {
        int j_end = (i <= j_max) ? i : j_max + 1;
        for (int j = j_start; j < j_end; ++j) {
          b[i] -= a[j + i*N] * x[j];
        }
        if (j_end == i) {
          x[i] = b[i] / a[i + i*N];
        }
      }
    }
  }
}

int main() {
  const int N = 32768;

  std::vector<double> a(N*N);
  std::vector<double> b(N);
  std::vector<double> x(N);

  auto x_gold = init_fwd_sub(x,a,b);

  double serial_time = 0.0;
  {
    auto st0 = std::chrono::high_resolution_clock::now();
    fwd_sub_serial(x,a,b);
    serial_time = 1e-9*(std::chrono::high_resolution_clock::now() - st0).count();
  }
  check_fwd_sub(x, x_gold);
  std::cout << "serial_time == " << serial_time << " seconds" << std::endl;
  return 0;
}
